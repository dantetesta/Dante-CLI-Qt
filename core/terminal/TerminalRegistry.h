// Map session-id → TerminalSession. Singleton.
#pragma once

#include "TerminalSession.h"
#include <QObject>
#include <QString>
#include <memory>
#include <unordered_map>

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
    // std::unordered_map (not QHash): QHash requires copyable values; we hold
    // unique_ptr to enforce single ownership of each PTY session.
    struct QStringHash { size_t operator()(const QString& s) const noexcept { return qHash(s); } };
    std::unordered_map<QString, std::unique_ptr<TerminalSession>, QStringHash> map_;
};

} // namespace dante::terminal
