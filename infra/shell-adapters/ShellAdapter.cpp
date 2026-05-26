#include "ShellAdapter.h"
#include "terminal/IPtyAdapter.h"

#if defined(Q_OS_WIN)
  #include "ConPtyAdapter.h"
#else
  #include "UnixPtyAdapter.h"
#endif

#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>

namespace dante::infra {

namespace {

std::unique_ptr<dante::terminal::IPtyAdapter> makePlatformPty(QObject* owner) {
#if defined(Q_OS_WIN)
    return std::unique_ptr<dante::terminal::IPtyAdapter>(new ConPtyAdapter(owner));
#else
    return std::unique_ptr<dante::terminal::IPtyAdapter>(new UnixPtyAdapter(owner));
#endif
}

} // namespace

ShellAdapter::ShellAdapter(QObject* parent)
    : QObject(parent)
    , pty_(makePlatformPty(this))
{
    connect(pty_.get(), &dante::terminal::IPtyAdapter::outputReceived,
            this, &ShellAdapter::outputReceived);
    connect(pty_.get(), &dante::terminal::IPtyAdapter::processExited,
            this, &ShellAdapter::processExited);
}

ShellAdapter::~ShellAdapter() {
    if (pty_) pty_->kill();
}

QString ShellAdapter::sanitize(QString s) const {
    s.remove(QChar(0));     // strip NUL bytes
    return s.trimmed();
}

QString ShellAdapter::resolveShell(QString candidate) const {
    candidate = sanitize(candidate);
    if (!candidate.isEmpty()) return candidate;
#if defined(Q_OS_WIN)
    for (const auto& name : { QStringLiteral("pwsh.exe"),
                              QStringLiteral("powershell.exe"),
                              QStringLiteral("cmd.exe") }) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty()) return path;
    }
    return QStringLiteral("cmd.exe");
#elif defined(Q_OS_MAC)
    return qEnvironmentVariable("SHELL", QStringLiteral("/bin/zsh"));
#else
    return qEnvironmentVariable("SHELL", QStringLiteral("/bin/bash"));
#endif
}

bool ShellAdapter::start(const QString& shell, const QString& cwd, int cols, int rows) {
    cols_ = qMax(1, cols);
    rows_ = qMax(1, rows);
    const QString resolved = resolveShell(shell);
    const QString workdir  = sanitize(cwd);

    if (!pty_) {
        error_ = QStringLiteral("ShellAdapter: pty backend not constructed");
        return false;
    }
    const bool ok = pty_->start(resolved, workdir, cols_, rows_);
    if (!ok) {
        error_ = pty_->errorString();
        return false;
    }
    error_.clear();
    return true;
}

void ShellAdapter::resize(int cols, int rows) {
    cols_ = qMax(1, cols);
    rows_ = qMax(1, rows);
    if (pty_) pty_->resize(cols_, rows_);
}

void ShellAdapter::write(const QByteArray& bytes) {
    if (pty_) pty_->write(bytes);
}

void ShellAdapter::kill() {
    if (pty_) pty_->kill();
}

} // namespace dante::infra
