// VT100/ANSI terminal widget — a QQuickPaintedItem that owns a VTParser +
// TerminalBuffer and renders the grid with QPainter. Hot path uses run-length
// batching: glyphs that share fg/bg/attrs are drawn in one drawText call per
// run, dramatically cheaper than per-cell.
//
// QML wires this in instead of the old TextArea. From the QML side:
//   TerminalView { sessionId: "tab-123"; fontFamily: Theme.fontMono; ... }
//
// The view calls `terminal.write(sessionId, payload)` for keyboard input —
// `terminal` is the TerminalBridge context property registered in main.cpp.

#pragma once

#include "terminal/TerminalBuffer.h"
#include "terminal/VTParser.h"

#include <QQuickPaintedItem>
#include <QFont>
#include <QFontMetricsF>
#include <QColor>
#include <QString>
#include <QTimer>
#include <memory>

namespace dante {

class TerminalView : public QQuickPaintedItem, public terminal::VTHandler {
    Q_OBJECT
    Q_PROPERTY(int     cols          READ cols          NOTIFY gridChanged)
    Q_PROPERTY(int     rows          READ rows          NOTIFY gridChanged)
    Q_PROPERTY(QString sessionId     READ sessionId     WRITE setSessionId   NOTIFY sessionIdChanged)
    Q_PROPERTY(QString fontFamily    READ fontFamily    WRITE setFontFamily  NOTIFY fontChanged)
    Q_PROPERTY(int     fontPixelSize READ fontPixelSize WRITE setFontPixelSize NOTIFY fontChanged)
    Q_PROPERTY(QColor  backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY paletteChanged)
    Q_PROPERTY(QColor  foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY paletteChanged)
public:
    explicit TerminalView(QQuickItem* parent = nullptr);
    ~TerminalView() override;

    int cols() const { return buffer_.cols(); }
    int rows() const { return buffer_.rows(); }
    QString sessionId() const { return sessionId_; }
    void setSessionId(const QString& s);
    QString fontFamily() const { return font_.family(); }
    void setFontFamily(const QString& f);
    int fontPixelSize() const { return font_.pixelSize(); }
    void setFontPixelSize(int s);
    QColor backgroundColor() const { return bgColor_; }
    void setBackgroundColor(const QColor& c);
    QColor foregroundColor() const { return fgColor_; }
    void setForegroundColor(const QColor& c);

    // Called from QML when TerminalBridge emits outputReceived for our id.
    Q_INVOKABLE void feed(const QString& chunk);
    Q_INVOKABLE void feedBytes(const QByteArray& chunk);
    Q_INVOKABLE void clear();

    // Send a key event back to the PTY. QML forwards Keys.onPressed here.
    Q_INVOKABLE void sendKey(int qtKey, int modifiers, const QString& text);

    // Scroll history (mouse wheel from QML, PgUp/PgDn keys).
    Q_INVOKABLE void scrollByLines(int delta);
    Q_INVOKABLE void scrollToBottom();

    void paint(QPainter* painter) override;

signals:
    void gridChanged();
    void sessionIdChanged();
    void fontChanged();
    void paletteChanged();
    void writeRequested(const QString& sessionId, const QByteArray& bytes);
    void oscEvent(int code, const QString& payload);
    void resizeRequested(const QString& sessionId, int cols, int rows);

protected:
    void geometryChange(const QRectF& newGeo, const QRectF& oldGeo) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // VTHandler implementation — these are hot, keep them inline-friendly.
    void printChar(char32_t cp, const terminal::PenState& pen) override;
    void lineFeed() override        { buffer_.lineFeed(); schedule(); }
    void carriageReturn() override  { buffer_.carriageReturn(); schedule(); }
    void backspace() override       { buffer_.backspace(); schedule(); }
    void tab() override             { buffer_.tab(); schedule(); }
    void bell() override            {}
    void cursorUp(int n) override   { buffer_.cursorUp(n); schedule(); }
    void cursorDown(int n) override { buffer_.cursorDown(n); schedule(); }
    void cursorForward(int n) override { buffer_.cursorForward(n); schedule(); }
    void cursorBack(int n) override { buffer_.cursorBack(n); schedule(); }
    void cursorPos(int r, int c) override { buffer_.cursorPos(r, c); schedule(); }
    void cursorColumn(int c) override { buffer_.cursorColumn(c); schedule(); }
    void saveCursor() override      { buffer_.saveCursor(); }
    void restoreCursor() override   { buffer_.restoreCursor(); schedule(); }
    void eraseInLine(int m) override { buffer_.eraseInLine(m); schedule(); }
    void eraseInDisplay(int m) override { buffer_.eraseInDisplay(m); schedule(); }
    void setCursorVisible(bool v) override { buffer_.setCursorVisible(v); schedule(); }
    void setAltScreen(bool v) override { buffer_.setAltScreen(v); schedule(); }
    void setBracketedPaste(bool v) override { bracketedPaste_ = v; }
    void osc(int code, const QString& payload) override { emit oscEvent(code, payload); }
    void setScrollRegion(int t, int b) override { buffer_.setScrollRegion(t, b); }
    void insertLines(int n) override { buffer_.insertLines(n); schedule(); }
    void deleteLines(int n) override { buffer_.deleteLines(n); schedule(); }
    void deleteChars(int n) override { buffer_.deleteChars(n); schedule(); }
    void reverseIndex() override { buffer_.reverseIndex(); schedule(); }
    void setCursorStyle(int s) override { buffer_.setCursorStyle(s); schedule(); }

private:
    void recomputeMetrics();
    void recomputeGrid();
    void schedule();                 // coalesce paints — 1 frame at 60Hz max
    QByteArray encodeKey(int qtKey, int modifiers, const QString& text) const;
    QColor toQColor(quint32 argb) const {
        return QColor::fromRgb(
            (argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF,
            (argb >> 24) & 0xFF);
    }

    terminal::TerminalBuffer buffer_;
    terminal::VTParser       parser_;

    QString sessionId_;
    QFont   font_;
    QFontMetricsF metrics_{QFont()};
    qreal   cellW_{8.0};
    qreal   cellH_{16.0};
    qreal   baseline_{12.0};
    QColor  bgColor_{14, 14, 18};
    QColor  fgColor_{230, 230, 235};
    bool    focused_{false};
    bool    bracketedPaste_{false};
    bool    repaintScheduled_{false};
    QTimer  repaintTimer_;
};

} // namespace dante
