// SPEC-160 — see Translator.h.
#include "Translator.h"

#include <QCoreApplication>
#include <QLocale>
#include <QStringList>
#include <QTranslator>

namespace dante::i18n {

namespace {

// Keep this list in sync with i18n/dante_<locale>.ts in the repo root.
const QStringList& kSupportedLocales() {
    static const QStringList kList = {QStringLiteral("pt_BR"), QStringLiteral("en")};
    return kList;
}

// Resolve "system" → a concrete supported locale. We accept either an exact
// match ("pt_BR") or a language-only match ("pt" → first pt_* in the list).
// Anything else falls back to pt_BR (the source language).
QString resolveLocale(const QString& requested) {
    if (requested.isEmpty() || requested == QStringLiteral("system")) {
        const QString system = QLocale::system().name();  // e.g. "pt_BR", "en_US"
        if (kSupportedLocales().contains(system))
            return system;
        const QString lang = system.section(QLatin1Char('_'), 0, 0);
        for (const QString& candidate : kSupportedLocales()) {
            if (candidate.startsWith(lang + QLatin1Char('_')) || candidate == lang)
                return candidate;
        }
        return QStringLiteral("pt_BR");
    }
    if (kSupportedLocales().contains(requested))
        return requested;
    return QStringLiteral("pt_BR");
}

}  // namespace

Translator::Translator(QObject* parent) : QObject(parent) {}

Translator::~Translator() {
    if (translator_ && QCoreApplication::instance()) {
        QCoreApplication::removeTranslator(translator_);
    }
    // translator_ is parented to `this` (or deleted manually); QObject teardown handles it.
}

QStringList Translator::supportedLocales() const {
    return kSupportedLocales();
}

QString Translator::resolveSystemLocale() const {
    return resolveLocale(QStringLiteral("system"));
}

void Translator::setLanguage(const QString& locale) {
    const QString resolved = resolveLocale(locale);
    if (resolved == currentLocale_ && translator_)
        return;
    if (installTranslator(resolved)) {
        currentLocale_ = resolved;
        emit languageChanged(resolved);
    }
}

bool Translator::installTranslator(const QString& locale) {
    auto* app = QCoreApplication::instance();
    if (!app)
        return false;

    auto* next = new QTranslator(this);
    // Catalogs live in the Qt resource system under :/i18n/, produced by
    // qt_add_translations(TS_FILES i18n/dante_<locale>.ts) → dante_<locale>.qm.
    const QString file = QStringLiteral("dante_%1").arg(locale);
    const bool loaded = next->load(file, QStringLiteral(":/i18n"));
    // Even if `loaded` is false (e.g. for pt_BR which equals source language),
    // installing an empty translator keeps the API consistent.
    if (translator_) {
        QCoreApplication::removeTranslator(translator_);
        translator_->deleteLater();
        translator_ = nullptr;
    }
    QCoreApplication::installTranslator(next);
    translator_ = next;
    // Returning true even when `loaded` is false is intentional: an empty
    // translator is still a valid installed state — qsTr() simply returns the
    // source PT-BR string. The `loaded` value is kept around for future
    // diagnostics / logging if the caller wants to know.
    (void)loaded;
    return true;
}

}  // namespace dante::i18n
