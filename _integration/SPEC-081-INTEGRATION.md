# SPEC-081 — Generators registry integration

> Depends on SPEC-080 being integrated first (AutoFillController is required
> for the palette's "Usar" button to route templates through the dialog).

## CMakeLists.txt

### `core/CMakeLists.txt`
Append to the `add_library(dante-core STATIC ...)` source list:

```
generators/Generator.h
generators/GeneratorsRegistry.cpp
generators/GeneratorsRegistry.h
```

**No QRC required.** The seed catalog is embedded as a `R"SEED(...)SEED"` raw
string literal inside `GeneratorsRegistry.cpp`. The companion
`generators_seed.json` file in the same folder is the human-readable source of
truth — when editing the catalog, edit the JSON file and copy/paste the new
contents over the literal in the `.cpp`. Investigation of `core/` found no
existing `.qrc` files, so adding one just for this would be premature.

If you later decide to ship the JSON via QRC instead, create
`core/resources.qrc` with `<file>generators/generators_seed.json</file>`,
register it in `core/CMakeLists.txt` via `qt_add_resources`, then swap
`kSeedJson` for a `QFile(":/generators/generators_seed.json")` read.

### `apps/desktop-qt/CMakeLists.txt`
Append to the `qt_add_executable(dante-cli WIN32 ...)` source list:

```
src/GeneratorsModel.cpp
src/GeneratorsModel.h
```

Register the new QML file:

1. Append `GeneratorsPalette` to the `foreach(qml_file Main TabBar ...)`
   block.
2. Append `../../ui/qml/GeneratorsPalette.qml` to the
   `qt_add_qml_module(dante-cli ... QML_FILES ...)` list.

## `apps/desktop-qt/src/main.cpp`

```cpp
#include "GeneratorsModel.h"
// ...
auto* generators = new dante::GeneratorsModel(&app);
engine.rootContext()->setContextProperty("generators", generators);
```

Place it next to the other context-property setups (favorites, snippets,
schemes). No signal wiring needed — the QML side calls `generators.pick(id)`
synchronously and feeds the result into `autoFill.prepare(text)`.

## `ui/qml/Main.qml`

1. Add an instance at the root level (sibling of `CommandPalette`,
   `AboutView`, etc.):

   ```qml
   GeneratorsPalette {
       id: generatorsPalette
   }
   ```

2. Register the keyboard shortcut. Use the same pattern Main.qml already uses
   for Ctrl+K / Ctrl+Shift+A:

   ```qml
   Shortcut {
       sequence: "Ctrl+G"
       context: Qt.ApplicationShortcut
       onActivated: generatorsPalette.open()
   }
   ```

## `ui/qml/Sidebar.qml`

Add a "Geradores" quick-action button. Use the same row pattern Sidebar.qml
already uses for the other sidebar shortcuts (cheatsheet/about/etc.):

```qml
// Inside the actions / shortcuts column:
SidebarAction {           // whichever component the file already uses
    icon: "⚙"
    label: qsTr("Geradores")
    shortcut: "Ctrl+G"
    onClicked: generatorsPalette.open()
}
```

If Sidebar.qml doesn't have a reusable `SidebarAction` component, replicate
the markup of the existing "Cheatsheet" or "Sobre" entry verbatim and swap
the text + handler.

## Verification

1. Launch the app. Press `Ctrl+G` — the Generators palette opens centered.
2. The left column lists 10 categories (`git`, `curl`, `jq`, `docker`,
   `ssh`, `find`, `grep`, `tar`, `system`, `database`).
3. Click `git` → 5 cards appear ("git commit", "git commit --amend",
   "git push origin <branch>", "git checkout -b", "git log --grep").
4. Click "Usar" on `git commit` → palette closes, AutoFillDialog opens
   with one field labeled **Mensagem**.
5. Type a message, click Executar — terminal receives
   `git commit -m "<message>"\r`.
6. Reopen Ctrl+G, type "docker" in the search box — every category is
   suppressed and only docker cards remain visible.
7. Optional: drop a `generators.json` at
   `<AppData>/Dante Testa/Dante CLI/generators.json` with an array like
   `[{"id":"git.commit","category":"git","name":"meu commit","description":"override","templateText":"git commit -m \"$(msg|text)\"\r","icon":"⭐"}]`
   — restart the app and confirm the seed entry is replaced.
