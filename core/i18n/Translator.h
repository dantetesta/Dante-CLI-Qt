// SPEC-160 — Runtime translator for Dante CLI Qt.
//
// Owns a single QTranslator instance and lets the rest of the app switch the
// active locale at runtime. The catalogs (.qm) are produced by
// `qt_add_translations` from i18n/dante_<locale>.ts and shipped inside the
// Qt resource system under the prefix ":/i18n/".
//
// Supported locales today:
//   • "pt_BR" — source language; loading still installs an empty translator
//               so all qsTr() calls resolve cleanly.
//   • "en"    — English fallback.
//   • "system" — match QLocale::system().name(); falls back to pt_BR if the
//               system language is not on the supported list.
//
// The fallback strategy is intentionally lax: if a .qm cannot be loaded the
// installed translator stays empty and qsTr() returns the source PT-BR
// strings — never crashes the app.
#pragma once

#include <QObject>
#include <QString>

class QTranslator;
class QCoreApplication;

namespace dante::i18n {

class Translator : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentLocale READ currentLocale NOTIFY languageChanged)

public:
    explicit Translator(QObject* parent = nullptr);
    ~Translator() override;

    // Resolved BCP-47 locale name actually loaded (e.g. "pt_BR" or "en").
    QString currentLocale() const { return currentLocale_; }

    // Returns the list of locales for which a .qm catalog is available.
    Q_INVOKABLE QStringList supportedLocales() const;

    // Returns the resolved locale that would be used for the "system" pseudo-locale.
    Q_INVOKABLE QString resolveSystemLocale() const;

public slots:
    // Accepted values: "pt_BR", "en", "system". Anything else falls back to "pt_BR".
    // Emits languageChanged when the active translator changes.
    void setLanguage(const QString& locale);

signals:
    void languageChanged(const QString& locale);

private:
    bool installTranslator(const QString& locale);

    QTranslator* translator_{nullptr};
    QString currentLocale_;
};

}  // namespace dante::i18n
