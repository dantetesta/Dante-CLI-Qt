#include "TerminalRegistry.h"
#include <QDebug>

namespace dante::terminal {

TerminalRegistry& TerminalRegistry::instance() {
    static TerminalRegistry r;
    return r;
}

TerminalSession* TerminalRegistry::spawn(const QString& sessionId,
                                        const QString& shell,
                                        const QString& cwd,
                                        int cols, int rows) {
    auto s = std::make_unique<TerminalSession>(sessionId);
    if (!s->start(shell, cwd, cols, rows)) {
        qWarning() << "[term] spawn failed:" << s->errorString();
        return nullptr;
    }
    auto* raw = s.get();
    map_.emplace(sessionId, std::move(s));
    return raw;
}

TerminalSession* TerminalRegistry::find(const QString& sessionId) const {
    const auto it = map_.find(sessionId);
    if (it == map_.end()) return nullptr;
    return it->second.get();
}

void TerminalRegistry::dispose(const QString& sessionId) {
    const auto it = map_.find(sessionId);
    if (it == map_.end()) return;
    it->second->kill();
    map_.erase(it);
}

} // namespace dante::terminal
