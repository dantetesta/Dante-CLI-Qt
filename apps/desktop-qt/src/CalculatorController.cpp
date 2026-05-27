#include "CalculatorController.h"
#include "persistence/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <cmath>

namespace dante {

namespace {
    constexpr const char* kHistoryFile = "calculator-history.json";

    QString historyPath() {
        return persistence::file(QString::fromLatin1(kHistoryFile));
    }
}

// ─── CalculatorHistoryModel ─────────────────────────────────────────────────

CalculatorHistoryModel::CalculatorHistoryModel(calculator::CalculatorEngine* engine,
                                               QObject* parent)
    : QAbstractListModel(parent), engine_(engine) {}

int CalculatorHistoryModel::rowCount(const QModelIndex& p) const {
    return p.isValid() ? 0 : static_cast<int>(engine_->history().size());
}

QVariant CalculatorHistoryModel::data(const QModelIndex& idx, int role) const {
    const auto& h = engine_->history();
    const int n  = static_cast<int>(h.size());
    if (idx.row() < 0 || idx.row() >= n) return {};
    // Render newest-first.
    const auto& entry = h[n - 1 - idx.row()];
    switch (role) {
        case ExpressionRole:      return entry.expression;
        case ValueRole:           return entry.value;
        case FormattedValueRole: {
            // Compact double → decimal/scientific based on magnitude.
            double v = entry.value;
            if (v == 0.0 || (std::fabs(v) >= 1e-6 && std::fabs(v) < 1e15)) {
                QString s = QString::number(v, 'g', 12);
                return s;
            }
            return QString::number(v, 'g', 12);
        }
    }
    return {};
}

QHash<int, QByteArray> CalculatorHistoryModel::roleNames() const {
    return {
        {ExpressionRole,     "expression"},
        {ValueRole,          "value"},
        {FormattedValueRole, "formattedValue"},
    };
}

void CalculatorHistoryModel::refresh() {
    beginResetModel();
    endResetModel();
}

// ─── CalculatorController ───────────────────────────────────────────────────

CalculatorController::CalculatorController(QObject* parent)
    : QObject(parent)
    , engine_(std::make_unique<calculator::CalculatorEngine>())
    , historyModel_(std::make_unique<CalculatorHistoryModel>(engine_.get())) {
    saveDebounce_.setSingleShot(true);
    saveDebounce_.setInterval(500);
    connect(&saveDebounce_, &QTimer::timeout, this, &CalculatorController::saveNow);
}

void CalculatorController::hydrate() {
    loadFromDisk();
    historyModel_->refresh();
    emit memoryChanged();
}

void CalculatorController::setDisplay(const QString& text) {
    if (display_ == text) return;
    display_ = text;
    justEvaluated_ = false;
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    emit displayTextChanged();
}

void CalculatorController::appendDigit(const QString& digit) {
    if (justEvaluated_) {
        // Typing a digit after "=" starts a fresh expression.
        display_.clear();
        justEvaluated_ = false;
    }
    if (digit == QStringLiteral(".")) {
        // Avoid double dots in the current number token.
        int i = display_.size() - 1;
        while (i >= 0) {
            const QChar c = display_.at(i);
            if (c == QLatin1Char('.')) return;
            if (!c.isDigit()) break;
            --i;
        }
    }
    display_.append(digit);
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    emit displayTextChanged();
}

void CalculatorController::appendOperator(const QString& op) {
    if (op.isEmpty()) return;
    justEvaluated_ = false;
    // Replace trailing operator (except '(') so "5++" becomes "5+".
    if (!display_.isEmpty()) {
        const QChar last = display_.back();
        static const QString ops = QStringLiteral("+-*/^%");
        if (ops.contains(last)) {
            display_.chop(1);
        }
    }
    display_.append(op);
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    emit displayTextChanged();
}

void CalculatorController::evaluate() {
    if (display_.trimmed().isEmpty()) return;
    auto r = engine_->evaluate(display_);
    if (!r.ok) {
        lastError_ = r.error;
        emit lastErrorChanged();
        return;
    }
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    display_ = formatNumber(r.value);
    justEvaluated_ = true;
    emit displayTextChanged();
    historyModel_->refresh();
    scheduleSave();
}

void CalculatorController::clear() {
    display_.clear();
    justEvaluated_ = false;
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    emit displayTextChanged();
}

void CalculatorController::clearEntry() {
    // Drop the trailing number/identifier token; if none, clear all.
    int i = display_.size() - 1;
    while (i >= 0) {
        const QChar c = display_.at(i);
        if (c.isDigit() || c == QLatin1Char('.') || c.isLetter()) { --i; continue; }
        break;
    }
    if (i == display_.size() - 1) { clear(); return; }
    display_.truncate(i + 1);
    emit displayTextChanged();
}

void CalculatorController::backspace() {
    if (display_.isEmpty()) return;
    display_.chop(1);
    justEvaluated_ = false;
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    emit displayTextChanged();
}

void CalculatorController::toggleSign() {
    // If the display is a pure number, toggle in place. Otherwise wrap last
    // token: "5+3" → "5+-3"; "5+-3" → "5+3".
    if (display_.isEmpty()) { display_ = QStringLiteral("-"); emit displayTextChanged(); return; }
    bool numericAll = true;
    for (const QChar c : display_) {
        if (!(c.isDigit() || c == QLatin1Char('.') || c == QLatin1Char('-')
              || c == QLatin1Char('e') || c == QLatin1Char('E') || c == QLatin1Char('+'))) {
            numericAll = false; break;
        }
    }
    if (numericAll) {
        if (display_.startsWith(QLatin1Char('-'))) display_.remove(0, 1);
        else display_.prepend(QLatin1Char('-'));
        emit displayTextChanged();
        return;
    }
    // Find start of trailing numeric token.
    int i = display_.size() - 1;
    while (i >= 0 && (display_.at(i).isDigit() || display_.at(i) == QLatin1Char('.'))) --i;
    if (i >= 0 && display_.at(i) == QLatin1Char('-')) {
        display_.remove(i, 1);
    } else {
        display_.insert(i + 1, QLatin1Char('-'));
    }
    emit displayTextChanged();
}

void CalculatorController::memoryAdd() {
    auto r = engine_->evaluate(display_.isEmpty() ? QStringLiteral("0") : display_);
    if (!r.ok) { lastError_ = r.error; emit lastErrorChanged(); return; }
    engine_->memoryAdd(r.value);
    emit memoryChanged();
}

void CalculatorController::memorySub() {
    auto r = engine_->evaluate(display_.isEmpty() ? QStringLiteral("0") : display_);
    if (!r.ok) { lastError_ = r.error; emit lastErrorChanged(); return; }
    engine_->memorySub(r.value);
    emit memoryChanged();
}

void CalculatorController::memoryRecall() {
    display_ = formatNumber(engine_->memory());
    justEvaluated_ = false;
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    emit displayTextChanged();
}

void CalculatorController::memoryClear() {
    engine_->memoryClear();
    emit memoryChanged();
}

void CalculatorController::pickHistory(int row) {
    const auto& h = engine_->history();
    const int n = static_cast<int>(h.size());
    if (row < 0 || row >= n) return;
    // Display is sorted newest-first like the model.
    display_ = h[n - 1 - row].expression;
    justEvaluated_ = false;
    if (!lastError_.isEmpty()) { lastError_.clear(); emit lastErrorChanged(); }
    emit displayTextChanged();
}

void CalculatorController::clearHistory() {
    engine_->clearHistory();
    historyModel_->refresh();
    scheduleSave();
}

QString CalculatorController::formatNumber(double v) const {
    if (std::fabs(v - std::round(v)) < 1e-12 && std::fabs(v) < 1e15) {
        // Integer-valued doubles render without a trailing ".0".
        return QString::number(static_cast<long long>(std::llround(v)));
    }
    return QString::number(v, 'g', 12);
}

void CalculatorController::scheduleSave() {
    saveDebounce_.start();
}

void CalculatorController::saveNow() {
    const QString path = historyPath();
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QJsonArray entries;
    for (const auto& e : engine_->history()) {
        entries.append(QJsonObject{
            {"expression", e.expression},
            {"value",      e.value},
        });
    }
    QJsonObject root{
        {"version", 1},
        {"entries", entries},
    };

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    out.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    out.commit();
}

void CalculatorController::loadFromDisk() {
    QFile f(historyPath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return;
    const auto root = doc.object();
    const auto arr  = root.value("entries").toArray();
    std::deque<calculator::HistoryEntry> entries;
    for (const auto& v : arr) {
        const auto o = v.toObject();
        calculator::HistoryEntry e;
        e.expression = o.value("expression").toString();
        e.value      = o.value("value").toDouble();
        entries.push_back(std::move(e));
    }
    engine_->setHistory(std::move(entries));
}

} // namespace dante
