// Process stats indicator — QML-facing wrapper around
// `dante::system::ProcessStatsSampler`. Polls the current process's CPU%
// and RSS on a QTimer (1 s default) and exposes them as Q_PROPERTYs so the
// BottomToolbar can show a discreet "CPU 12% · 142 MB" pill.
//
// Mirrors the Swift sibling's `ProcessStats` for the app-process slice
// (we don't track child shell trees here — TerminalRegistry will get its
// own per-shell stats later).
//
// Jitter guard: we only emit *Changed signals when the new value moves more
// than 0.5% (CPU) or 1 MB (memory) from the last published value. Keeps the
// indicator text from flickering on every micro-tick.
#pragma once

#include <QObject>
#include <QString>
#include <memory>

class QTimer;

namespace dante::system { class ProcessStatsSampler; }

namespace dante {

class ProcessStatsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(double  cpuPercent       READ cpuPercent       NOTIFY cpuPercentChanged)
    Q_PROPERTY(double  memoryMB         READ memoryMB         NOTIFY memoryMBChanged)
    Q_PROPERTY(QString formattedSummary READ formattedSummary NOTIFY formattedSummaryChanged)
    Q_PROPERTY(QString tooltipText      READ tooltipText      NOTIFY formattedSummaryChanged)
    Q_PROPERTY(int     samplingInterval READ samplingInterval WRITE setSamplingInterval NOTIFY samplingIntervalChanged)
public:
    explicit ProcessStatsController(QObject* parent = nullptr);
    ~ProcessStatsController() override;

    double  cpuPercent()       const { return cpuPercent_; }
    double  memoryMB()         const { return memoryMB_; }
    QString formattedSummary() const { return formattedSummary_; }
    QString tooltipText()      const { return tooltipText_; }
    int     samplingInterval() const;

    Q_INVOKABLE void setSamplingInterval(int ms);

signals:
    void cpuPercentChanged();
    void memoryMBChanged();
    void formattedSummaryChanged();
    void samplingIntervalChanged();

private:
    void sample();
    void rebuildSummary();

    std::unique_ptr<system::ProcessStatsSampler> sampler_;
    QTimer* timer_{nullptr};

    double  cpuPercent_{0.0};
    double  memoryMB_{0.0};
    QString formattedSummary_;
    QString tooltipText_;
};

} // namespace dante
