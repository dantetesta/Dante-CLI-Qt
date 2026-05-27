// Cross-platform CPU% / RSS sampler for the *current* process.
//
// Pure C++ (no Qt) on purpose so it can be reused from a CLI tool or test
// harness without dragging QtCore in. The Qt-side controller adapts it.
//
// Behavior mirrors the Swift sibling `ProcessStats.swift`:
//   - CPU% is computed from a delta between two samples of cumulative user+
//     system CPU time, divided by wall-clock elapsed time. First call returns
//     0 because there is no baseline yet — caller must keep the Sampler alive
//     across ticks (the previous-sample cache is per-instance, no globals,
//     no mutex).
//   - Resident bytes on Windows uses `GetProcessMemoryInfo().WorkingSetSize`.
//     On macOS uses `task_info(MACH_TASK_BASIC_INFO).resident_size` — the same
//     number Activity Monitor reports for "Real Memory" (close enough for an
//     indicator; we don't need phys_footprint precision here).
//   - Linux fallback reads `/proc/self/stat` for CPU clock ticks and
//     `/proc/self/status` (`VmRSS`) for resident bytes. Optional; only
//     compiled when neither Q_OS_WIN nor Q_OS_MAC is defined.

#pragma once

#include <cstdint>

namespace dante::system {

/// Per-instance state for sampling CPU usage of the current process.
/// Holding instance-local previous samples avoids any global mutex.
class ProcessStatsSampler {
public:
    ProcessStatsSampler();
    ~ProcessStatsSampler();

    ProcessStatsSampler(const ProcessStatsSampler&) = delete;
    ProcessStatsSampler& operator=(const ProcessStatsSampler&) = delete;

    /// CPU% of the current process in the [0, 100 * cores] range.
    /// First call returns 0 (no baseline). Subsequent calls return the
    /// delta of cumulative user+system CPU time over the elapsed wall-clock
    /// interval since the previous invocation, multiplied by 100.
    double currentCpuPercent();

    /// Resident set size in bytes for the current process.
    std::int64_t currentResidentBytes() const;

private:
    struct Impl;
    Impl* d_;
};

/// Convenience: number of logical CPU cores reported by the OS.
/// Used to cap CPU% display at `coreCount * 100`.
int coreCount();

} // namespace dante::system
