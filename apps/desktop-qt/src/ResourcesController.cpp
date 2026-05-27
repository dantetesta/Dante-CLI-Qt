#include "ResourcesController.h"

namespace dante {

using dante::claude::Agent;
using dante::claude::Mcp;
using dante::claude::ResourceScope;
using dante::claude::Skill;

// ────────────────────────── SkillsListModel ──────────────────────────

SkillsListModel::SkillsListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int SkillsListModel::rowCount(const QModelIndex& p) const {
    return p.isValid() ? 0 : items_.size();
}

QVariant SkillsListModel::data(const QModelIndex& idx, int role) const {
    if (idx.row() < 0 || idx.row() >= items_.size()) return {};
    const auto& s = items_.at(idx.row());
    switch (role) {
        case NameRole:        return s.name;
        case DescriptionRole: return s.description;
        case ScopeRole:       return dante::claude::scopeLabel(s.scope);
        case FilePathRole:    return s.filePath;
    }
    return {};
}

QHash<int, QByteArray> SkillsListModel::roleNames() const {
    return {
        {NameRole,        "name"},
        {DescriptionRole, "description"},
        {ScopeRole,       "scope"},
        {FilePathRole,    "filePath"},
    };
}

void SkillsListModel::replace(const QList<Skill>& items) {
    beginResetModel();
    items_ = items;
    endResetModel();
}

QString SkillsListModel::nameAt(int row) const {
    if (row < 0 || row >= items_.size()) return {};
    return items_.at(row).name;
}

// ────────────────────────── AgentsListModel ──────────────────────────

AgentsListModel::AgentsListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int AgentsListModel::rowCount(const QModelIndex& p) const {
    return p.isValid() ? 0 : items_.size();
}

QVariant AgentsListModel::data(const QModelIndex& idx, int role) const {
    if (idx.row() < 0 || idx.row() >= items_.size()) return {};
    const auto& a = items_.at(idx.row());
    switch (role) {
        case NameRole:        return a.name;
        case DescriptionRole: return a.description;
        case ScopeRole:       return dante::claude::scopeLabel(a.scope);
        case FilePathRole:    return a.filePath;
        case ToolsRole:       return a.tools;
        case ModelRole:       return a.model;
    }
    return {};
}

QHash<int, QByteArray> AgentsListModel::roleNames() const {
    return {
        {NameRole,        "name"},
        {DescriptionRole, "description"},
        {ScopeRole,       "scope"},
        {FilePathRole,    "filePath"},
        {ToolsRole,       "tools"},
        {ModelRole,       "model"},
    };
}

void AgentsListModel::replace(const QList<Agent>& items) {
    beginResetModel();
    items_ = items;
    endResetModel();
}

QString AgentsListModel::nameAt(int row) const {
    if (row < 0 || row >= items_.size()) return {};
    return items_.at(row).name;
}

// ─────────────────────────── McpsListModel ───────────────────────────

McpsListModel::McpsListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int McpsListModel::rowCount(const QModelIndex& p) const {
    return p.isValid() ? 0 : items_.size();
}

QVariant McpsListModel::data(const QModelIndex& idx, int role) const {
    if (idx.row() < 0 || idx.row() >= items_.size()) return {};
    const auto& m = items_.at(idx.row());
    switch (role) {
        case NameRole:       return m.name;
        case SummaryRole:    return m.summary();
        case ScopeRole:      return dante::claude::scopeLabel(m.scope);
        case SourcePathRole: return m.sourcePath;
    }
    return {};
}

QHash<int, QByteArray> McpsListModel::roleNames() const {
    return {
        {NameRole,       "name"},
        {SummaryRole,    "description"}, // mirror skills/agents key so QML reuses ResourceRow
        {ScopeRole,      "scope"},
        {SourcePathRole, "filePath"},
    };
}

void McpsListModel::replace(const QList<Mcp>& items) {
    beginResetModel();
    items_ = items;
    endResetModel();
}

QString McpsListModel::nameAt(int row) const {
    if (row < 0 || row >= items_.size()) return {};
    return items_.at(row).name;
}

// ────────────────────────── ResourcesController ──────────────────────

ResourcesController::ResourcesController(QObject* parent)
    : QObject(parent)
    , skillsModel_(new SkillsListModel(this))
    , agentsModel_(new AgentsListModel(this))
    , mcpsModel_(new McpsListModel(this))
    , scanner_(std::make_unique<dante::claude::ClaudeResourceScanner>(this)) {
    connect(scanner_.get(), &dante::claude::ClaudeResourceScanner::skillsLoaded,
            this, [this](const QList<Skill>& items) { skillsModel_->replace(items); });
    connect(scanner_.get(), &dante::claude::ClaudeResourceScanner::agentsLoaded,
            this, [this](const QList<Agent>& items) { agentsModel_->replace(items); });
    connect(scanner_.get(), &dante::claude::ClaudeResourceScanner::mcpsLoaded,
            this, [this](const QList<Mcp>& items) { mcpsModel_->replace(items); });
    connect(scanner_.get(), &dante::claude::ClaudeResourceScanner::scanningChanged,
            this, [this](bool s) {
                if (scanning_ == s) return;
                scanning_ = s;
                emit scanningChanged();
            });
}

ResourcesController::~ResourcesController() = default;

void ResourcesController::hydrate() {
    scanner_->rescanImmediate();
}

void ResourcesController::refresh() {
    scanner_->rescanImmediate();
}

void ResourcesController::setProjectCwd(const QString& cwd) {
    scanner_->setProjectCwd(cwd);
}

void ResourcesController::invokeSkill(const QString& name) {
    if (name.isEmpty()) return;
    emit requestTerminalWrite(QStringLiteral("/") + name + QStringLiteral(" "));
}

void ResourcesController::invokeAgent(const QString& name) {
    if (name.isEmpty()) return;
    // Agents in Claude Code aren't slash-commands — the user invokes
    // them by mentioning `@<name>`. Match the Swift sibling's chosen
    // injection form (slash, since that's what the user expects from
    // this sidebar's "Inserir" affordance).
    emit requestTerminalWrite(QStringLiteral("/") + name + QStringLiteral(" "));
}

void ResourcesController::invokeMcp(const QString& name) {
    if (name.isEmpty()) return;
    // MCPs aren't slash-invoked either; the row click copies the name
    // into the terminal as a reference the user can wrap with `@`.
    emit requestTerminalWrite(name);
}

} // namespace dante
