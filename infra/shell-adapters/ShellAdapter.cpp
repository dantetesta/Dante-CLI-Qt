#include "ShellAdapter.h"
#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>

namespace dante::infra {

ShellAdapter::ShellAdapter(QObject* parent)
    : QObject(parent)
{
    proc_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&proc_, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray data = proc_.readAllStandardOutput();
        emit outputReceived(QString::fromUtf8(data));
    });
    connect(&proc_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) { emit processExited(code); });
}

ShellAdapter::~ShellAdapter() { kill(); }

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

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("TERM", "xterm-256color");
    env.insert("COLORTERM", "truecolor");
    env.insert("DANTE_CLI", "1");
    if (!env.contains("LANG")) env.insert("LANG", "en_US.UTF-8");
    proc_.setProcessEnvironment(env);
    if (!workdir.isEmpty() && QFileInfo::exists(workdir)) {
        proc_.setWorkingDirectory(workdir);
    }

    QStringList args;
#if !defined(Q_OS_WIN)
    args << "-l";   // login shell
#endif
    proc_.start(resolved, args);
    if (!proc_.waitForStarted(3000)) {
        error_ = QStringLiteral("waitForStarted timeout: %1").arg(proc_.errorString());
        return false;
    }
    return true;
}

void ShellAdapter::resize(int cols, int rows) {
    cols_ = qMax(1, cols);
    rows_ = qMax(1, rows);
    // QProcess has no resize; needs real PTY adapter. No-op for MVP.
}

void ShellAdapter::write(const QByteArray& bytes) {
    if (proc_.state() != QProcess::Running) return;
    proc_.write(bytes);
}

void ShellAdapter::kill() {
    if (proc_.state() == QProcess::Running) {
        proc_.terminate();
        if (!proc_.waitForFinished(1500)) {
            proc_.kill();
            proc_.waitForFinished(500);
        }
    }
}

} // namespace dante::infra
