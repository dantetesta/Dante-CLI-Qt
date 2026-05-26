#include "TerminalBuffer.h"
#include <algorithm>
#include <QtGlobal>

namespace dante::terminal {

// East Asian Wide / Fullwidth detection — minimal ranges good enough for CJK
// + most emoji. NOT a full Unicode East Asian Width table; we skip combining
// marks (treated as zero-width by ignoring them in putChar).
static bool isWide(char32_t cp) {
    if (cp < 0x1100) return false;
    return
        (cp >= 0x1100 && cp <= 0x115F) ||
        (cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3041 && cp <= 0x33FF) ||
        (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0x4E00 && cp <= 0x9FFF) ||
        (cp >= 0xA000 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE30 && cp <= 0xFE4F) ||
        (cp >= 0xFF00 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        (cp >= 0x1F300 && cp <= 0x1FAFF) ||  // emoji block (rough)
        (cp >= 0x20000 && cp <= 0x2FFFD) ||
        (cp >= 0x30000 && cp <= 0x3FFFD);
}

TerminalBuffer::TerminalBuffer() {
    resize(80, 24);
}

void TerminalBuffer::resize(int cols, int rows) {
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;

    auto resizeScreen = [&](Screen& s) {
        QVector<Cell> next(cols * rows);
        // Carry over existing content row-by-row from top. Skip the copy
        // entirely on first construction (cells empty) — rows/cols defaults
        // suggest a 80×24 grid we don't actually have yet.
        if (!s.cells.isEmpty()) {
            const int copyRows = std::min(s.rows, rows);
            const int copyCols = std::min(s.cols, cols);
            for (int r = 0; r < copyRows; ++r) {
                for (int c = 0; c < copyCols; ++c) {
                    next[r * cols + c] = s.cells[r * s.cols + c];
                }
            }
        }
        s.cells = std::move(next);
        s.cols = cols;
        s.rows = rows;
    };
    resizeScreen(primary_);
    resizeScreen(alt_);

    regionTop_ = 0;
    regionBottom_ = rows - 1;
    if (curRow_ >= rows) curRow_ = rows - 1;
    if (curCol_ >= cols) curCol_ = cols - 1;
    wrapPending_ = false;
}

void TerminalBuffer::setCursor(int row, int col) {
    curRow_ = std::clamp(row, 0, active_->rows - 1);
    curCol_ = std::clamp(col, 0, active_->cols - 1);
    wrapPending_ = false;
}

void TerminalBuffer::ensureCursorInBounds() {
    if (curRow_ < 0) curRow_ = 0;
    if (curRow_ >= active_->rows) curRow_ = active_->rows - 1;
    if (curCol_ < 0) curCol_ = 0;
    if (curCol_ >= active_->cols) curCol_ = active_->cols - 1;
}

const Cell* TerminalBuffer::lineCells(int vy) const {
    // vy in [-scrollback_.size(), rows)
    if (vy >= 0) return &active_->cells[vy * active_->cols];
    const int idx = scrollback_.size() + vy; // negative -> from end
    if (idx < 0 || idx >= scrollback_.size()) return nullptr;
    return scrollback_[idx].constData();
}

void TerminalBuffer::setScrollOffset(int o) {
    const int maxOff = scrollback_.size();
    scrollOffset_ = std::clamp(o, 0, maxOff);
}

void TerminalBuffer::pushToScrollback(int row) {
    if (active_ != &primary_) return; // alt screen has no scrollback
    QVector<Cell> line(active_->cols);
    const Cell* src = &active_->cells[row * active_->cols];
    std::copy(src, src + active_->cols, line.begin());
    scrollback_.push_back(std::move(line));
    if (scrollback_.size() > scrollbackMax_) {
        scrollback_.removeFirst();
    }
}

void TerminalBuffer::scrollUpInRegion(int n) {
    if (n <= 0) return;
    const int top = regionTop_;
    const int bot = regionBottom_;
    n = std::min(n, bot - top + 1);

    // Lines leaving the top of the *full screen* region go to scrollback.
    const bool fullRegion = (top == 0) && (bot == active_->rows - 1);
    if (fullRegion) {
        for (int i = 0; i < n; ++i) pushToScrollback(top + i);
    }

    const int cols = active_->cols;
    for (int r = top; r <= bot - n; ++r) {
        std::copy(
            active_->cells.begin() + (r + n) * cols,
            active_->cells.begin() + (r + n + 1) * cols,
            active_->cells.begin() + r * cols);
    }
    for (int r = bot - n + 1; r <= bot; ++r) {
        clearRow(r, 0, cols - 1, lastPen_);
    }
    // Live viewport should snap to bottom on new output so the user sees it.
    scrollOffset_ = 0;
}

void TerminalBuffer::scrollDownInRegion(int n) {
    if (n <= 0) return;
    const int top = regionTop_;
    const int bot = regionBottom_;
    n = std::min(n, bot - top + 1);
    const int cols = active_->cols;
    for (int r = bot; r >= top + n; --r) {
        std::copy(
            active_->cells.begin() + (r - n) * cols,
            active_->cells.begin() + (r - n + 1) * cols,
            active_->cells.begin() + r * cols);
    }
    for (int r = top; r < top + n; ++r) {
        clearRow(r, 0, cols - 1, lastPen_);
    }
}

void TerminalBuffer::clearRow(int r, int colStart, int colEnd, const PenState& pen) {
    if (r < 0 || r >= active_->rows) return;
    Cell blank;
    blank.cp = 0x20;
    blank.fg = pen.fg;
    blank.bg = pen.bg;
    blank.attrs = 0;
    for (int c = colStart; c <= colEnd && c < active_->cols; ++c) {
        active_->at(r, c) = blank;
    }
}

void TerminalBuffer::putChar(char32_t cp, const PenState& pen) {
    lastPen_ = pen;

    // Strip C0 — handled elsewhere — and combining marks (we don't compose).
    if (cp < 0x20) return;
    if ((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE20 && cp <= 0xFE2F)) {
        return; // ignore combining marks
    }

    const int cols = active_->cols;
    const bool wide = isWide(cp);

    // Deferred wrap from previous emit at last column.
    if (wrapPending_) {
        curCol_ = 0;
        if (curRow_ == regionBottom_) scrollUpInRegion(1);
        else if (curRow_ < active_->rows - 1) ++curRow_;
        wrapPending_ = false;
    }

    // Wide char that wouldn't fit — wrap first.
    if (wide && curCol_ >= cols - 1) {
        curCol_ = 0;
        if (curRow_ == regionBottom_) scrollUpInRegion(1);
        else if (curRow_ < active_->rows - 1) ++curRow_;
        wrapPending_ = false;
    }

    Cell& cell = active_->at(curRow_, curCol_);
    cell.cp = cp;
    cell.fg = pen.fg;
    cell.bg = pen.bg;
    cell.attrs = pen.attrs;

    if (wide && curCol_ + 1 < cols) {
        Cell& tr = active_->at(curRow_, curCol_ + 1);
        tr.cp = kWideTrailer;
        tr.fg = pen.fg;
        tr.bg = pen.bg;
        tr.attrs = pen.attrs;
        curCol_ += 2;
    } else {
        ++curCol_;
    }

    if (curCol_ >= cols) {
        // Defer the wrap until next char; this matches xterm DECAWM so apps
        // can re-issue a CR/LF without producing a blank line.
        curCol_ = cols - 1;
        wrapPending_ = true;
    }
}

void TerminalBuffer::lineFeed() {
    if (curRow_ == regionBottom_) {
        scrollUpInRegion(1);
    } else if (curRow_ < active_->rows - 1) {
        ++curRow_;
    }
    wrapPending_ = false;
}

void TerminalBuffer::carriageReturn() {
    curCol_ = 0;
    wrapPending_ = false;
}

void TerminalBuffer::backspace() {
    if (wrapPending_) { wrapPending_ = false; return; }
    if (curCol_ > 0) --curCol_;
}

void TerminalBuffer::tab() {
    const int next = ((curCol_ / 8) + 1) * 8;
    curCol_ = std::min(next, active_->cols - 1);
    wrapPending_ = false;
}

void TerminalBuffer::cursorUp(int n)    { curRow_ = std::max(0, curRow_ - n); wrapPending_ = false; }
void TerminalBuffer::cursorDown(int n)  { curRow_ = std::min(active_->rows - 1, curRow_ + n); wrapPending_ = false; }
void TerminalBuffer::cursorForward(int n){ curCol_ = std::min(active_->cols - 1, curCol_ + n); wrapPending_ = false; }
void TerminalBuffer::cursorBack(int n)   { curCol_ = std::max(0, curCol_ - n); wrapPending_ = false; }
void TerminalBuffer::cursorColumn(int col){ curCol_ = std::clamp(col, 0, active_->cols - 1); wrapPending_ = false; }

void TerminalBuffer::cursorPos(int row, int col) {
    if (row >= 0) curRow_ = std::clamp(row, 0, active_->rows - 1);
    if (col >= 0) curCol_ = std::clamp(col, 0, active_->cols - 1);
    wrapPending_ = false;
}

void TerminalBuffer::saveCursor() {
    savedRow_ = curRow_;
    savedCol_ = curCol_;
}

void TerminalBuffer::restoreCursor() {
    curRow_ = std::clamp(savedRow_, 0, active_->rows - 1);
    curCol_ = std::clamp(savedCol_, 0, active_->cols - 1);
    wrapPending_ = false;
}

void TerminalBuffer::eraseInLine(int mode) {
    const int cols = active_->cols;
    switch (mode) {
        case 0: clearRow(curRow_, curCol_, cols - 1, lastPen_); break;
        case 1: clearRow(curRow_, 0, curCol_, lastPen_); break;
        case 2: clearRow(curRow_, 0, cols - 1, lastPen_); break;
        default: break;
    }
    wrapPending_ = false;
}

void TerminalBuffer::eraseInDisplay(int mode) {
    const int cols = active_->cols;
    const int rows = active_->rows;
    switch (mode) {
        case 0:
            clearRow(curRow_, curCol_, cols - 1, lastPen_);
            for (int r = curRow_ + 1; r < rows; ++r) clearRow(r, 0, cols - 1, lastPen_);
            break;
        case 1:
            for (int r = 0; r < curRow_; ++r) clearRow(r, 0, cols - 1, lastPen_);
            clearRow(curRow_, 0, curCol_, lastPen_);
            break;
        case 2:
            for (int r = 0; r < rows; ++r) clearRow(r, 0, cols - 1, lastPen_);
            break;
        case 3:
            scrollback_.clear();
            scrollOffset_ = 0;
            break;
        default: break;
    }
    wrapPending_ = false;
}

void TerminalBuffer::setAltScreen(bool on) {
    if (on && active_ == &alt_) return;
    if (!on && active_ == &primary_) return;
    active_ = on ? &alt_ : &primary_;
    if (on) {
        // Clear alt on entry — vim/less expect a fresh screen.
        for (auto& c : alt_.cells) c = Cell{};
    }
    savedRow_ = 0; savedCol_ = 0;
    curRow_ = 0; curCol_ = 0;
    regionTop_ = 0;
    regionBottom_ = active_->rows - 1;
    wrapPending_ = false;
    scrollOffset_ = 0;
}

void TerminalBuffer::setScrollRegion(int top, int bottom) {
    if (top < 0) top = 0;
    if (bottom < 0 || bottom >= active_->rows) bottom = active_->rows - 1;
    if (top >= bottom) { top = 0; bottom = active_->rows - 1; }
    regionTop_ = top;
    regionBottom_ = bottom;
    curRow_ = 0; curCol_ = 0;
    wrapPending_ = false;
}

void TerminalBuffer::insertLines(int n) {
    if (curRow_ < regionTop_ || curRow_ > regionBottom_) return;
    const int avail = regionBottom_ - curRow_ + 1;
    n = std::min(n, avail);
    const int cols = active_->cols;
    for (int r = regionBottom_; r >= curRow_ + n; --r) {
        std::copy(
            active_->cells.begin() + (r - n) * cols,
            active_->cells.begin() + (r - n + 1) * cols,
            active_->cells.begin() + r * cols);
    }
    for (int r = curRow_; r < curRow_ + n; ++r) clearRow(r, 0, cols - 1, lastPen_);
}

void TerminalBuffer::deleteLines(int n) {
    if (curRow_ < regionTop_ || curRow_ > regionBottom_) return;
    const int avail = regionBottom_ - curRow_ + 1;
    n = std::min(n, avail);
    const int cols = active_->cols;
    for (int r = curRow_; r <= regionBottom_ - n; ++r) {
        std::copy(
            active_->cells.begin() + (r + n) * cols,
            active_->cells.begin() + (r + n + 1) * cols,
            active_->cells.begin() + r * cols);
    }
    for (int r = regionBottom_ - n + 1; r <= regionBottom_; ++r) clearRow(r, 0, cols - 1, lastPen_);
}

void TerminalBuffer::deleteChars(int n) {
    const int cols = active_->cols;
    n = std::min(n, cols - curCol_);
    for (int c = curCol_; c < cols - n; ++c) {
        active_->at(curRow_, c) = active_->at(curRow_, c + n);
    }
    for (int c = cols - n; c < cols; ++c) {
        Cell blank;
        blank.fg = lastPen_.fg;
        blank.bg = lastPen_.bg;
        active_->at(curRow_, c) = blank;
    }
}

void TerminalBuffer::reverseIndex() {
    if (curRow_ == regionTop_) scrollDownInRegion(1);
    else if (curRow_ > 0) --curRow_;
    wrapPending_ = false;
}

} // namespace dante::terminal
