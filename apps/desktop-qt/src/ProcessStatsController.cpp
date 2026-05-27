#include "ProcessStatsController.h"
#include "system/ProcessStats.h"

#include <QTimer>
#include <QtMath>
#include <cmath>

namespace dante {

namespace {
    constexpr int    kDefaultIntervalMs = 1000;
    constexpr double kCpuJitterPct      = 0.5;   // ignore moves smaller than this
    constexpr double kMemJitterMB       = 1.0;
}

ProcessStatsController::ProcessStatsController(QObject* parent)
    : QObject(parent)
    , sampler_(std::make_unique<system::ProcessStatsSampler>())
    , timer_(new QTimer(this))
{
    timer_->setInterval(kDefaultIntervalMs);
    timer_->setSingleShot(false);
    connect(timer_, &QTimer::timeout, this, &ProcessStatsController::sample);
    // Prime the sampler so the first user-visible tick has a real delta to
    // compare against (otherwise the first 1000ms always reads 0%).
    (void)sampler_->currentCpuPercent();
    rebuildSummary();
    timer_->start();
}

ProcessStatsController::~ProcessStatsController() = default;

int ProcessStatsController::samplingInterval() const {
    return timer_ ? timer_->interval() : kDefaultIntervalMs;
}

void ProcessStatsController::setSamplingInterval(int ms) {
    if (ms <= 0 || !timer_) return;
    if (timer_->interval() == ms) return;
    timer_->setInterval(ms);
    emit samplingIntervalChanged();
    // Force a fresh summary so the tooltip's "sampling: Ns" line updates.
    rebuildSummary();
}

void ProcessStatsController::sample() {
    const double cpu = sampler_->currentCpuPercent();
    const double mem = double(sampler_->currentResidentBytes())
                       / (1024.0 * 1024.0);

    bool changed = false;
    if (std::fabs(cpu - cpuPercent_) >= kCpuJitterPct) {
        cpuPercent_ = cpu;
        emit cpuPercentChanged();
        changed = true;
    }
    if (std::fabs(mem - memoryMB_) >= kMemJitterMB) {
        memoryMB_ = mem;
        emit memoryMBChanged();
        changed = true;
    }
    if (changed) rebuildSummary();
}

void ProcessStatsController::rebuildSummary() {
    formattedSummary_ = QString("CPU %1%% · %2 MB")
        .arg(qRound(cpuPercent_))
        .arg(qRound(memoryMB_));
    tooltipText_ = QString("CPU sampling: %1s · RSS: %2 MB · CPU: %3%%")
        .arg(QString::number(samplingInterval() / 1000.0, 'f', 0))
        .arg(QString::number(memoryMB_, 'f', 1))
        .arg(QString::number(cpuPercent_, 'f', 1));
    emit formattedSummaryChanged();
}

} // namespace dante
