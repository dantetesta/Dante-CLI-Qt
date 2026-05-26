// Command Palette controller — Cmd+K / Ctrl+K fuzzy-search modal.
//
// Exposed to QML as the `palette` context property. Owns:
//   • A static-ish catalog of commands (Command struct) — built once at
//     construction time from AppState/Favorites/Snippets/ThemeRegistry.
//   • The current query string and the filtered/scored result list.
//   • The selected row index (activeIndex) for keyboard navigation.
//
// Side effects are emitted as high-level signals (terminalWriteRequested,
// aiToggleRequested, voiceStartRequested, clearRequested) so this class
// stays loosely coupled to TerminalBridge / AIController. main.cpp wires
// those signals to the respective controllers via QObject::connect().
//
// Fuzzy matcher: subsequence + word-boundary/first-char bonus. No external
// library. See score() in the .cpp for the exact rule set.
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>
#include <functional>

namespace dante {

class AppState;
class FavoritesModel;
class SnippetsModel;

namespace themes { class ThemeRegistry; }

class PaletteController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool         open        READ open                                NOTIFY openChanged)
    Q_PROPERTY(QString      query       READ query       WRITE setQuery          NOTIFY queryChanged)
    Q_PROPERTY(QVariantList results     READ results                             NOTIFY resultsChanged)
    Q_PROPERTY(int          activeIndex READ activeIndex WRITE setActiveIndex    NOTIFY activeIndexChanged)
public:
    explicit PaletteController(AppState* appState,
                               FavoritesModel* favorites,
                               SnippetsModel* snippets,
                               themes::ThemeRegistry* schemes,
                               QObject* parent = nullptr);

    bool         open()        const { return open_; }
    QString      query()       const { return query_; }
    QVariantList results()     const { return results_; }
    int          activeIndex() const { return activeIndex_; }

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrev();
    Q_INVOKABLE void execute();
    Q_INVOKABLE void setQuery(QString q);
    Q_INVOKABLE void setActiveIndex(int i);

    /// Rebuild the dynamic portion of the catalog (favorites / snippets /
    /// themes). Cheap; called whenever those models notify a change.
    Q_INVOKABLE void rebuildCatalog();

signals:
    void openChanged();
    void queryChanged();
    void resultsChanged();
    void activeIndexChanged();

    /// High-level side-effect signals — main.cpp wires to controllers.
    void terminalWriteRequested(const QString& sessionId, const QString& text);
    void clearRequested(const QString& sessionId);
    void aiToggleRequested();
    void voiceStartRequested();
    void focusTerminalRequested();

private:
    struct Command {
        QString id;
        QString label;
        QString hint;
        QString category;
        QString shortcut;
        QString icon;        // emoji or single glyph (optional)
        std::function<void()> callback;
    };

    void buildStaticCatalog();
    void refilter();

    /// Fuzzy subsequence scorer. Returns INT_MIN when not a match.
    /// Otherwise: +10/char, +25 at word boundary, +50 first char,
    /// −1 per intervening non-matched char between two matches.
    static int score(const QString& query, const QString& label);

    void setOpen(bool v);

    AppState*               appState_   {nullptr};
    FavoritesModel*         favorites_  {nullptr};
    SnippetsModel*          snippets_   {nullptr};
    themes::ThemeRegistry*  schemes_    {nullptr};

    bool                    open_       {false};
    QString                 query_;
    QVector<Command>        catalog_;   // full set (static + dynamic)
    QVector<int>            filtered_;  // indices into catalog_, top 20
    QVariantList            results_;   // mirror of filtered_, ready for QML
    int                     activeIndex_{0};
};

} // namespace dante
