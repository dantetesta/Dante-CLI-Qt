// QML-facing bridge to TerminalRegistry. Each terminal QML view calls
// `terminal.spawn(sessionId)` on its onCompleted, then receives output via
// the `outputReceived(id, chunk)` signal.
#pragma once
#include <QObject>
#include <QString>

namespace dante {

class TerminalBridge : public QObject {
    Q_OBJECT
public:
    explicit TerminalBridge(QObject* parent = nullptr);

    Q_INVOKABLE void spawn(const QString& sessionId, const QString& cwd);
    Q_INVOKABLE void write(const QString& sessionId, const QString& text);
    Q_INVOKABLE void resize(const QString& sessionId, int cols, int rows);
    Q_INVOKABLE void kill(const QString& sessionId);

signals:
    void outputReceived(const QString& sessionId, const QString& chunk);
    void cwdChanged(const QString& sessionId, const QString& cwd);
    void titleChanged(const QString& sessionId, const QString& title);
    void sessionExited(const QString& sessionId, int code);
};

} // namespace dante
