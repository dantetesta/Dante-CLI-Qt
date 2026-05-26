// OS adapter — spawns a shell with REAL PTY semantics.
//
//   - Windows: `ConPtyAdapter` (CreatePseudoConsole + STARTUPINFOEXW)
//   - macOS / Linux: `UnixPtyAdapter` (forkpty + ioctl(TIOCSWINSZ))
//
// This class is now a thin facade that owns a concrete `IPtyAdapter` chosen
// at construction time. The public surface is preserved so existing callers
// (TerminalSession etc.) compile without changes.

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>

namespace dante::terminal { class IPtyAdapter; }

namespace dante::infra {

class ShellAdapter : public QObject {
    Q_OBJECT
public:
    explicit ShellAdapter(QObject* parent = nullptr);
    ~ShellAdapter() override;

    bool start(const QString& shell, const QString& cwd, int cols, int rows);
    void resize(int cols, int rows);
    void write(const QByteArray& bytes);
    void kill();

    QString errorString() const { return error_; }

signals:
    void outputReceived(const QString& chunk);
    void processExited(int code);

private:
    QString resolveShell(QString candidate) const;
    QString sanitize(QString s) const;   // strip NUL bytes (Win path parity)

    std::unique_ptr<dante::terminal::IPtyAdapter> pty_;
    QString error_;
    int     cols_{80};
    int     rows_{24};
};

} // namespace dante::infra
