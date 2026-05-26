// One PTY session. On Windows uses ConPTY (via WinAPI in ShellAdapter);
// on macOS/Linux uses POSIX openpty(). Emits OSC 7 (cwd) and OSC 0/1/2
// (title) parsed out of the byte stream so the UI can update tab metadata.
//
// CRITICAL ARCH RULE: market-data/risk threading rules don't apply here
// (we're a terminal app), but the spirit holds — no UI thread blocking,
// reader is on its own QThread, writer is non-blocking.

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>

namespace dante::infra { class ShellAdapter; }

namespace dante::terminal {

class TerminalSession : public QObject {
    Q_OBJECT
public:
    explicit TerminalSession(QString sessionId, QObject* parent = nullptr);
    ~TerminalSession() override;

    /// Start shell. Returns false if spawn failed; check errorString().
    bool start(const QString& shell, const QString& cwd, int cols, int rows);

    /// Resize the PTY.
    void resize(int cols, int rows);

    /// Send bytes to the shell's stdin.
    void write(const QByteArray& bytes);

    /// Terminate the shell and clean up the reader thread.
    void kill();

    QString sessionId() const { return sessionId_; }
    QString errorString() const { return error_; }

signals:
    /// Raw output chunk from the shell (UTF-8). Connected to UI for rendering
    /// and to the OSC parser for cwd/title extraction.
    void output(const QString& chunk);

    /// Shell process ended.
    void exited(int code);

    /// Parsed OSC 7 — current working directory changed.
    void cwdChanged(const QString& path);

    /// Parsed OSC 0/1/2 — window/icon title.
    void titleChanged(const QString& title);

private:
    void feedOsc(const QString& chunk);

    QString sessionId_;
    QString error_;
    std::unique_ptr<infra::ShellAdapter> shell_;
    QByteArray oscBuffer_;
    bool inOsc_{false};
};

} // namespace dante::terminal
