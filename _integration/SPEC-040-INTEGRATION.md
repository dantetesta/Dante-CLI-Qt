# SPEC-040 — AI Providers backend + Settings tab — Integration notes

This SPEC ships:

- `core/ai/AIProvider.h` — POD domain type (`id`, `name`, `baseUrl`, `apiKeyRef`,
  `model`, `kind` ∈ {chat,whisper,both}, `enabled`).
- `core/ai/AIProviderStore.{h,cpp}` — pure model store (load/save/upsert/find).
  Default-seeded with Groq + OpenAI + OpenRouter when the JSON is missing.
- `apps/desktop-qt/src/AIProvidersModel.{h,cpp}` — `QAbstractListModel`
  wrapping the store, owning `ai-providers.json` (500 ms debounced writes via
  a model-side `QTimer` ⇒ `JsonStore::scheduleWrite`). Tracks
  `defaultChatProviderId` / `defaultWhisperProviderId` (stored alongside the
  array as an object root — backward-compatible reader still accepts a bare
  array file).
- `ui/qml/editors/AIProviderEditor.qml` — inline (non-modal) form with
  name / baseUrl / API key (eye-toggled password field) / model (editable
  combo) / kind / enabled / Save / Cancel.
- `ui/qml/AIProvidersTab.qml` — two-column tab: provider list (with selection
  highlight, "padrão chat"/"padrão voz" badges, delete on hover) + the
  editor above.

Persisted file is `<AppData>/Dante Testa/Dante CLI/ai-providers.json`:

```json
{
  "providers":      [ { "id": "…", "name": "Groq", "baseUrl": "https://…",
                        "apiKeyRef": "<literal-secret>", "model": "…",
                        "kind": "both", "enabled": true } ],
  "defaultChat":    "<provider-id>",
  "defaultWhisper": "<provider-id>"
}
```

## CMakeLists.txt additions

### `core/CMakeLists.txt`

Inside the `add_library(dante-core STATIC …)` source list, add the new pair
right next to the existing `ai/GroqClient.*` entry:

```cmake
    ai/GroqClient.cpp
    ai/GroqClient.h
    ai/AIProvider.h
    ai/AIProviderStore.cpp
    ai/AIProviderStore.h
```

(`AIProvider.h` is header-only so it doesn't strictly need to be listed for
linking, but adding it keeps the IDE project explorer accurate.)

### `apps/desktop-qt/CMakeLists.txt`

1. Add the model to the executable source list (after `FileTreeController.h`):

   ```cmake
       src/FileTreeController.cpp
       src/FileTreeController.h
       src/AIProvidersModel.cpp
       src/AIProvidersModel.h
   ```

2. Add the QML files to both the alias loop and the `qt_add_qml_module`
   `QML_FILES` block:

   ```cmake
   # In the first foreach (root .qml files):
   foreach(qml_file Main TabBar TabChip Sidebar Terminal BottomToolbar Theme
                    AIOverlay AIMessage SchemePicker
                    PaneView SplitContainer CommandPalette VoiceWidget
                    UpdateBanner LayoutDesigner SettingsPanel
                    EmptySlot VideoOpenDialog GridWorkspace FileTreeView
                    MovablePopup CheatsheetView AboutView EditorView
                    RecursiveSplit AIProvidersTab)
   ```

   ```cmake
   # In the editors foreach:
   foreach(qml_file EmojiPicker FavoriteEditor SnippetEditor CredentialEditor
                    AIProviderEditor)
   ```

   ```cmake
   # In qt_add_qml_module(...) QML_FILES:
       ../../ui/qml/AIProvidersTab.qml
       ../../ui/qml/editors/AIProviderEditor.qml
   ```

## AppState.h additions (if any)

None. The model is self-contained and writes its own JSON file. The legacy
`groqApiKey` setting in `AppState` stays as-is for backward compatibility
with the Voz tab; once Voz is migrated to read from `aiProviders` directly
we can deprecate that field.

## AIController.cpp changes

Right now `AIController::refreshApiKey()` reads `appState_->groqApiKey()`
and pins the hardcoded Groq base URL inside `GroqClient`. Once the new
model is wired, route through it when a default chat provider is set.
Add `AIProvidersModel*` as a ctor argument; the diff (illustrative):

```diff
--- a/apps/desktop-qt/src/AIController.h
+++ b/apps/desktop-qt/src/AIController.h
@@
-    explicit AIController(AppState* appState, QObject* parent = nullptr);
+    explicit AIController(AppState* appState,
+                          AIProvidersModel* providers,
+                          QObject* parent = nullptr);
@@
     AppState*        appState_{nullptr};
+    AIProvidersModel* providers_{nullptr};
     ai::GroqClient*  groq_{nullptr};

--- a/apps/desktop-qt/src/AIController.cpp
+++ b/apps/desktop-qt/src/AIController.cpp
@@
-AIController::AIController(AppState* appState, QObject* parent)
+AIController::AIController(AppState* appState,
+                           AIProvidersModel* providers,
+                           QObject* parent)
     : QObject(parent)
     , appState_(appState)
+    , providers_(providers)
     , groq_(new ai::GroqClient(this))
@@
     if (appState_) {
         connect(appState_, &AppState::settingsChanged, this, &AIController::refreshApiKey);
     }
+    if (providers_) {
+        connect(providers_, &AIProvidersModel::defaultsChanged,
+                this, &AIController::refreshApiKey);
+        connect(providers_, &AIProvidersModel::dataChanged,
+                this, &AIController::refreshApiKey);
+    }
@@
 void AIController::refreshApiKey() {
-    if (!appState_) {
-        setStatus("no_api_key");
-        return;
-    }
-    const QString key = appState_->groqApiKey();
-    groq_->setApiKey(key);
+    QString key;
+    QString baseUrl;
+    QString model;
+    if (providers_) {
+        const auto p = providers_->providerById(providers_->defaultChatProviderId());
+        if (!p.id.isEmpty() && p.enabled) {
+            key     = p.apiKeyRef;
+            baseUrl = p.baseUrl;
+            model   = p.model;
+        }
+    }
+    // Fallback to the legacy single-provider Groq settings until the
+    // user picks a default chat provider in the new tab.
+    if (key.isEmpty() && appState_) key = appState_->groqApiKey();
+    groq_->setApiKey(key);
+    if (!baseUrl.isEmpty()) groq_->setBaseUrl(baseUrl);
+    if (!model.isEmpty())   groq_->setChatModel(model);
     if (key.isEmpty()) {
         setStatus("no_api_key");
     } else if (status_ == "no_api_key") {
         setStatus("idle");
     }
 }
```

`GroqClient` currently has hardcoded `kChatUrl`/`kWhisperUrl`. To route to
arbitrary OpenAI-compatible endpoints, add a `setBaseUrl(QString)` setter
and concatenate `"/chat/completions"` / `"/audio/transcriptions"` paths at
request time. This is a follow-up patch — the model store + UI shipped in
this SPEC are functional without it (the user can register providers and
pick defaults; AIController falls back to the legacy Groq path until that
GroqClient diff lands).

## SettingsPanel.qml changes

Replace the entire `// 3 — AI Providers (placeholder …)` block (currently
the `ScrollView { … } /* end of placeholder */` between the two comments
at lines ~368-430) with a single AIProvidersTab instance. Exact replacement:

**Find** (around line 368):

```qml
            // 3 — AI Providers (placeholder — providers are still hardcoded
            // in the toolbar; the editor lands once the backend stores them).
            ScrollView {
                clip: true
                ColumnLayout {
                    width: tabStack.width
                    spacing: 18
                    Item { Layout.preferredHeight: 12 }

                    Text {
                        Layout.leftMargin: 18; Layout.rightMargin: 18
                        Layout.fillWidth: true
                        text: qsTr("Configure as CLIs de IA exibidas na toolbar.")
                        color: Theme.fgMuted
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                    }
                    SectionCard {
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Layout.margins: 14

                            // Hardcoded preview rows mirroring the Swift defaults.
                            Repeater {
                                model: [
                                    {name: "Claude",  cmd: "claude",  icon: "🤖", colorIdx: 2},
                                    {name: "Gemini",  cmd: "gemini",  icon: "✨", colorIdx: 6},
                                    {name: "Codex",   cmd: "codex",   icon: "💻", colorIdx: 4}
                                ]
                                delegate: ProviderRow {
                                    name: modelData.name
                                    command: modelData.cmd
                                    icon: modelData.icon
                                    colorIdx: modelData.colorIdx
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.topMargin: 4
                                Button {
                                    text: qsTr("Adicionar Provider")
                                    enabled: false
                                    onClicked: { /* TODO: backend */ }
                                }
                                Button {
                                    text: qsTr("Resetar para padrão")
                                    flat: true
                                    enabled: false
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: qsTr("(edição em breve)")
                                    color: Theme.fgFaint
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 12 }
                }
            }
```

**Replace with**:

```qml
            // 3 — AI Providers (SPEC-040 — full editor backed by aiProviders).
            AIProvidersTab {
                // Top-level item of the tab — fills its StackLayout slot.
            }
```

The `ProviderRow` component declaration at the bottom of `SettingsPanel.qml`
can stay (other tabs don't reference it, but removing it is non-load-bearing
— leave it for now to keep this SPEC's diff small).

## main.cpp additions

```diff
@@
 #include "AIController.h"
+#include "AIProvidersModel.h"
@@
     auto* ai         = new dante::AIController(appState, &app);
+    auto* aiProviders = new dante::AIProvidersModel(&app);
@@
     appState->hydrate();
     favorites->hydrate();
     snippets->hydrate();
     credentials->hydrate();
+    aiProviders->hydrate();
@@
     engine.rootContext()->setContextProperty("ai",         ai);
+    engine.rootContext()->setContextProperty("aiProviders", aiProviders);
     engine.rootContext()->setContextProperty("voice",      voice);
```

After the `AIController` accepts `AIProvidersModel*` (see diff above), change
the construction line to:

```cpp
    auto* aiProviders = new dante::AIProvidersModel(&app);
    auto* ai          = new dante::AIController(appState, aiProviders, &app);
    // ... hydrate order: appState first, then aiProviders before AIController
    //     actually uses its refs.
```

(The order in `main.cpp` is fine because `AIController::refreshApiKey()` runs
once during construction and again on every `defaultsChanged` signal — the
first one will see an empty store, the second will see the hydrated one.)

## Verification

1. App launches, Settings opens, tab "AI Providers" shows the seeded list
   (Groq / OpenAI / OpenRouter) on first run.
2. Clicking "+ Adicionar provider" appends a row and selects it; the editor
   on the right populates with the placeholder defaults.
3. Editing name / baseUrl / API key / model / kind / enabled and clicking
   "Salvar" persists to `ai-providers.json` within ~500 ms (debounce).
4. Selecting a row and clicking "Definir como padrão (chat)" stamps the
   "padrão chat" badge; same for voice.
5. Deleting a provider (hover trash icon) removes it from the list and
   clears any default reference that pointed at it.
6. Restarting the app rehydrates the list exactly as it was, including the
   default-chat / default-voice picks.
7. (After `AIController` integration above) sending an AI prompt routes
   through the picked default chat provider's base URL + model + key. Until
   the GroqClient `setBaseUrl` follow-up lands, the chat path continues to
   use the hardcoded `api.groq.com` endpoint with whatever key
   `AIController::refreshApiKey()` resolves.
