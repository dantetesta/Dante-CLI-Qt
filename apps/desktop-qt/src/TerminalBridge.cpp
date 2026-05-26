#include "TerminalBridge.h"
#include "terminal/TerminalRegistry.h"
#include "terminal/TerminalSession.h"
#include "persistence/AppPaths.h"
#include <QStandardPaths>
#include <QDebug>

namespace dante {

TerminalBridge::TerminalBridge(QObject* parent) : QObject(parent) {}

void TerminalBridge::spawn(const QString& sessionId, const QString& cwd) {
    auto& reg = terminal::TerminalRegistry::instance();
    if (reg.find(sessionId)) {
        qInfo() << "[term] session already alive" << sessionId;
        return;
    }
    const QString home = cwd.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation) : cwd;
    auto* session = reg.spawn(sessionId, /*shell*/QString(), home, 80, 24);
    if (!session) return;
    connect(session, &terminal::TerminalSession::output, this,
            [this, sessionId](const QString& chunk) {
                emit outputReceived(sessionId, chunk);
            });
    connect(session, &terminal::TerminalSession::cwdChanged, this,
            [this, sessionId](const QString& p) { emit cwdChanged(sessionId, p); });
    connect(session, &terminal::TerminalSession::titleChanged, this,
            [this, sessionId](const QString& t) { emit titleChanged(sessionId, t); });
    connect(session, &terminal::TerminalSession::exited, this,
            [this, sessionId](int code) { emit sessionExited(sessionId, code); });
}

void TerminalBridge::write(const QString& sessionId, const QString& text) {
    if (auto* s = terminal::TerminalRegistry::instance().find(sessionId))
        s->write(text.toUtf8());
}
void TerminalBridge::resize(const QString& sessionId, int cols, int rows) {
    if (auto* s = terminal::TerminalRegistry::instance().find(sessionId))
        s->resize(cols, rows);
}
void TerminalBridge::kill(const QString& sessionId) {
    terminal::TerminalRegistry::instance().dispose(sessionId);
}

} // namespace dante
