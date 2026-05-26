#include "ConPtyAdapter.h"

#include <QDebug>

#if defined(Q_OS_WIN)
  // We rely on _WIN32_WINNT being set to 0x0A00 by the top-level CMakeLists,
  // which gates the ConPTY pseudo-console symbols. ConPTY shipped in Win10
  // 1809 (build 17763); pre-1809 users will get a clear error at runtime.
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
  #endif
  #include <windows.h>
  #include <consoleapi.h>
  #include <processthreadsapi.h>
  #include <synchapi.h>
  #include <memory>
  #include <vector>
#endif

namespace dante::infra {

// ---------------------------------------------------------------------------
// ConPtyReader — pulls bytes off the output pipe on its own thread and emits
// them up to the adapter. Blocking ReadFile is fine because we close the pipe
// from the owner thread to wake it up during shutdown.
// ---------------------------------------------------------------------------
class ConPtyReader : public QObject {
    Q_OBJECT
public:
    ConPtyReader(void* hOut, void* hProcess, std::atomic<bool>* stop)
        : hOut_(hOut), hProcess_(hProcess), stop_(stop) {}

public slots:
    void run() {
#if defined(Q_OS_WIN)
        std::vector<char> buf(4096);
        for (;;) {
            if (stop_->load(std::memory_order_relaxed)) break;
            DWORD got = 0;
            const BOOL ok = ::ReadFile(static_cast<HANDLE>(hOut_),
                                       buf.data(),
                                       static_cast<DWORD>(buf.size()),
                                       &got,
                                       nullptr);
            if (!ok || got == 0) break;     // pipe closed / broken
            emit chunk(QString::fromUtf8(buf.data(), static_cast<int>(got)));
        }
        DWORD code = 0;
        if (hProcess_) {
            ::WaitForSingleObject(static_cast<HANDLE>(hProcess_), 1000);
            ::GetExitCodeProcess(static_cast<HANDLE>(hProcess_), &code);
        }
        emit finished(static_cast<int>(code));
#else
        emit finished(0);
#endif
    }

signals:
    void chunk(const QString& s);
    void finished(int code);

private:
    void* hOut_;
    void* hProcess_;
    std::atomic<bool>* stop_;
};

// ---------------------------------------------------------------------------
// ConPtyAdapter
// ---------------------------------------------------------------------------
ConPtyAdapter::ConPtyAdapter(QObject* parent)
    : IPtyAdapter(parent)
{}

ConPtyAdapter::~ConPtyAdapter() {
    ConPtyAdapter::kill();
    teardown();
}

bool ConPtyAdapter::start(const QString& shell, const QString& cwd, int cols, int rows) {
#if defined(Q_OS_WIN)
    cols_ = qMax(1, cols);
    rows_ = qMax(1, rows);

    // Defensive NUL scrub — Tauri parity (CreateProcessW eats embedded NULs
    // and produces inscrutable failures otherwise).
    QString shellPath = shell; shellPath.remove(QChar(0));
    QString workdir   = cwd;   workdir.remove(QChar(0));
    shellPath = shellPath.trimmed();
    workdir   = workdir.trimmed();

    if (shellPath.isEmpty()) {
        // Sensible Win default: pwsh -> powershell -> cmd. We let the caller
        // upstream resolve absolutes; if they passed empty, fall back to cmd.
        shellPath = QStringLiteral("cmd.exe");
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;

    HANDLE inR = nullptr, inW = nullptr, outR = nullptr, outW = nullptr;
    if (!::CreatePipe(&inR, &inW, &sa, 0) ||
        !::CreatePipe(&outR, &outW, &sa, 0)) {
        error_ = QStringLiteral("CreatePipe failed: %1").arg(::GetLastError());
        if (inR) ::CloseHandle(inR);
        if (inW) ::CloseHandle(inW);
        if (outR) ::CloseHandle(outR);
        if (outW) ::CloseHandle(outW);
        return false;
    }

    COORD size{};
    size.X = static_cast<SHORT>(cols_);
    size.Y = static_cast<SHORT>(rows_);

    HPCON hpc = nullptr;
    const HRESULT hr = ::CreatePseudoConsole(size, inR, outW, 0, &hpc);
    if (FAILED(hr)) {
        error_ = QStringLiteral("CreatePseudoConsole failed: hr=0x%1")
                     .arg(QString::number(static_cast<uint>(hr), 16));
        ::CloseHandle(inR); ::CloseHandle(inW);
        ::CloseHandle(outR); ::CloseHandle(outW);
        return false;
    }
    // Per docs: once the PC owns these endpoints, the caller should close them.
    ::CloseHandle(inR);
    ::CloseHandle(outW);
    inR = nullptr; outW = nullptr;

    // STARTUPINFOEX with PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE_ATTRIBUTE.
    SIZE_T attrSize = 0;
    ::InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
    auto attrBuf = std::make_unique<unsigned char[]>(attrSize);
    auto* attrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrBuf.get());
    if (!::InitializeProcThreadAttributeList(attrList, 1, 0, &attrSize)) {
        error_ = QStringLiteral("InitializeProcThreadAttributeList failed: %1")
                     .arg(::GetLastError());
        ::ClosePseudoConsole(hpc);
        ::CloseHandle(inW); ::CloseHandle(outR);
        return false;
    }
    if (!::UpdateProcThreadAttribute(
            attrList, 0,
            PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
            hpc, sizeof(hpc), nullptr, nullptr)) {
        error_ = QStringLiteral("UpdateProcThreadAttribute failed: %1")
                     .arg(::GetLastError());
        ::DeleteProcThreadAttributeList(attrList);
        ::ClosePseudoConsole(hpc);
        ::CloseHandle(inW); ::CloseHandle(outR);
        return false;
    }

    STARTUPINFOEXW si{};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    si.lpAttributeList = attrList;

    PROCESS_INFORMATION pi{};

    // Command line buffer is mutated by CreateProcessW; std::wstring gives
    // us writable storage with a guaranteed NUL terminator.
    std::wstring cmd = shellPath.toStdWString();
    std::wstring cwdW = workdir.toStdWString();
    LPCWSTR cwdPtr = cwdW.empty() ? nullptr : cwdW.c_str();

    const BOOL spawned = ::CreateProcessW(
        nullptr,
        cmd.data(),
        nullptr, nullptr,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT,
        nullptr,
        cwdPtr,
        &si.StartupInfo,
        &pi);
    ::DeleteProcThreadAttributeList(attrList);

    if (!spawned) {
        error_ = QStringLiteral("CreateProcessW failed: %1").arg(::GetLastError());
        ::ClosePseudoConsole(hpc);
        ::CloseHandle(inW); ::CloseHandle(outR);
        return false;
    }

    hPC_           = hpc;
    hInputWrite_   = inW;
    hOutputRead_   = outR;
    hProcess_      = pi.hProcess;
    hThread_       = pi.hThread;
    stopFlag_.store(false, std::memory_order_relaxed);

    readerThread_ = new QThread(this);
    reader_       = new ConPtyReader(hOutputRead_, hProcess_, &stopFlag_);
    reader_->moveToThread(readerThread_);
    connect(readerThread_, &QThread::started, reader_, &ConPtyReader::run);
    connect(reader_, &ConPtyReader::chunk,
            this, [this](const QString& s) { emit outputReceived(s); },
            Qt::QueuedConnection);
    connect(reader_, &ConPtyReader::finished,
            this, [this](int code) { emit processExited(code); },
            Qt::QueuedConnection);
    connect(readerThread_, &QThread::finished, reader_, &QObject::deleteLater);
    readerThread_->start();
    return true;
#else
    (void)shell; (void)cwd; (void)cols; (void)rows;
    error_ = QStringLiteral("ConPtyAdapter: not supported on this platform");
    return false;
#endif
}

void ConPtyAdapter::resize(int cols, int rows) {
#if defined(Q_OS_WIN)
    cols_ = qMax(1, cols);
    rows_ = qMax(1, rows);
    if (!hPC_) return;
    COORD s{};
    s.X = static_cast<SHORT>(cols_);
    s.Y = static_cast<SHORT>(rows_);
    const HRESULT hr = ::ResizePseudoConsole(static_cast<HPCON>(hPC_), s);
    if (FAILED(hr)) {
        qWarning() << "ConPtyAdapter::resize ResizePseudoConsole failed hr="
                   << QString::number(static_cast<uint>(hr), 16);
    }
#else
    (void)cols; (void)rows;
#endif
}

void ConPtyAdapter::write(const QByteArray& bytes) {
#if defined(Q_OS_WIN)
    if (!hInputWrite_ || bytes.isEmpty()) return;
    DWORD written = 0;
    const char* p = bytes.constData();
    DWORD remaining = static_cast<DWORD>(bytes.size());
    while (remaining > 0) {
        if (!::WriteFile(static_cast<HANDLE>(hInputWrite_),
                         p, remaining, &written, nullptr)) break;
        if (written == 0) break;
        p += written;
        remaining -= written;
    }
#else
    (void)bytes;
#endif
}

void ConPtyAdapter::kill() {
#if defined(Q_OS_WIN)
    // Closing the pseudoconsole + handles causes the shell to receive EOF on
    // its console input, which is the polite way to ask it to exit.
    if (hProcess_) {
        ::TerminateProcess(static_cast<HANDLE>(hProcess_), 0);
        ::WaitForSingleObject(static_cast<HANDLE>(hProcess_), 1500);
    }
    teardown();
#endif
}

void ConPtyAdapter::teardown() {
#if defined(Q_OS_WIN)
    stopFlag_.store(true, std::memory_order_relaxed);

    if (hPC_)          { ::ClosePseudoConsole(static_cast<HPCON>(hPC_)); hPC_ = nullptr; }
    if (hInputWrite_)  { ::CloseHandle(static_cast<HANDLE>(hInputWrite_));  hInputWrite_  = nullptr; }
    if (hOutputRead_)  { ::CloseHandle(static_cast<HANDLE>(hOutputRead_));  hOutputRead_  = nullptr; }

    if (readerThread_) {
        readerThread_->quit();
        readerThread_->wait(2000);
        readerThread_->deleteLater();
        readerThread_ = nullptr;
        reader_       = nullptr;
    }

    if (hThread_)  { ::CloseHandle(static_cast<HANDLE>(hThread_));  hThread_  = nullptr; }
    if (hProcess_) { ::CloseHandle(static_cast<HANDLE>(hProcess_)); hProcess_ = nullptr; }
#endif
}

} // namespace dante::infra

#include "ConPtyAdapter.moc"
