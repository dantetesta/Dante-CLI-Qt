// Grid + scrollback for one TerminalView. NOT a true xterm buffer — alt
// screen is a separate primary/alt pair (we keep two grids); scrollback only
// applies to the primary screen. Cells store a 21-bit code point (enough for
// the full Unicode range) so non-BMP glyphs render correctly.
//
// Memory: cell is 12 bytes (cp 4B + fg 4B + bg 4B + attrs 1B + pad). For an
// 80x24 grid + 50_000 line scrollback that's ~46 MB worst case — acceptable.
// Wide chars (CJK / emoji) take two cells; the second cell has cp == 0xFFFE
// (sentinel) and gets skipped during rendering.

#pragma once

#include "VTParser.h"
#include <QVector>
#include <QString>
#include <cstdint>

namespace dante::terminal {

struct Cell {
    char32_t cp{0x20};        // Unicode code point
    quint32  fg{0xFFE6E6EB};
    quint32  bg{0xFF0E0E12};
    quint8   attrs{0};
    quint8   _pad[3]{};
};

constexpr char32_t kWideTrailer = 0xFFFE;

/// Per-screen grid (primary or alt).
struct Screen {
    QVector<Cell> cells;   // size == rows * cols (flat, row-major)
    int cols{80};
    int rows{24};

    Cell& at(int r, int c) { return cells[r * cols + c]; }
    const Cell& at(int r, int c) const { return cells[r * cols + c]; }
};

class TerminalBuffer {
public:
    TerminalBuffer();

    // Geometry.
    void resize(int cols, int rows);
    int  cols() const { return active_->cols; }
    int  rows() const { return active_->rows; }

    // Cursor (0-based).
    int  curRow() const { return curRow_; }
    int  curCol() const { return curCol_; }
    void setCursor(int row, int col);
    bool cursorVisible() const { return cursorVisible_; }
    void setCursorVisible(bool v) { cursorVisible_ = v; }
    int  cursorStyle() const { return cursorStyle_; }
    void setCursorStyle(int s) { cursorStyle_ = s; }

    // Screen access for renderer. row in [0, rows). For rendering with
    // scrollback offset, use line() which folds scrollback + active screen.
    const Cell& cellAt(int r, int c) const { return active_->at(r, c); }

    // Scrollback access for the renderer's virtual viewport. `vy` runs from
    // -(scrollback().size()) up to rows()-1; negative indices are scrollback.
    int  scrollbackLines() const { return scrollback_.size(); }
    const Cell* lineCells(int vy) const;

    // Primary writing API — called from the handler.
    void putChar(char32_t cp, const PenState& pen);
    void lineFeed();
    void carriageReturn();
    void backspace();
    void tab();
    void cursorUp(int n);
    void cursorDown(int n);
    void cursorForward(int n);
    void cursorBack(int n);
    void cursorColumn(int col);
    void cursorPos(int row, int col); // -1 means keep
    void saveCursor();
    void restoreCursor();
    void eraseInLine(int mode);
    void eraseInDisplay(int mode);
    void setAltScreen(bool on);
    void setScrollRegion(int top, int bottom);
    void insertLines(int n);
    void deleteLines(int n);
    void deleteChars(int n);
    void reverseIndex();

    // Scroll history forward / backward — drives mouse-wheel + PgUp/PgDn.
    int  scrollOffset() const { return scrollOffset_; }
    void setScrollOffset(int o);
    void scrollViewBy(int delta) { setScrollOffset(scrollOffset_ + delta); }
    void scrollToBottom() { scrollOffset_ = 0; }

private:
    void scrollUpInRegion(int n);
    void scrollDownInRegion(int n);
    void clearRow(int r, int colStart, int colEnd, const PenState& pen);
    void pushToScrollback(int row);
    void ensureCursorInBounds();

    Screen primary_;
    Screen alt_;
    Screen* active_{&primary_};

    QVector<QVector<Cell>> scrollback_; // each row = full row width at the time of scroll
    int scrollbackMax_{50000};
    int scrollOffset_{0};               // 0 = bottom (live). Positive = scrolled up.

    int curRow_{0};
    int curCol_{0};
    int savedRow_{0}, savedCol_{0};

    int regionTop_{0};
    int regionBottom_{23};

    bool cursorVisible_{true};
    int  cursorStyle_{0};
    bool wrapPending_{false}; // DECAWM-style — last column emit defers wrap
    PenState lastPen_;
};

} // namespace dante::terminal
