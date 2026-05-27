# SPEC-160 — i18n consistente (PT-BR + EN with .ts files)

Ships two Qt Linguist source catalogs (`i18n/dante_pt_BR.ts`,
`i18n/dante_en.ts`), a small `dante::i18n::Translator` QObject that owns the
active `QTranslator`, and the AppState wiring needed to expose the choice in
the **Geral** tab of the Settings panel.

Files added by this SPEC (already on disk):

```
core/i18n/Translator.h
core/i18n/Translator.cpp
i18n/dante_pt_BR.ts          (272 messages across 30 contexts)
i18n/dante_en.ts             (272 messages, every visible string mapped)
```

Everything below is the **integration patch** — apply by hand to the listed
files. No build was attempted from inside the SPEC.

---

## 1. CMakeLists.txt

### Root `CMakeLists.txt`
Add Qt6::LinguistTools alongside the existing Qt component imports and hand
the two `.ts` files to `qt_add_translations` once the `dante-cli` target is
defined:

```cmake
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick Qml LinguistTools ...)

# ... after the dante-cli executable target is fully populated ...
qt_add_translations(dante-cli
    TS_FILES
        ${CMAKE_SOURCE_DIR}/i18n/dante_pt_BR.ts
        ${CMAKE_SOURCE_DIR}/i18n/dante_en.ts
    QM_FILES_OUTPUT_VARIABLE dante_qm_files
    LUPDATE_OPTIONS -no-obsolete
    RESOURCE_PREFIX /i18n
)
```

`RESOURCE_PREFIX /i18n` mirrors the path our Translator uses
(`load("dante_<locale>", ":/i18n")`).

### `core/CMakeLists.txt`
Append the new sources to the `dante-core` static library:

```cmake
i18n/Translator.cpp
i18n/Translator.h
```

`target_include_directories(dante-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})`
already lets consumers `#include "i18n/Translator.h"`.

---

## 2. `apps/desktop-qt/src/main.cpp`

After `QApplication`/`QGuiApplication` is constructed and **before**
`engine.load(...)`:

```cpp
#include "i18n/Translator.h"
// ...
auto* tr = new dante::i18n::Translator(&app);
tr->setLanguage(appState->uiLanguage().isEmpty() ? "system" : appState->uiLanguage());
engine.rootContext()->setContextProperty("translator", tr);

// Re-load translations on language change. Simplest robust path is to ask the
// QML engine to retranslate; the empty-event broadcast picks up every qsTr().
QObject::connect(tr, &dante::i18n::Translator::languageChanged,
                 &engine, [&engine]() { engine.retranslate(); });

// Bridge AppState → Translator. settingsChanged is the bulk-write signal.
QObject::connect(appState, &dante::AppState::settingsChanged,
                 tr, [tr, appState]() { tr->setLanguage(appState->uiLanguage()); });
```

`engine.retranslate()` (Qt 6.6+) re-evaluates every `qsTr()` binding in QML.
On earlier Qt versions, recreate the engine (`engine.clearComponentCache()`
then reload the root QML file).

---

## 3. `core/domain/Models.h` — `AppSettings`

Add one field, default to `"system"` so first-run honours the OS language:

```cpp
struct AppSettings {
    // ... existing fields ...
    QString uiLanguage{"system"};  // SPEC-160: "pt_BR" | "en" | "system"
};
```

If `AppSettings` has explicit serialization helpers (e.g.
`toJson`/`fromJson`), add the new key there too:

```cpp
// toJson:
obj.insert("uiLanguage", uiLanguage);
// fromJson:
uiLanguage = obj.value("uiLanguage").toString("system");
```

---

## 4. `apps/desktop-qt/src/AppState.{h,cpp}`

### Header
```cpp
class AppState : public QObject {
    Q_OBJECT
    // ... existing properties ...
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY settingsChanged)

public:
    QString uiLanguage() const { return settings_.uiLanguage; }
    void setUiLanguage(const QString& locale);
    // ...
};
```

### Implementation
```cpp
void AppState::setUiLanguage(const QString& locale) {
    if (settings_.uiLanguage == locale) return;
    settings_.uiLanguage = locale;
    persistSettings();             // existing helper that writes settings.json
    emit settingsChanged();
}
```

`settingsChanged` is already wired to `main.cpp`'s Translator bridge above,
so flipping this property is the only hop the UI needs.

---

## 5. `ui/qml/SettingsPanel.qml` — Geral tab

Inside the "Geral" `Column`/`GridLayout`, drop a new row:

```qml
RowLayout {
    Layout.fillWidth: true
    Label {
        text: qsTr("Idioma")
        Layout.preferredWidth: 180
    }
    ComboBox {
        id: languageCombo
        Layout.fillWidth: true
        textRole: "label"
        valueRole: "code"
        model: [
            { label: qsTr("Sistema"), code: "system" },
            { label: qsTr("Português"), code: "pt_BR" },
            { label: qsTr("English"), code: "en" },
        ]
        Component.onCompleted: {
            const code = appState.uiLanguage || "system";
            const idx = model.findIndex(it => it.code === code);
            currentIndex = Math.max(0, idx);
        }
        onActivated: appState.uiLanguage = model[currentIndex].code
    }
}
```

The combo binds **one-way** out to `appState.uiLanguage`; the Translator
listens on `settingsChanged` and `engine.retranslate()` redraws the QML tree.

---

## 6. Resources / .qm files

`qt_add_translations(... RESOURCE_PREFIX /i18n)` makes Qt embed compiled
`dante_pt_BR.qm` and `dante_en.qm` at `:/i18n/dante_<locale>.qm`. That is
exactly the path `Translator::installTranslator()` passes to
`QTranslator::load(name, dir)`. **No manual .qrc edit is required.**

---

## 7. Verification

Smoke test once the patch lands:

1. Launch the app on a system with locale `en_US`. First-run picks
   `uiLanguage="system"`, the Translator resolves to `en`, the menu bar,
   tooltips, status bar and Settings panel should all read English.
2. Switch the Idioma combo to **Português** → strings flip to PT-BR
   immediately (no app restart).
3. Switch to **Sistema** on a `pt_BR.UTF-8` machine → falls back to PT-BR.
4. Force a missing `.qm` (rename `dante_en.qm` in `:/i18n` for a manual test):
   the Translator should still install an empty translator and the UI should
   stay in PT-BR source (never crash, never blank).
5. Run `lupdate` once a quarter to refresh the `.ts` files as new `qsTr()`
   calls land; commit the diff.

---

## 8. Notes

- `dante_pt_BR.ts` lists each source string with itself as the translation.
  We keep it around so Linguist can show 100% completion for PT-BR and so any
  future PT-BR copy edits can be made centrally without touching QML.
- `dante_en.ts` covers all 272 source strings harvested today. Future
  additions go through `lupdate`; missing translations gracefully fall back
  to the PT-BR source thanks to the `type="unfinished"` convention.
- The Translator's locale resolution intentionally accepts only `pt_BR` and
  `en` to avoid surprising the user with half-translated experiences. Adding
  a third language is a 3-line change to `kSupportedLocales()` plus a new
  `dante_<locale>.ts`.
