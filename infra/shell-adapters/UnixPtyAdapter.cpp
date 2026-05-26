#include "UnixPtyAdapter.h"

#include <QDebug>
#include <QFileInfo>

#include <array>

#if defined(Q_OS_UNIX)
  #include <cerrno>
  #include <csignal>
  #include <cstdlib>
  #include <cstring>
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #include <sys/wait.h>
  #include <termios.h>
  #include <unistd.h>
  #if defined(Q_OS_MAC) || defined(Q_OS_BSD4)
    #include <util.h>
  #else
    #include <pty.h>
  #endif
#endif

namespace dante::infra {

// ---------------------------------------------------------------------------
// UnixPtyReader — runs on its own QThread. Blocks in read(2) until either
// the master fd closes (child exit) or we're asked to stop. Emits chunks
// upward via signal; the IPtyAdapter receiver in the GUI thread sees them
// through queued connection delivery (Qt picks that automatically because the
// signal is emitted from a non-owner thread).
// ---------------------------------------------------------------------------
class UnixPtyReader : public QObject {
    Q_OBJECT
public:
    UnixPtyReader(int fd, int pid, std::atomic<bool>* stop)
        : fd_(fd), pid_(pid), stop_(stop) {}

public slots:
    void run() {
#if defined(Q_OS_UNIX)
        std::array<char, 4096> buf{};
        while (!stop_->load(std::memory_order_relaxed)) {
            const ssize_t n = ::read(fd_, buf.data(), buf.size());
            if (n > 0) {
                emit chunk(QString::fromUtf8(buf.data(), static_cast<int>(n)));
                continue;
            }
            if (n == 0) break;                  // EOF — child closed master
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            break;                              // EIO on Linux when slave gone
        }
        int status = 0;
        if (pid_ > 0) {
            // Non-blocking reap; the parent's kill() will follow up with
            // waitpid if we haven't observed the exit yet.
            ::waitpid(pid_, &status, WNOHANG);
        }
        const int code = WIFEXITED(status) ? WEXITSTATUS(status)
                       : WIFSIGNALED(status) ? 128 + WTERMSIG(status)
                       : 0;
        emit finished(code);
#else
        emit finished(0);
#endif
    }

signals:
    void chunk(const QString& s);
    void finished(int code);

private:
    int fd_;
    int pid_;
    std::atomic<bool>* stop_;
};

// ---------------------------------------------------------------------------
// UnixPtyAdapter
// ---------------------------------------------------------------------------
UnixPtyAdapter::UnixPtyAdapter(QObject* parent)
    : IPtyAdapter(parent)
{}

UnixPtyAdapter::~UnixPtyAdapter() {
    UnixPtyAdapter::kill();
    teardownReader();
}

bool UnixPtyAdapter::start(const QString& shell, const QString& cwd, int cols, int rows) {
#if defined(Q_OS_UNIX)
    cols_ = qMax(1, cols);
    rows_ = qMax(1, rows);

    // Scrub NUL bytes — the Tauri version learned this the hard way when
    // Windows paths smuggled embedded NULs into CreateProcessW. Same defensive
    // posture here even though POSIX doesn't have the same parser quirk.
    QString shellPath = shell; shellPath.remove(QChar(0));
    QString workdir   = cwd;   workdir.remove(QChar(0));
    shellPath = shellPath.trimmed();
    workdir   = workdir.trimmed();

    if (shellPath.isEmpty()) {
        shellPath = qEnvironmentVariable("SHELL", QStringLiteral("/bin/zsh"));
    }
    if (!QFileInfo::exists(shellPath)) {
        error_ = QStringLiteral("shell not found: %1").arg(shellPath);
        return false;
    }

    struct winsize ws{};
    ws.ws_col = static_cast<unsigned short>(cols_);
    ws.ws_row = static_cast<unsigned short>(rows_);

    int master = -1;
    const pid_t pid = ::forkpty(&master, nullptr, nullptr, &ws);
    if (pid < 0) {
        error_ = QStringLiteral("forkpty failed: %1").arg(::strerror(errno));
        return false;
    }
    if (pid == 0) {
        // Child. We're inside a new session attached to the slave PTY.
        if (!workdir.isEmpty()) {
            (void)::chdir(workdir.toLocal8Bit().constData());
        }
        ::setenv("TERM", "xterm-256color", 1);
        ::setenv("COLORTERM", "truecolor", 1);
        ::setenv("DANTE_CLI", "1", 1);
        if (::getenv("LANG") == nullptr) ::setenv("LANG", "en_US.UTF-8", 1);

        const std::string sh = shellPath.toStdString();
        // Login shell — pre-existing QProcess code did the same on POSIX.
        ::execl(sh.c_str(), sh.c_str(), "-l", static_cast<char*>(nullptr));
        // execl only returns on failure.
        _exit(127);
    }

    // Parent — non-blocking writes are nicer but tests showed a blocking
    // master fd is fine and avoids partial-write bookkeeping.
    masterFd_ = master;
    childPid_ = pid;
    stopFlag_.store(false, std::memory_order_relaxed);

    readerThread_ = new QThread(this);
    reader_       = new UnixPtyReader(masterFd_, childPid_, &stopFlag_);
    reader_->moveToThread(readerThread_);

    connect(readerThread_, &QThread::started, reader_, &UnixPtyReader::run);
    connect(reader_, &UnixPtyReader::chunk,
            this, [this](const QString& s) { emit outputReceived(s); },
            Qt::QueuedConnection);
    connect(reader_, &UnixPtyReader::finished,
            this, [this](int code) { emit processExited(code); },
            Qt::QueuedConnection);
    connect(readerThread_, &QThread::finished, reader_, &QObject::deleteLater);

    readerThread_->start();
    return true;
#else
    (void)shell; (void)cwd; (void)cols; (void)rows;
    error_ = QStringLiteral("UnixPtyAdapter: not supported on this platform");
    return false;
#endif
}

void UnixPtyAdapter::resize(int cols, int rows) {
#if defined(Q_OS_UNIX)
    cols_ = qMax(1, cols);
    rows_ = qMax(1, rows);
    if (masterFd_ < 0) return;
    struct winsize ws{};
    ws.ws_col = static_cast<unsigned short>(cols_);
    ws.ws_row = static_cast<unsigned short>(rows_);
    if (::ioctl(masterFd_, TIOCSWINSZ, &ws) < 0) {
        qWarning() << "UnixPtyAdapter::resize TIOCSWINSZ failed:" << ::strerror(errno);
    }
#else
    (void)cols; (void)rows;
#endif
}

void UnixPtyAdapter::write(const QByteArray& bytes) {
#if defined(Q_OS_UNIX)
    if (masterFd_ < 0 || bytes.isEmpty()) return;
    const char* p = bytes.constData();
    qsizetype remaining = bytes.size();
    while (remaining > 0) {
        const ssize_t w = ::write(masterFd_, p, static_cast<size_t>(remaining));
        if (w > 0) { p += w; remaining -= w; continue; }
        if (w < 0 && errno == EINTR) continue;
        break;  // EPIPE / EBADF / EIO — child went away
    }
#else
    (void)bytes;
#endif
}

void UnixPtyAdapter::kill() {
#if defined(Q_OS_UNIX)
    if (childPid_ > 0) {
        ::kill(childPid_, SIGHUP);
        // Give the shell a moment to flush; if it stays, escalate.
        int status = 0;
        for (int i = 0; i < 15; ++i) {
            if (::waitpid(childPid_, &status, WNOHANG) != 0) { childPid_ = -1; break; }
            QThread::msleep(20);
        }
        if (childPid_ > 0) {
            ::kill(childPid_, SIGKILL);
            ::waitpid(childPid_, &status, 0);
            childPid_ = -1;
        }
    }
    teardownReader();
#endif
}

void UnixPtyAdapter::teardownReader() {
#if defined(Q_OS_UNIX)
    stopFlag_.store(true, std::memory_order_relaxed);
    if (masterFd_ >= 0) {
        ::close(masterFd_);   // unblocks the reader's read() with EOF
        masterFd_ = -1;
    }
    if (readerThread_) {
        readerThread_->quit();
        readerThread_->wait(2000);
        readerThread_->deleteLater();
        readerThread_ = nullptr;
        reader_       = nullptr;   // owned by thread; deleteLater wired up
    }
#endif
}

} // namespace dante::infra

// AUTOMOC needs the worker QObject's moc generated; including the moc
// inline keeps the class private to this TU without adding it to CMake.
#include "UnixPtyAdapter.moc"
