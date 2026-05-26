// Map session-id → TerminalSession. Singleton.
#pragma once

#include "TerminalSession.h"
#include <QHash>
#include <QObject>
#include <memory>

namespace dante::terminal {

class TerminalRegistry : public QObject {
    Q_OBJECT
public:
    static TerminalRegistry& instance();

    TerminalSession* spawn(const QString& sessionId,
                           const QString& shell,
                           const QString& cwd,
                           int cols, int rows);
    TerminalSession* find(const QString& sessionId) const;
    void dispose(const QString& sessionId);

private:
    TerminalRegistry() = default;
    QHash<QString, std::unique_ptr<TerminalSession>> map_;
};

} // namespace dante::terminal
