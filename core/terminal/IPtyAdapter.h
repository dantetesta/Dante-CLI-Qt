// Abstract PTY adapter interface.
//
// Concrete implementations:
//   - `infra::ConPtyAdapter` (Windows, ConPTY: CreatePseudoConsole)
//   - `infra::UnixPtyAdapter` (macOS/Linux, forkpty)
//
// All implementations MUST:
//   - run their byte-reader on a dedicated QThread (UI must not block)
//   - call the OS resize syscall on `resize()` (no no-ops)
//   - emit `outputReceived` via Qt::QueuedConnection (cross-thread)
//   - sanitize the shell path / cwd of stray NUL bytes before passing to the OS
//
// Signals are deliberately UTF-8 QString chunks because the upstream
// `TerminalSession` already parses OSC sequences as text.

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

namespace dante::terminal {

class IPtyAdapter : public QObject {
    Q_OBJECT
public:
    explicit IPtyAdapter(QObject* parent = nullptr) : QObject(parent) {}
    ~IPtyAdapter() override = default;

    /// Spawn the shell with an initial PTY size. Returns false on failure;
    /// inspect `errorString()` for details. cols/rows must be >= 1.
    virtual bool start(const QString& shell, const QString& cwd, int cols, int rows) = 0;

    /// Propagate a new window size to the kernel (TIOCSWINSZ / ResizePseudoConsole).
    /// MUST issue the actual syscall — silently dropping the resize will break
    /// full-screen apps (vim, htop, fzf).
    virtual void resize(int cols, int rows) = 0;

    /// Write bytes to the shell's stdin (PTY master).
    virtual void write(const QByteArray& bytes) = 0;

    /// Terminate the child and tear down the reader thread.
    virtual void kill() = 0;

    /// Last error from `start()`. Empty if not in error state.
    virtual QString errorString() const = 0;

signals:
    /// UTF-8 chunk read from the PTY master. Emitted on the reader thread —
    /// receivers in another thread will get queued delivery automatically.
    void outputReceived(const QString& chunk);

    /// Child process exited with the given status. Emitted after the reader
    /// thread has drained the remaining bytes.
    void processExited(int code);
};

} // namespace dante::terminal
