// Application-wide event bus.
// Singleton QObject; emits typed signals so other layers (QML, services)
// observe cross-cutting events without coupling.
#pragma once
#include <QObject>
#include <QString>

namespace dante::domain {

class EventBus : public QObject {
    Q_OBJECT
public:
    static EventBus& instance();

signals:
    // Terminal lifecycle
    void terminalSpawned(const QString& sessionId);
    void terminalExited(const QString& sessionId, int code);
    void terminalOutput(const QString& sessionId, const QString& chunk);
    void terminalCwdChanged(const QString& sessionId, const QString& cwd);
    void terminalTitleChanged(const QString& sessionId, const QString& title);

    // Tab actions (driven by QML)
    void newTabRequested();
    void closeTabRequested(const QString& tabId);
    void selectTabRequested(const QString& tabId);

    // Cross-cutting injection
    void injectTextRequested(const QString& text);

private:
    EventBus() = default;
};

} // namespace dante::domain
