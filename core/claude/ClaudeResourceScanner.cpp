#include "ClaudeResourceScanner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>

namespace dante::claude {

namespace {

QString homePath() {
    return QDir::homePath();
}

QString homeSubpath(const QString& sub) {
    return QDir::cleanPath(homePath() + QDir::separator() + sub);
}

QString cwdSubpath(const QString& cwd, const QString& sub) {
    return QDir::cleanPath(cwd + QDir::separator() + sub);
}

} // namespace

// ────────────────────────────────────────────────────────────────────
// YamlFrontmatter — minimal parser, mirrors Swift sibling's behavior.
// ────────────────────────────────────────────────────────────────────

QHash<QString, QString> YamlFrontmatter::parse(const QString& content) {
    QHash<QString, QString> result;
    const QStringList lines = content.split(QChar('\n'));
    if (lines.isEmpty() || lines.first().trimmed() != QStringLiteral("---"))
        return result;
    int endIndex = lines.size();
    for (int i = 1; i < lines.size(); ++i) {
        if (lines.at(i).trimmed() == QStringLiteral("---")) {
            endIndex = i;
            break;
        }
    }
    int i = 1;
    while (i < endIndex) {
        const QString line = lines.at(i);
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) { ++i; continue; }
        // Only top-level keys (no leading whitespace).
        const bool indented = line.startsWith(QChar(' ')) || line.startsWith(QChar('\t'));
        const int colon = line.indexOf(QChar(':'));
        if (indented || colon <= 0) { ++i; continue; }
        const QString key = line.left(colon).trimmed();
        QString rest = line.mid(colon + 1).trimmed();
        // Block scalar.
        if (rest == QStringLiteral("|") || rest == QStringLiteral(">")) {
            QStringList block;
            ++i;
            while (i < endIndex) {
                const QString next = lines.at(i);
                if (next.isEmpty()) { block.append(QString()); ++i; continue; }
                if (next.startsWith(QChar(' ')) || next.startsWith(QChar('\t'))) {
                    block.append(next.trimmed());
                    ++i;
                } else {
                    break;
                }
            }
            result.insert(key, block.join(QChar(' ')).trimmed());
            continue;
        }
        // Strip surrounding quotes.
        if (rest.size() >= 2 &&
            ((rest.startsWith(QChar('"')) && rest.endsWith(QChar('"'))) ||
             (rest.startsWith(QChar('\'')) && rest.endsWith(QChar('\''))))) {
            rest = rest.mid(1, rest.size() - 2);
        }
        result.insert(key, rest);
        ++i;
    }
    return result;
}

// ────────────────────────────────────────────────────────────────────
// ClaudeResourceScanner
// ────────────────────────────────────────────────────────────────────

ClaudeResourceScanner::ClaudeResourceScanner(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<QList<Skill>>("QList<dante::claude::Skill>");
    qRegisterMetaType<QList<Agent>>("QList<dante::claude::Agent>");
    qRegisterMetaType<QList<Mcp>>("QList<dante::claude::Mcp>");

    debounce_.setSingleShot(true);
    debounce_.setInterval(300);
    connect(&debounce_, &QTimer::timeout, this,
            &ClaudeResourceScanner::rescanImmediate);
    connect(&watcher_, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString&) { debounce_.start(); });
    connect(&watcher_, &QFileSystemWatcher::fileChanged,
            this, [this](const QString&) { debounce_.start(); });

    rearmWatcher();
}

ClaudeResourceScanner::~ClaudeResourceScanner() = default;

void ClaudeResourceScanner::setProjectCwd(const QString& cwd) {
    if (projectCwd_ == cwd) return;
    projectCwd_ = cwd;
    rearmWatcher();
    rescan();
}

void ClaudeResourceScanner::rescan() {
    // Coalesce calls — the debounce timer fires once 300ms after the
    // last poke, then rescanImmediate() runs the actual work.
    debounce_.start();
}

void ClaudeResourceScanner::rearmWatcher() {
    const QStringList previous =
        watcher_.directories() + watcher_.files();
    if (!previous.isEmpty()) watcher_.removePaths(previous);

    QStringList targets;
    targets << homeSubpath(QStringLiteral(".claude/skills"))
            << homeSubpath(QStringLiteral(".claude/agents"));
    const QString mcpFile = homeSubpath(QStringLiteral(".mcp.json"));
    if (QFileInfo::exists(mcpFile)) targets << mcpFile;
    if (!projectCwd_.isEmpty()) {
        targets << cwdSubpath(projectCwd_, QStringLiteral(".claude/skills"));
        targets << cwdSubpath(projectCwd_, QStringLiteral(".claude/agents"));
        const QString projMcp = cwdSubpath(projectCwd_, QStringLiteral(".mcp.json"));
        if (QFileInfo::exists(projMcp)) targets << projMcp;
    }
    for (const QString& p : targets) {
        if (QFileInfo::exists(p)) watcher_.addPath(p);
    }
}

void ClaudeResourceScanner::rescanImmediate() {
    if (scanning_) {
        // Queue another scan after the in-flight one. The debounce
        // re-fires when a watcher event lands during the run.
        return;
    }
    scanning_ = true;
    emit scanningChanged(true);

    const QString cwd = projectCwd_;

    auto* watcher = new QFutureWatcher<void>(this);
    QFuture<void> fut = QtConcurrent::run([this, cwd] {
        QList<Skill> skills;
        skills.append(scanSkillsDir(
            homeSubpath(QStringLiteral(".claude/skills")), ResourceScope::User));
        if (!cwd.isEmpty()) {
            skills.append(scanSkillsDir(
                cwdSubpath(cwd, QStringLiteral(".claude/skills")),
                ResourceScope::Project));
        }
        std::sort(skills.begin(), skills.end(),
                  [](const Skill& a, const Skill& b) {
                      return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                  });

        QList<Agent> agents;
        agents.append(scanAgentsDir(
            homeSubpath(QStringLiteral(".claude/agents")), ResourceScope::User));
        if (!cwd.isEmpty()) {
            agents.append(scanAgentsDir(
                cwdSubpath(cwd, QStringLiteral(".claude/agents")),
                ResourceScope::Project));
        }
        std::sort(agents.begin(), agents.end(),
                  [](const Agent& a, const Agent& b) {
                      return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                  });

        QList<Mcp> mcps;
        mcps.append(scanMcpsFile(
            homeSubpath(QStringLiteral(".mcp.json")), ResourceScope::User));
        if (!cwd.isEmpty()) {
            mcps.append(scanMcpsFile(
                cwdSubpath(cwd, QStringLiteral(".mcp.json")),
                ResourceScope::Project));
        }
        std::sort(mcps.begin(), mcps.end(),
                  [](const Mcp& a, const Mcp& b) {
                      return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                  });

        // Hop back to the UI thread via the parent's event loop.
        QMetaObject::invokeMethod(this, [this, skills, agents, mcps] {
            emit skillsLoaded(skills);
            emit agentsLoaded(agents);
            emit mcpsLoaded(mcps);
            scanning_ = false;
            emit scanningChanged(false);
        }, Qt::QueuedConnection);
    });
    connect(watcher, &QFutureWatcher<void>::finished,
            watcher, &QFutureWatcher<void>::deleteLater);
    watcher->setFuture(fut);
}

// ────────────────────────────────────────────────────────────────────
// Static workers
// ────────────────────────────────────────────────────────────────────

QList<Skill> ClaudeResourceScanner::scanSkillsDir(const QString& path,
                                                   ResourceScope scope) {
    QList<Skill> out;
    QDir dir(path);
    if (!dir.exists()) return out;
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    const QStringList candidates =
        {QStringLiteral("SKILL.md"), QStringLiteral("skill.md"),
         QStringLiteral("Skill.md")};
    for (const QString& entry : entries) {
        if (entry.startsWith(QChar('.'))) continue;
        const QString dirPath = QDir::cleanPath(path + QDir::separator() + entry);
        for (const QString& candidate : candidates) {
            const QString fp = QDir::cleanPath(dirPath + QDir::separator() + candidate);
            QFile f(fp);
            if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            const QString content = QString::fromUtf8(f.readAll());
            f.close();
            const auto yaml = YamlFrontmatter::parse(content);
            Skill s;
            s.name = yaml.value(QStringLiteral("name"), entry);
            s.description = yaml.value(QStringLiteral("description"));
            s.scope = scope;
            s.filePath = fp;
            for (auto it = yaml.constBegin(); it != yaml.constEnd(); ++it) {
                if (it.key() == QStringLiteral("name") ||
                    it.key() == QStringLiteral("description")) continue;
                s.extras.insert(it.key(), it.value());
            }
            out.append(s);
            break;
        }
    }
    return out;
}

QList<Agent> ClaudeResourceScanner::scanAgentsDir(const QString& path,
                                                   ResourceScope scope) {
    QList<Agent> out;
    QDir dir(path);
    if (!dir.exists()) return out;
    const QStringList entries =
        dir.entryList(QStringList{QStringLiteral("*.md")},
                      QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.startsWith(QChar('.'))) continue;
        const QString fp = QDir::cleanPath(path + QDir::separator() + entry);
        QFile f(fp);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        const QString content = QString::fromUtf8(f.readAll());
        f.close();
        const auto yaml = YamlFrontmatter::parse(content);
        Agent a;
        const QString stem = QFileInfo(entry).completeBaseName();
        a.name = yaml.value(QStringLiteral("name"), stem);
        a.description = yaml.value(QStringLiteral("description"));
        a.scope = scope;
        a.filePath = fp;
        a.tools = yaml.value(QStringLiteral("tools"));
        a.model = yaml.value(QStringLiteral("model"));
        out.append(a);
    }
    return out;
}

QList<Mcp> ClaudeResourceScanner::scanMcpsFile(const QString& path,
                                                ResourceScope scope) {
    QList<Mcp> out;
    QFile f(path);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return out;
    const QByteArray bytes = f.readAll();
    f.close();
    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return out;
    const auto root = doc.object();
    const auto serversVal = root.value(QStringLiteral("mcpServers"));
    if (!serversVal.isObject()) return out;
    const auto servers = serversVal.toObject();
    for (auto it = servers.constBegin(); it != servers.constEnd(); ++it) {
        if (!it.value().isObject()) continue;
        const auto entry = it.value().toObject();
        Mcp m;
        m.name = it.key();
        m.scope = scope;
        m.command = entry.value(QStringLiteral("command")).toString();
        const auto argsVal = entry.value(QStringLiteral("args"));
        if (argsVal.isArray()) {
            for (const auto& a : argsVal.toArray()) m.args << a.toString();
        }
        m.url = entry.value(QStringLiteral("url")).toString();
        m.transport = entry.value(QStringLiteral("transport")).toString();
        m.sourcePath = path;
        out.append(m);
    }
    return out;
}

} // namespace dante::claude
