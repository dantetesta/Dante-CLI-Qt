#pragma once

#include <QString>
#include <QStringList>
#include <deque>
#include <optional>

namespace dante::calculator {

struct EvalResult {
    bool    ok{false};
    double  value{0.0};
    QString error;
};

struct HistoryEntry {
    QString expression;
    double  value{0.0};
};

class CalculatorEngine {
public:
    CalculatorEngine();

    EvalResult evaluate(const QString& expression);

    double memory() const { return memory_; }
    void   memoryAdd(double v) { memory_ += v; }
    void   memorySub(double v) { memory_ -= v; }
    void   memoryClear()       { memory_ = 0.0; }

    const std::deque<HistoryEntry>& history() const { return history_; }
    void clearHistory() { history_.clear(); }
    void pushHistory(const QString& expr, double value);
    // Rehydrate the ring buffer (newest last). Caller is responsible for cap.
    void setHistory(std::deque<HistoryEntry> entries);

    static constexpr int kHistoryCap = 50;

private:
    double  memory_{0.0};
    std::deque<HistoryEntry> history_;
};

} // namespace dante::calculator
