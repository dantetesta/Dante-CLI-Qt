// Structured logging facade. Today: QtMessageHandler installing a file+stderr
// sink under AppData/logs. Tomorrow: swap to spdlog without touching callers.
#pragma once
#include <QString>

namespace dante::telemetry {
    /// Install the global Qt message handler. Call once from main().
    void install();
    /// Returns the absolute path of the active log file.
    QString currentLogPath();
}
