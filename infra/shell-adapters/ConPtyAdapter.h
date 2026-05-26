// Windows ConPTY adapter (Win10 1809+).
//
// Uses CreatePseudoConsole / STARTUPINFOEXW with
// PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE_ATTRIBUTE, then drives a blocking
// ReadFile loop on a dedicated QThread. On non-Windows platforms the class
// compiles to an empty stub so callers can include it unconditionally.

#pragma once

#include "terminal/IPtyAdapter.h"

#include <QThread>
#include <atomic>

namespace dante::infra {

class ConPtyReader;  // fwd: blocking-read worker

class ConPtyAdapter : public dante::terminal::IPtyAdapter {
    Q_OBJECT
public:
    explicit ConPtyAdapter(QObject* parent = nullptr);
    ~ConPtyAdapter() override;

    bool start(const QString& shell, const QString& cwd, int cols, int rows) override;
    void resize(int cols, int rows) override;
    void write(const QByteArray& bytes) override;
    void kill() override;
    QString errorString() const override { return error_; }

private:
    void teardown();

    // We hide the Win32 handles behind void* so this header stays clean of
    // <windows.h> includes (clients don't need it).
    void*  hPC_{nullptr};       // HPCON
    void*  hInputRead_{nullptr};
    void*  hInputWrite_{nullptr};
    void*  hOutputRead_{nullptr};
    void*  hOutputWrite_{nullptr};
    void*  hProcess_{nullptr};
    void*  hThread_{nullptr};
    int    cols_{80};
    int    rows_{24};
    QString error_;

    QThread*           readerThread_{nullptr};
    ConPtyReader*      reader_{nullptr};
    std::atomic<bool>  stopFlag_{false};
};

} // namespace dante::infra
