#include "ProcessStats.h"

#include <chrono>
#include <thread>

#if defined(_WIN32)
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <psapi.h>
#elif defined(__APPLE__)
  #include <mach/mach.h>
  #include <mach/mach_init.h>
  #include <mach/task.h>
  #include <mach/task_info.h>
  #include <unistd.h>
#else
  #include <unistd.h>
  #include <cstdio>
  #include <cstring>
#endif

namespace dante::system {

namespace {

using Clock = std::chrono::steady_clock;

// Returns the cumulative user+system CPU time of the current process in
// microseconds, or 0 if the underlying syscall failed.
std::uint64_t cumulativeCpuMicros() {
#if defined(_WIN32)
    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    if (!GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser)) {
        return 0;
    }
    // FILETIME is in 100-ns ticks.
    auto toMicros = [](const FILETIME& ft) -> std::uint64_t {
        ULARGE_INTEGER u;
        u.LowPart  = ft.dwLowDateTime;
        u.HighPart = ft.dwHighDateTime;
        return u.QuadPart / 10ULL;
    };
    return toMicros(ftKernel) + toMicros(ftUser);
#elif defined(__APPLE__)
    task_thread_times_info_data_t info{};
    mach_msg_type_number_t count = TASK_THREAD_TIMES_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_THREAD_TIMES_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
        return 0;
    }
    // task_thread_times_info covers *live* threads only. Combine with
    // basic_info_64 to include terminated threads — otherwise long-lived
    // processes that recycle worker threads under-report CPU%.
    task_basic_info_64_data_t basic{};
    mach_msg_type_number_t bcount = TASK_BASIC_INFO_64_COUNT;
    std::uint64_t terminated = 0;
    if (task_info(mach_task_self(), TASK_BASIC_INFO_64,
                  reinterpret_cast<task_info_t>(&basic), &bcount) == KERN_SUCCESS) {
        terminated = std::uint64_t(basic.user_time.seconds)    * 1'000'000ULL
                   + std::uint64_t(basic.user_time.microseconds)
                   + std::uint64_t(basic.system_time.seconds)  * 1'000'000ULL
                   + std::uint64_t(basic.system_time.microseconds);
    }
    const std::uint64_t live =
          std::uint64_t(info.user_time.seconds)    * 1'000'000ULL
        + std::uint64_t(info.user_time.microseconds)
        + std::uint64_t(info.system_time.seconds)  * 1'000'000ULL
        + std::uint64_t(info.system_time.microseconds);
    return live + terminated;
#else
    // Linux: /proc/self/stat fields 14 (utime) + 15 (stime) in clock ticks.
    FILE* f = std::fopen("/proc/self/stat", "r");
    if (!f) return 0;
    char buf[1024];
    size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
    std::fclose(f);
    if (n == 0) return 0;
    buf[n] = '\0';
    // Find the last ')' to skip the (comm) field which can contain spaces.
    char* rparen = std::strrchr(buf, ')');
    if (!rparen) return 0;
    // Fields after comm start at index 3 (state). utime is field 14 overall,
    // i.e. 11 tokens after the state char (state, ppid, pgrp, session,
    // tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt, utime).
    long long utime = 0, stime = 0;
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    int parsed = std::sscanf(rparen + 2,
        "%c %d %d %d %d %d %lu %lu %lu %lu %lu %lld %lld",
        &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
        &flags, &minflt, &cminflt, &majflt, &cmajflt,
        &utime, &stime);
    if (parsed < 13) return 0;
    const long hz = sysconf(_SC_CLK_TCK);
    if (hz <= 0) return 0;
    // Convert clock ticks → microseconds.
    return std::uint64_t((utime + stime)) * 1'000'000ULL / std::uint64_t(hz);
#endif
}

} // namespace

struct ProcessStatsSampler::Impl {
    std::uint64_t   lastCpuMicros = 0;
    Clock::time_point lastSample  = Clock::time_point{};
    bool primed = false;
};

ProcessStatsSampler::ProcessStatsSampler()  : d_(new Impl) {}
ProcessStatsSampler::~ProcessStatsSampler() { delete d_; }

double ProcessStatsSampler::currentCpuPercent() {
    const std::uint64_t now = cumulativeCpuMicros();
    const auto wallNow = Clock::now();

    if (!d_->primed) {
        d_->lastCpuMicros = now;
        d_->lastSample    = wallNow;
        d_->primed        = true;
        return 0.0;
    }

    const std::uint64_t deltaCpu =
        (now >= d_->lastCpuMicros) ? (now - d_->lastCpuMicros) : 0;
    const auto elapsed = wallNow - d_->lastSample;
    const std::uint64_t elapsedMicros = std::uint64_t(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());

    d_->lastCpuMicros = now;
    d_->lastSample    = wallNow;

    if (elapsedMicros == 0) return 0.0;

    double pct = (double(deltaCpu) / double(elapsedMicros)) * 100.0;
    const double cap = double(coreCount()) * 100.0;
    if (pct < 0.0) pct = 0.0;
    if (pct > cap) pct = cap;
    return pct;
}

std::int64_t ProcessStatsSampler::currentResidentBytes() const {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(),
                              reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                              sizeof(pmc))) {
        return 0;
    }
    return std::int64_t(pmc.WorkingSetSize);
#elif defined(__APPLE__)
    mach_task_basic_info_data_t info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
        return 0;
    }
    return std::int64_t(info.resident_size);
#else
    FILE* f = std::fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[256];
    std::int64_t kb = 0;
    while (std::fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, "VmRSS:", 6) == 0) {
            std::sscanf(line + 6, " %lld", reinterpret_cast<long long*>(&kb));
            break;
        }
    }
    std::fclose(f);
    return kb * 1024;
#endif
}

int coreCount() {
#if defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return int(si.dwNumberOfProcessors > 0 ? si.dwNumberOfProcessors : 1);
#else
    const long n = sysconf(_SC_NPROCESSORS_ONLN);
    return int(n > 0 ? n : 1);
#endif
}

} // namespace dante::system
