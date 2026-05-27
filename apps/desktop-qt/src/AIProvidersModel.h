// QML-facing list model around `dante::ai::AIProviderStore`. Owns the
// JsonStore for ai-providers.json + tracks the user's "default chat" /
// "default whisper" picks (both serialized into the same file so this
// model is self-contained — no AppState additions needed).
#pragma once

#include "ai/AIProviderStore.h"
#include "persistence/JsonStore.h"

#include <QAbstractListModel>
#include <QString>
#include <QTimer>
#include <memory>

namespace dante {

class AIProvidersModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString defaultChatProviderId    READ defaultChatProviderId    WRITE pickDefaultChat    NOTIFY defaultsChanged)
    Q_PROPERTY(QString defaultWhisperProviderId READ defaultWhisperProviderId WRITE pickDefaultWhisper NOTIFY defaultsChanged)
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        BaseUrlRole,
        ModelRole,
        KindRole,
        EnabledRole,
        HasApiKeyRole,
        IsDefaultChatRole,
        IsDefaultWhisperRole,
    };

    explicit AIProvidersModel(QObject* parent = nullptr);

    /// Read ai-providers.json from disk + repopulate the model. Safe to
    /// call repeatedly; emits modelReset.
    void hydrate();

    /* QAbstractListModel */
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /* QML-callable mutators */

    /// Append a fresh blank provider (UUID assigned). Returns the new id
    /// so the QML editor can immediately select it.
    Q_INVOKABLE QString addProvider();

    /// Patch fields of an existing provider. Empty/no-op when id unknown.
    Q_INVOKABLE void updateProvider(const QString& id,
                                    const QString& name,
                                    const QString& baseUrl,
                                    const QString& apiKey,
                                    const QString& model,
                                    const QString& kind,
                                    bool enabled);

    Q_INVOKABLE void removeProvider(const QString& id);

    /// Return the full provider record (or empty map). Used by the editor
    /// when entering "edit" mode so it can populate every field including
    /// the secret one.
    Q_INVOKABLE QVariantMap get(const QString& id) const;

    /// Mark `id` as the default chat / whisper provider. Empty id clears
    /// the pick. Persists via the same debounced JsonStore.
    Q_INVOKABLE void pickDefaultChat(const QString& id);
    Q_INVOKABLE void pickDefaultWhisper(const QString& id);

    QString defaultChatProviderId()    const { return defaultChatId_; }
    QString defaultWhisperProviderId() const { return defaultWhisperId_; }

    /// Look up a provider by id (returns default-constructed AIProvider
    /// when absent). Wired to AIController so chat requests can resolve
    /// the active provider's baseUrl / model / apiKey.
    ai::AIProvider providerById(const QString& id) const { return store_.find(id); }

signals:
    void defaultsChanged();

private:
    void schedulePersist();
    void persistNow();
    int  indexOf(const QString& id) const;
    QVariant roleValueFor(const ai::AIProvider& p, int role) const;

    ai::AIProviderStore                     store_;
    std::unique_ptr<persistence::JsonStore> file_;
    QTimer                                  debounce_;
    QString                                 defaultChatId_;
    QString                                 defaultWhisperId_;
};

} // namespace dante
