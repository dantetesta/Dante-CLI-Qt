// OS adapter — spawns a shell with PTY semantics.
// Windows: real ConPTY via Win32 (CreatePseudoConsole + STARTUPINFOEX).
// macOS/Linux: forkpty().
//
// MVP NOTE: this version uses QProcess on all platforms (no PTY).
// That means apps like vim/htop won't render correctly. Replacing the
// internals with ConPTY/forkpty is a self-contained follow-up; the
// `ShellAdapter` API surface won't change.

#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QByteArray>

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
    QString sanitize(QString s) const;   // strip NUL bytes (Win11 dirs crate leak parity)

    QProcess proc_;
    QString  error_;
    int      cols_{80};
    int      rows_{24};
};

} // namespace dante::infra
