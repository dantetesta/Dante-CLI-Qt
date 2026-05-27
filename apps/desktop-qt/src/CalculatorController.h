#pragma once

#include "calculator/CalculatorEngine.h"

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

namespace dante {

class CalculatorHistoryModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        ExpressionRole = Qt::UserRole + 1,
        ValueRole,
        FormattedValueRole,
    };

    explicit CalculatorHistoryModel(calculator::CalculatorEngine* engine,
                                    QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Notifies QML that the engine's ring buffer changed under us.
    void refresh();

private:
    calculator::CalculatorEngine* engine_;
};

class CalculatorController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayText  READ displayText  WRITE setDisplay  NOTIFY displayTextChanged)
    Q_PROPERTY(QString lastError    READ lastError                       NOTIFY lastErrorChanged)
    Q_PROPERTY(double  memory       READ memory                          NOTIFY memoryChanged)
    Q_PROPERTY(QAbstractListModel* historyModel READ historyModel CONSTANT)

public:
    explicit CalculatorController(QObject* parent = nullptr);

    void hydrate();

    QString displayText() const { return display_; }
    QString lastError()   const { return lastError_; }
    double  memory()      const { return engine_->memory(); }
    QAbstractListModel* historyModel() const { return historyModel_.get(); }

    Q_INVOKABLE void setDisplay(const QString& text);
    Q_INVOKABLE void appendDigit(const QString& digit);
    Q_INVOKABLE void appendOperator(const QString& op);
    Q_INVOKABLE void evaluate();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void clearEntry();
    Q_INVOKABLE void backspace();
    Q_INVOKABLE void toggleSign();

    Q_INVOKABLE void memoryAdd();
    Q_INVOKABLE void memorySub();
    Q_INVOKABLE void memoryRecall();
    Q_INVOKABLE void memoryClear();

    Q_INVOKABLE void pickHistory(int row);
    Q_INVOKABLE void clearHistory();

    // Used by QML for rendering result rows consistently with the engine.
    Q_INVOKABLE QString formatNumber(double v) const;

signals:
    void displayTextChanged();
    void lastErrorChanged();
    void memoryChanged();

private:
    void scheduleSave();
    void saveNow();
    void loadFromDisk();

    std::unique_ptr<calculator::CalculatorEngine> engine_;
    std::unique_ptr<CalculatorHistoryModel>       historyModel_;
    QTimer  saveDebounce_;
    QString display_;
    QString lastError_;
    bool    justEvaluated_{false};
};

} // namespace dante
