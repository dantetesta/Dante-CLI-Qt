#include "TerminalView.h"

#include <QPainter>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QClipboard>
#include <QTextStream>
#include <QDebug>

namespace dante {

TerminalView::TerminalView(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFocus(true);
    setActiveFocusOnTab(true);

    // Reasonable default font; QML overrides via setFontFamily.
    font_ = QFont("Menlo", 13);
    font_.setStyleHint(QFont::TypeWriter);
    font_.setFixedPitch(true);
    recomputeMetrics();

    parser_.setHandler(this);

    // Coalesce paints — when the PTY dumps a large chunk, we get many handler
    // callbacks; we only need one update() per frame.
    repaintTimer_.setSingleShot(true);
    repaintTimer_.setInterval(16); // ~60Hz cap
    connect(&repaintTimer_, &QTimer::timeout, this, [this] {
        repaintScheduled_ = false;
        update();
    });
}

TerminalView::~TerminalView() = default;

void TerminalView::setSessionId(const QString& s) {
    if (sessionId_ == s) return;
    sessionId_ = s;
    emit sessionIdChanged();
}

void TerminalView::setFontFamily(const QString& f) {
    if (font_.family() == f) return;
    font_.setFamily(f);
    font_.setFixedPitch(true);
    font_.setStyleHint(QFont::TypeWriter);
    recomputeMetrics();
    recomputeGrid();
    emit fontChanged();
    update();
}

void TerminalView::setFontPixelSize(int s) {
    if (s <= 0 || font_.pixelSize() == s) return;
    font_.setPixelSize(s);
    recomputeMetrics();
    recomputeGrid();
    emit fontChanged();
    update();
}

void TerminalView::setBackgroundColor(const QColor& c) {
    if (bgColor_ == c) return;
    bgColor_ = c;
    emit paletteChanged();
    update();
}

void TerminalView::setForegroundColor(const QColor& c) {
    if (fgColor_ == c) return;
    fgColor_ = c;
    emit paletteChanged();
    update();
}

void TerminalView::recomputeMetrics() {
    metrics_ = QFontMetricsF(font_);
    // Use horizontalAdvance for a representative monospace glyph; ascent +
    // descent for the row height. Round up to whole pixels so glyphs don't
    // jitter across cells.
    cellW_ = std::max(1.0, std::ceil(metrics_.horizontalAdvance(QLatin1Char('M'))));
    cellH_ = std::max(1.0, std::ceil(metrics_.ascent() + metrics_.descent() + 2.0));
    baseline_ = metrics_.ascent() + 1.0;
}

void TerminalView::recomputeGrid() {
    if (cellW_ <= 0 || cellH_ <= 0) return;
    const int newCols = std::max(1, int(width()  / cellW_));
    const int newRows = std::max(1, int(height() / cellH_));
    if (newCols == buffer_.cols() && newRows == buffer_.rows()) return;
    buffer_.resize(newCols, newRows);
    emit gridChanged();
    if (!sessionId_.isEmpty()) emit resizeRequested(sessionId_, newCols, newRows);
}

void TerminalView::geometryChange(const QRectF& newGeo, const QRectF& oldGeo) {
    QQuickPaintedItem::geometryChange(newGeo, oldGeo);
    recomputeGrid();
    update();
}

void TerminalView::feed(const QString& chunk) {
    feedBytes(chunk.toUtf8());
}

void TerminalView::feedBytes(const QByteArray& chunk) {
    parser_.feed(chunk);
    schedule();
}

void TerminalView::clear() {
    buffer_.eraseInDisplay(2);
    buffer_.cursorPos(0, 0);
    update();
}

void TerminalView::printChar(char32_t cp, const terminal::PenState& pen) {
    buffer_.putChar(cp, pen);
    schedule();
}

void TerminalView::schedule() {
    if (repaintScheduled_) return;
    repaintScheduled_ = true;
    repaintTimer_.start();
}

// ---- Painting -------------------------------------------------------------

void TerminalView::paint(QPainter* painter) {
    painter->fillRect(boundingRect(), bgColor_);
    painter->setFont(font_);

    const int cols = buffer_.cols();
    const int rows = buffer_.rows();
    if (cols <= 0 || rows <= 0) return;

    // Origin row in the virtual coordinate space:
    //   live view  → 0
    //   scrolled N → -N (we shift the active screen down, showing scrollback
    //   at top)
    const int scrollOff = buffer_.scrollOffset();
    const int firstVy = -scrollOff;

    // Pre-allocate; reused across rows.
    QString runText;
    runText.reserve(cols);

    for (int rRow = 0; rRow < rows; ++rRow) {
        const int vy = firstVy + rRow;
        const terminal::Cell* line = buffer_.lineCells(vy);
        if (!line) continue;

        const qreal y = rRow * cellH_;

        // First pass: paint contiguous background runs to avoid AA seams.
        int runStart = 0;
        quint32 runBg = line[0].bg;
        for (int c = 1; c <= cols; ++c) {
            const quint32 bg = (c < cols) ? line[c].bg : 0;
            if (c == cols || bg != runBg) {
                if (runBg != 0xFF0E0E12 && runBg != 0) {
                    painter->fillRect(
                        QRectF(runStart * cellW_, y, (c - runStart) * cellW_, cellH_),
                        toQColor(runBg));
                }
                runStart = c;
                runBg = bg;
            }
        }

        // Second pass: foreground runs sharing fg + attrs.
        int idx = 0;
        while (idx < cols) {
            // Skip wide trailers and blank-space-with-default-fg fast path.
            if (line[idx].cp == terminal::kWideTrailer) { ++idx; continue; }

            quint32 runFg = line[idx].fg;
            quint8  runAttrs = line[idx].attrs;
            int     start = idx;

            // Collect run.
            runText.clear();
            while (idx < cols) {
                const auto& cell = line[idx];
                if (cell.cp == terminal::kWideTrailer) { ++idx; continue; }
                if (idx != start && (cell.fg != runFg || cell.attrs != runAttrs)) break;
                if (cell.cp >= 0x20) {
                    if (cell.cp < 0x10000) {
                        runText.append(QChar(quint16(cell.cp)));
                    } else {
                        // Surrogate pair.
                        const quint32 v = cell.cp - 0x10000;
                        runText.append(QChar(quint16(0xD800 + (v >> 10))));
                        runText.append(QChar(quint16(0xDC00 + (v & 0x3FF))));
                    }
                } else {
                    runText.append(QLatin1Char(' '));
                }
                ++idx;
            }
            if (runText.isEmpty()) continue;

            const bool inv = runAttrs & terminal::CellAttr::Inverse;
            QColor fg = toQColor(runFg);
            if (inv) {
                // Quick invert: swap with the dominant bg in this run.
                fg = toQColor(line[start].bg);
                painter->fillRect(
                    QRectF(start * cellW_, y, (idx - start) * cellW_, cellH_),
                    toQColor(runFg));
            }
            if (runAttrs & terminal::CellAttr::Faint) {
                fg.setAlphaF(fg.alphaF() * 0.55);
            }

            QFont f = font_;
            if (runAttrs & terminal::CellAttr::Bold)   f.setBold(true);
            if (runAttrs & terminal::CellAttr::Italic) f.setItalic(true);
            painter->setFont(f);

            painter->setPen(fg);
            painter->drawText(
                QPointF(start * cellW_, y + baseline_),
                runText);

            if (runAttrs & terminal::CellAttr::Underline) {
                const qreal uy = y + baseline_ + 1.5;
                painter->drawLine(
                    QPointF(start * cellW_, uy),
                    QPointF(idx   * cellW_, uy));
            }
        }
    }

    // Cursor — only render on the live screen (scrollOff == 0).
    if (buffer_.cursorVisible() && scrollOff == 0) {
        const qreal cx = buffer_.curCol() * cellW_;
        const qreal cy = buffer_.curRow() * cellH_;
        QRectF rect(cx, cy, cellW_, cellH_);
        const int style = buffer_.cursorStyle();
        const bool barShape = (style == 5 || style == 6);
        const bool underline = (style == 3 || style == 4);
        if (!focused_) {
            painter->setPen(fgColor_);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));
        } else if (barShape) {
            painter->fillRect(QRectF(cx, cy, std::max(1.5, cellW_ * 0.15), cellH_), fgColor_);
        } else if (underline) {
            painter->fillRect(QRectF(cx, cy + cellH_ - 2, cellW_, 2), fgColor_);
        } else {
            painter->fillRect(rect, fgColor_);
            // Re-draw the glyph beneath the block cursor in inverted color so
            // it stays legible.
            const terminal::Cell& cell = buffer_.cellAt(buffer_.curRow(), buffer_.curCol());
            if (cell.cp >= 0x20 && cell.cp != terminal::kWideTrailer) {
                painter->setPen(bgColor_);
                QString s;
                if (cell.cp < 0x10000) s.append(QChar(quint16(cell.cp)));
                else {
                    const quint32 v = cell.cp - 0x10000;
                    s.append(QChar(quint16(0xD800 + (v >> 10))));
                    s.append(QChar(quint16(0xDC00 + (v & 0x3FF))));
                }
                painter->setFont(font_);
                painter->drawText(QPointF(cx, cy + baseline_), s);
            }
        }
    }
}

// ---- Input ----------------------------------------------------------------

void TerminalView::focusInEvent(QFocusEvent*)  { focused_ = true; update(); }
void TerminalView::focusOutEvent(QFocusEvent*) { focused_ = false; update(); }

void TerminalView::mousePressEvent(QMouseEvent* event) {
    forceActiveFocus();
    if (event->button() == Qt::MiddleButton) {
        // X11-style middle-click paste.
        const QString sel = QGuiApplication::clipboard()->text(QClipboard::Selection);
        if (!sel.isEmpty()) {
            const QByteArray bytes = sel.toUtf8();
            emit writeRequested(sessionId_, bytes);
        }
        event->accept();
        return;
    }
    if (mouseEventMode_ != 0) {
        reportMouse(int(event->button()), event->modifiers(), event->position(), 'p');
    }
    event->accept();
}

void TerminalView::mouseReleaseEvent(QMouseEvent* event) {
    if (mouseEventMode_ != 0) {
        reportMouse(int(event->button()), event->modifiers(), event->position(), 'r');
    }
    event->accept();
}

void TerminalView::mouseMoveEvent(QMouseEvent* event) {
    // 1002 = button-event tracking — only while a button is held.
    if (mouseEventMode_ == 1002 && event->buttons() != Qt::NoButton) {
        reportMouse(int(event->buttons() & ~Qt::NoButton),
                    event->modifiers(), event->position(), 'm');
    }
    event->accept();
}

void TerminalView::hoverMoveEvent(QHoverEvent* event) {
    // 1003 = any-event — also reports bare moves (no button held).
    if (mouseEventMode_ == 1003) {
        reportMouse(int(Qt::NoButton), event->modifiers(),
                    event->position(), 'm');
    }
    event->accept();
}

void TerminalView::setMouseTracking(int mode, bool on) {
    if (mode == 1006) {
        mouseSGR_ = on;
        return;
    }
    if (on) {
        mouseEventMode_ = mode;
        setAcceptHoverEvents(mode == 1003);  // only enable hover for any-event
        setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    } else if (mouseEventMode_ == mode) {
        mouseEventMode_ = 0;
        setAcceptHoverEvents(false);
    }
}

void TerminalView::reportMouse(int qtButton, Qt::KeyboardModifiers mods,
                               QPointF localPos, char kind) {
    if (mouseEventMode_ == 0) return;
    // Cell coordinates (1-based, clamped).
    int col = int(localPos.x() / cellW_) + 1;
    int row = int(localPos.y() / cellH_) + 1;
    if (col < 1) col = 1; if (row < 1) row = 1;
    // Button code (per xterm convention):
    //   0=left, 1=middle, 2=right, 3=release(legacy), 32+=motion, 64+=wheel
    int cb;
    switch (qtButton) {
        case Qt::LeftButton:   cb = 0; break;
        case Qt::MiddleButton: cb = 1; break;
        case Qt::RightButton:  cb = 2; break;
        default:               cb = 3; break;   // no button (motion-only)
    }
    if (kind == 'm') cb |= 32;
    if (mods & Qt::ShiftModifier)   cb |= 4;
    if (mods & Qt::AltModifier)     cb |= 8;
    if (mods & Qt::ControlModifier) cb |= 16;

    QByteArray seq;
    if (mouseSGR_) {
        const char trailer = (kind == 'r') ? 'm' : 'M';
        seq = QStringLiteral("\x1b[<%1;%2;%3%4")
                  .arg(cb).arg(col).arg(row).arg(QChar(trailer)).toLatin1();
    } else {
        // Legacy X10/1000 encoding — each byte is value + 32.
        // For releases, button code becomes 3 (legacy).
        int legacyCb = (kind == 'r') ? 3 : cb;
        const int max = 223 - 32;
        if (col > max + 1) col = max + 1;
        if (row > max + 1) row = max + 1;
        seq.reserve(6);
        seq.append("\x1b[M");
        seq.append(char(legacyCb + 32));
        seq.append(char(col + 32));
        seq.append(char(row + 32));
    }
    emit writeRequested(sessionId_, seq);
}

void TerminalView::wheelEvent(QWheelEvent* event) {
    const int linesPerNotch = 3;
    const int delta = event->angleDelta().y();
    if (delta == 0) { event->ignore(); return; }
    // Scroll up = show older. Wheel up = positive delta in Qt.
    buffer_.scrollViewBy(delta > 0 ? linesPerNotch : -linesPerNotch);
    update();
    event->accept();
}

void TerminalView::scrollByLines(int delta) { buffer_.scrollViewBy(delta); update(); }
void TerminalView::scrollToBottom() { buffer_.scrollToBottom(); update(); }

void TerminalView::keyPressEvent(QKeyEvent* event) {
    // PgUp/PgDn with Shift scrolls history; without, send to PTY.
    if ((event->modifiers() & Qt::ShiftModifier) &&
        (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown)) {
        const int n = buffer_.rows() / 2;
        buffer_.scrollViewBy(event->key() == Qt::Key_PageUp ? n : -n);
        update();
        event->accept();
        return;
    }
    // Any keystroke that targets the shell should snap the view back to live.
    if (buffer_.scrollOffset() != 0) {
        buffer_.scrollToBottom();
        update();
    }

    const QByteArray payload = encodeKey(event->key(), event->modifiers(), event->text());
    if (!payload.isEmpty()) {
        emit writeRequested(sessionId_, payload);
        event->accept();
        return;
    }
    QQuickPaintedItem::keyPressEvent(event);
}

QByteArray TerminalView::encodeKey(int qtKey, int modifiers, const QString& text) const {
    const bool ctrl  = modifiers & Qt::ControlModifier;
    const bool alt   = modifiers & Qt::AltModifier;
    const bool shift = modifiers & Qt::ShiftModifier;

    // Special keys first.
    switch (qtKey) {
        case Qt::Key_Return:
        case Qt::Key_Enter:    return "\r";
        case Qt::Key_Backspace: return "\x7f"; // DEL — what bash/zsh expect
        case Qt::Key_Tab:      return shift ? QByteArray("\x1b[Z") : QByteArray("\t");
        case Qt::Key_Escape:   return "\x1b";
        case Qt::Key_Up:       return "\x1b[A";
        case Qt::Key_Down:     return "\x1b[B";
        case Qt::Key_Right:    return "\x1b[C";
        case Qt::Key_Left:     return "\x1b[D";
        case Qt::Key_Home:     return "\x1b[H";
        case Qt::Key_End:      return "\x1b[F";
        case Qt::Key_PageUp:   return "\x1b[5~";
        case Qt::Key_PageDown: return "\x1b[6~";
        case Qt::Key_Insert:   return "\x1b[2~";
        case Qt::Key_Delete:   return "\x1b[3~";
        case Qt::Key_F1:  return "\x1bOP";
        case Qt::Key_F2:  return "\x1bOQ";
        case Qt::Key_F3:  return "\x1bOR";
        case Qt::Key_F4:  return "\x1bOS";
        case Qt::Key_F5:  return "\x1b[15~";
        case Qt::Key_F6:  return "\x1b[17~";
        case Qt::Key_F7:  return "\x1b[18~";
        case Qt::Key_F8:  return "\x1b[19~";
        case Qt::Key_F9:  return "\x1b[20~";
        case Qt::Key_F10: return "\x1b[21~";
        case Qt::Key_F11: return "\x1b[23~";
        case Qt::Key_F12: return "\x1b[24~";
        default: break;
    }

    // Ctrl + letter / common bindings.
    if (ctrl && !alt && text.isEmpty()) {
        if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
            const char c = char(qtKey - Qt::Key_A + 1);
            return QByteArray(1, c);
        }
        if (qtKey == Qt::Key_Space) return QByteArray(1, '\0');
        if (qtKey == Qt::Key_BracketLeft) return "\x1b";   // Ctrl-[
        if (qtKey == Qt::Key_Backslash)   return "\x1c";
        if (qtKey == Qt::Key_BracketRight)return "\x1d";
        if (qtKey == Qt::Key_AsciiCircum) return "\x1e";
        if (qtKey == Qt::Key_Underscore)  return "\x1f";
    }
    if (ctrl && !text.isEmpty()) {
        const QChar c0 = text.at(0).toUpper();
        if (c0 >= QLatin1Char('A') && c0 <= QLatin1Char('Z')) {
            return QByteArray(1, char(c0.toLatin1() - 'A' + 1));
        }
    }

    // Alt = ESC prefix per xterm convention.
    if (alt && !text.isEmpty()) {
        QByteArray out = "\x1b";
        out.append(text.toUtf8());
        return out;
    }

    return text.toUtf8();
}

void TerminalView::sendKey(int qtKey, int modifiers, const QString& text) {
    const QByteArray payload = encodeKey(qtKey, modifiers, text);
    if (!payload.isEmpty()) emit writeRequested(sessionId_, payload);
}

} // namespace dante
