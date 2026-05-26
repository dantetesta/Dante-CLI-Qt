// POSIX forkpty-based adapter (macOS / Linux / *BSD).
//
// Spawns the shell via forkpty(3), keeps the master fd in the parent process,
// drives a blocking-read loop on a dedicated QThread, and propagates window
// size changes through ioctl(TIOCSWINSZ).
//
// On Windows (Q_OS_WIN), this class compiles to a hollow stub so the unified
// `ShellAdapter` facade has a single header to depend on without leaking
// platform-specific includes. The body lives entirely behind
// `#if defined(Q_OS_UNIX)` in the .cpp.

#pragma once

#include "terminal/IPtyAdapter.h"

#include <QThread>
#include <atomic>
#include <memory>

namespace dante::infra {

class UnixPtyReader; // fwd: lives on its own QThread

class UnixPtyAdapter : public dante::terminal::IPtyAdapter {
    Q_OBJECT
public:
    explicit UnixPtyAdapter(QObject* parent = nullptr);
    ~UnixPtyAdapter() override;

    bool start(const QString& shell, const QString& cwd, int cols, int rows) override;
    void resize(int cols, int rows) override;
    void write(const QByteArray& bytes) override;
    void kill() override;
    QString errorString() const override { return error_; }

private:
    void teardownReader();   // joins thread + closes fd; safe to call repeatedly

    int      masterFd_{-1};
    int      childPid_{-1};
    int      cols_{80};
    int      rows_{24};
    QString  error_;

    QThread*                          readerThread_{nullptr};
    UnixPtyReader*                    reader_{nullptr};
    std::atomic<bool>                 stopFlag_{false};
};

} // namespace dante::infra
