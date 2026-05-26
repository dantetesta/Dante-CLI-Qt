// VT500-series state machine (Paul Williams) — minimal subset for the apps
// a Windows dev uses 90% of the time (pwsh prompt, oh-my-posh, ls --color,
// git log graph, npm/cargo). NOT 100% xterm conformant; explicit TODOs:
//   - mouse modes (X10/SGR-mouse, focus-in/out)
//   - alt-screen full conformance (we toggle a flag and let the buffer choose)
//   - DEC line drawing G0/G1 charset switching
//   - sixel / iTerm2 images
//   - scrollback search / hyperlinks (OSC 8)
//
// Designed to consume raw bytes (not QString) so UTF-8 multibyte sequences
// pass through without being mangled by Qt's string conversions. A tiny
// embedded UTF-8 decoder produces QChar code points that we forward to
// `printChar(cp, attrs)`.
//
// Callback-driven (no signals) — VTParser lives inside TerminalView's paint
// path and we want hot-loop dispatch with zero meta-call overhead. Wire up
// with `parser.setHandler(...)` before calling `feed()`.

#pragma once

#include <QByteArray>
#include <QChar>
#include <QString>
#include <cstdint>
#include <functional>
#include <vector>

namespace dante::terminal {

/// Cell rendering attributes (bitfield in TerminalBuffer::Cell::attrs).
namespace CellAttr {
    constexpr quint8 Bold      = 1 << 0;
    constexpr quint8 Italic    = 1 << 1;
    constexpr quint8 Underline = 1 << 2;
    constexpr quint8 Inverse   = 1 << 3;
    constexpr quint8 Faint     = 1 << 4;
}

/// Current SGR state — packed into each printed cell.
struct PenState {
    quint32 fg{0xFFE6E6EB}; // matches Theme.fg
    quint32 bg{0xFF0E0E12}; // matches Theme.ink
    quint8  attrs{0};
    bool    fgDefault{true};
    bool    bgDefault{true};
};

/// What the parser tells the buffer to do. Pure interface — VTParser invokes
/// these in feed(); the consumer (TerminalView) routes them into the buffer
/// and emits Qt signals only when something genuinely UI-visible happens.
class VTHandler {
public:
    virtual ~VTHandler() = default;

    // Glyph output. `cp` is a Unicode code point (UTF-32) — the parser folds
    // surrogates into a single call.
    virtual void printChar(char32_t cp, const PenState& pen) = 0;

    // Control bytes that wrap rather than escape into CSI.
    virtual void lineFeed()         = 0;   // LF / VT / FF
    virtual void carriageReturn()   = 0;   // CR
    virtual void backspace()        = 0;   // BS
    virtual void tab()              = 0;   // HT
    virtual void bell()             = 0;   // BEL (non-OSC)

    // CSI cursor movement. Coordinates are 1-based in the wire protocol but
    // we normalize to 0-based here.
    virtual void cursorUp(int n)    = 0;
    virtual void cursorDown(int n)  = 0;
    virtual void cursorForward(int n) = 0;
    virtual void cursorBack(int n)    = 0;
    virtual void cursorPos(int row0, int col0) = 0; // 0-based
    virtual void cursorColumn(int col0)        = 0; // CSI G — 0-based
    virtual void saveCursor()       = 0;
    virtual void restoreCursor()    = 0;

    // Erase. mode follows ECMA-48: 0=from cursor, 1=to cursor, 2=all, 3=scrollback.
    virtual void eraseInLine(int mode)    = 0;
    virtual void eraseInDisplay(int mode) = 0;

    // Modes.
    virtual void setCursorVisible(bool on) = 0;
    virtual void setAltScreen(bool on)     = 0;
    virtual void setBracketedPaste(bool on)= 0;

    /// SPEC-031 — DEC mouse tracking modes.
    ///   1000 = X10 (button press only)
    ///   1002 = button-event (press + drag)
    ///   1003 = any-event (press + drag + bare move)
    ///   1006 = SGR encoding (decimal, button + col + row + M/m)
    /// When `on=true` the handler turns the mode on; off when false.
    /// Default impl is a no-op so existing handlers don't need to override.
    virtual void setMouseTracking(int mode, bool on) { Q_UNUSED(mode) Q_UNUSED(on) }

    // OSC events — strings only, parser handles framing (BEL or ST).
    virtual void osc(int code, const QString& payload) = 0;

    // Scroll region (CSI r). 0-based, inclusive.
    virtual void setScrollRegion(int top0, int bottom0) = 0;

    // Insert/delete lines.
    virtual void insertLines(int n) = 0;
    virtual void deleteLines(int n) = 0;
    virtual void deleteChars(int n) = 0;

    // Reverse Index — ESC M. Moves cursor up one row, scrolling at top.
    virtual void reverseIndex() = 0;

    // Cursor shape (DECSCUSR — CSI Ps SP q). 0=default, 1=block-blink,
    // 2=block-steady, 3=underline-blink, 4=underline-steady, 5=bar-blink,
    // 6=bar-steady. Optional; default impl is a no-op.
    virtual void setCursorStyle(int /*shape*/) {}
};

class VTParser {
public:
    VTParser();

    void setHandler(VTHandler* h) { handler_ = h; }
    PenState& pen() { return pen_; }
    const PenState& pen() const { return pen_; }

    /// Feed raw bytes from the PTY. Idempotent on partial input — the state
    /// machine remembers where it was.
    void feed(const QByteArray& bytes);
    void feed(const char* data, int len);

private:
    enum State : quint8 {
        SGround = 0,
        SEscape,
        SCsiEntry,
        SCsiParam,
        SCsiIntermediate,
        SCsiIgnore,
        SOscString,
        SDcsIgnore,    // we ignore DCS bodies entirely
        SSosPmApcString,
        SCharSet,      // ESC ( / ESC )  + one char
    };

    void doPrint(char32_t cp);
    void doExecute(quint8 b);
    void csiDispatch(quint8 finalByte);
    void escDispatch(quint8 finalByte);
    void oscDispatch();
    void clearCsi();

    // SGR helper.
    void applySgr();

    // UTF-8 decoder feeding code points to doPrint.
    void utf8Feed(quint8 b);

    VTHandler* handler_{nullptr};
    State      state_{SGround};

    // CSI accumulator.
    std::vector<int> params_;
    int              currentParam_{-1};   // -1 == empty (defaults to 0 / 1)
    QByteArray       intermediates_;
    bool             privateMarker_{false}; // CSI ? ...
    QByteArray       oscBuffer_;
    bool             escIntermediateConsumed_{false};
    quint8           pendingEsc_{0};

    // UTF-8 decoder state.
    quint32 utf8Acc_{0};
    int     utf8Need_{0};

    PenState pen_;

    // Saved cursor for ESC 7 / DECSC — actual position lives in the buffer,
    // but the pen is parser-side.
    PenState savedPen_;
    bool     hasSavedPen_{false};
};

} // namespace dante::terminal
