// QML-facing controller for Claude resource lists.
//
// Owns a ClaudeResourceScanner and exposes three QAbstractListModels
// (skills, agents, mcps) plus invoke* helpers that emit the slash-
// command form to the active terminal.
#pragma once

#include "claude/ClaudeResources.h"
#include "claude/ClaudeResourceScanner.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

#include <memory>

namespace dante {

class ResourcesController;

// ---- Internal models ------------------------------------------------

class SkillsListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        ScopeRole,
        FilePathRole,
    };
    explicit SkillsListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void replace(const QList<dante::claude::Skill>& items);
    QString nameAt(int row) const;

private:
    QList<dante::claude::Skill> items_;
};

class AgentsListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        ScopeRole,
        FilePathRole,
        ToolsRole,
        ModelRole,
    };
    explicit AgentsListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void replace(const QList<dante::claude::Agent>& items);
    QString nameAt(int row) const;

private:
    QList<dante::claude::Agent> items_;
};

class McpsListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        SummaryRole,
        ScopeRole,
        SourcePathRole,
    };
    explicit McpsListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void replace(const QList<dante::claude::Mcp>& items);
    QString nameAt(int row) const;

private:
    QList<dante::claude::Mcp> items_;
};

// ---- Controller -----------------------------------------------------

class ResourcesController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel* skillsModel READ skillsModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* agentsModel READ agentsModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* mcpsModel   READ mcpsModel   CONSTANT)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
public:
    explicit ResourcesController(QObject* parent = nullptr);
    ~ResourcesController() override;

    QAbstractListModel* skillsModel() const { return skillsModel_; }
    QAbstractListModel* agentsModel() const { return agentsModel_; }
    QAbstractListModel* mcpsModel()   const { return mcpsModel_; }
    bool scanning() const { return scanning_; }

    // Kick off the first scan. main.cpp calls this after construction
    // so the watcher arms after the engine context is in place.
    Q_INVOKABLE void hydrate();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setProjectCwd(const QString& cwd);

    // Slash-command insertion helpers. The Swift sibling injects the
    // form `/<name> ` (trailing space) so the user can keep typing
    // arguments before hitting Enter — we match it.
    Q_INVOKABLE void invokeSkill(const QString& name);
    Q_INVOKABLE void invokeAgent(const QString& name);
    Q_INVOKABLE void invokeMcp(const QString& name);

signals:
    void requestTerminalWrite(const QString& text);
    void scanningChanged();

private:
    SkillsListModel* skillsModel_ = nullptr;
    AgentsListModel* agentsModel_ = nullptr;
    McpsListModel*   mcpsModel_   = nullptr;
    std::unique_ptr<dante::claude::ClaudeResourceScanner> scanner_;
    bool scanning_ = false;
};

} // namespace dante
