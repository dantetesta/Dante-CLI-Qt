# Dante CLI — SPEC.md

**Engenharia reversa do Swift sibling + matriz de transpile pra Qt 6 / C++20.**

Documento canônico que define o que o produto **deve** ser e o estado atual
do port. Cada gap tem um ID estável (`SPEC-NNN`). Cada commit subsequente
deve fechar pelo menos um gap e referenciá-lo na mensagem.

- **Origem**: `~/Desktop/_ORGANIZADO_2026-05-12/.../TestaTerminal/` — 108 arquivos Swift, 21 531 LOC, macOS 14+, SwiftUI + SwiftTerm + Highlightr.
- **Destino**: este repo (`Dante-CLI-Qt`) — Qt 6.5 / QML / MSVC + macOS, atualmente v0.7.0-alpha.16.
- **Cobertura medida**: ~40 % features end-to-end, ~85 % do scaffold de UI, ~93 % dos controllers (LOC), audio nativo zero.

---

## 1. Mapa arquitetural Swift → Qt

| Swift | Qt 6 / C++ | Status |
|---|---|---|
| `AppState` (`@MainActor` singleton, Combine `@Published`) | `dante::AppState : QObject` com `Q_PROPERTY` + sinais | ✅ |
| `Models/*.swift` (POD Codable) | `core/domain/Models.h` (structs) + `*Model.h/cpp` (`QAbstractListModel`) | ✅ parcial — faltam modelos para Editor/Browser/Video/Calculator (ver SPEC-021..024) |
| `Services/*Store.swift` (JSONEncoder, debounce 300 ms) | `core/persistence/JsonStore` (`QSaveFile` atomic, debounce QTimer 300–500 ms) | ✅ |
| `Services/TerminalSessionRegistry` + SwiftTerm | `core/terminal/TerminalSession` + `TerminalBuffer` + `VTParser` + `apps/.../TerminalView` | ✅ MVP — mouse modes 1000/1002/1003/1006 ainda TODO em `VTParser.cpp:265` (SPEC-031) |
| `Services/GroqChatClient` + `GroqWhisperClient` | `core/ai/GroqClient` (chat + chatStream SSE + whisper multipart) | ✅ |
| `Services/AIProviderStore` | `apps/.../AIController` + ❌ **falta backend de Providers** (SPEC-040) | 🟡 |
| `Services/BrowserSessionRegistry` (WKWebView) | ❌ **falta WebEngine integration** (SPEC-050) | ❌ |
| `Services/FileBrowserService` + `GitStatusService` | `apps/.../FileTreeController` (QFileSystemModel) + ❌ git status (SPEC-051) | 🟡 |
| `Services/ProcessStats` (CPU/RAM) | ❌ **falta ProcessStatsController** (SPEC-060) | ❌ |
| `Services/ClaudeResourceScanner` + Right Sidebar | ❌ **falta `~/.claude/` scanner + Skills/Agents/MCPs panel** (SPEC-070, 071) | ❌ |
| `Services/AutoFillService` (681 LOC, `$VAR`, `$(…)`) | ❌ **falta AutoFill engine** (SPEC-080) | ❌ |
| `Services/Generators` (templates de comando) | ❌ **falta Generators registry** (SPEC-081) | ❌ |
| `Services/UpdateChecker` + `InstallCleanup` | `core/update/UpdateChecker` + `apps/.../UpdateController` + Inno `/SILENT` | ✅ |
| `Services/CalculatorEngine` + `CalculatorPiPManager` | ❌ **falta Calculator pane + engine** (SPEC-024) | ❌ |
| `Utilities/AppCommands` (atalhos) | atalhos hardcoded em `Main.qml` + `PaletteController` | 🟡 — faltam ~10 atalhos (SPEC-090) |
| `Utilities/TerminalThemes` (43 schemes) | `core/themes/ThemeRegistry` (11 schemes) | 🟡 — **32 schemes faltando** (SPEC-100) |
| `Views/RootView` | `ui/qml/Main.qml` | ✅ |
| `Views/Sidebar/*` (Files, Favorites, Snippets, Credentials) | `ui/qml/Sidebar.qml` + `editors/*` + `FileTreeView.qml` | ✅ |
| `Views/RightSidebar/*` (Skills, Agents, MCPs) | ❌ **não existe** (SPEC-070) | ❌ |
| `Views/Settings/SettingsScene` (5 abas) | `ui/qml/SettingsPanel.qml` (5 abas) | 🟡 — aba "AI Providers" stub (SPEC-040) |
| `Views/Toolbar/*` (AI launcher, Mic, Stats, Split picker) | `ui/qml/BottomToolbar.qml` + `VoiceWidget.qml` | 🟡 — falta CPU/RAM (SPEC-060), AI launcher é hardcoded (SPEC-040) |
| `Views/Terminal/PaneSplitView` + `PaneNode` (binary tree, N panes) | `ui/qml/SplitContainer.qml` + `GridWorkspace.qml` | 🟡 — **só 2 panes ou grid** (SPEC-110) |
| `Views/TabBar/TabChip` (pin, inline edit, color, emoji, scheme menu) | `ui/qml/TabBar.qml` + `TabChip.qml` | 🟡 — falta pin, inline edit double-click, color picker no menu (SPEC-120) |
| `Views/Editor/EditorPaneView` (Highlightr) | ❌ **falta editor com syntax highlight** (SPEC-021) | ❌ |
| `Views/Browser/BrowserPaneView` (WKWebView) | ❌ **falta browser embedded** (SPEC-022) | ❌ |
| `Views/Video/VideoPaneView` (AVPlayer) | ❌ **falta video player** (SPEC-023) — só dialog placeholder |
| `Views/Calculator/CalculatorPaneView` | ❌ **falta calculadora** (SPEC-024) |
| `Views/EmojiPicker` (1500+ emojis, categorias, recents) | `ui/qml/editors/EmojiPicker.qml` (67 LOC, grid mínimo) | 🟡 — **falta catálogo grande + categorias + recents** (SPEC-130) |
| `Views/ExplainModalView` | ✅ via `AIOverlay` (botão "Explicar") |
| `Views/CheatsheetView` (⌘/) | ❌ **falta cheatsheet popup** (SPEC-140) |
| `Views/AboutView` | ❌ **falta About window** (SPEC-141) |
| Focus mode (⌘., overlay) | ❌ **falta focus mode** (SPEC-142) |
| `MicButton` (idle/rec/transcribing, pulse, timer) | `VoiceWidget.qml` | ✅ |
| `UpdateBanner` + `UpdatePromptView` | `UpdateBanner.qml` | ✅ |
| `SplitWorkspacePicker` | `LayoutDesigner.qml` | ✅ |
| `MovablePopup` (não existe no Swift, Sheet nativo) | `MovablePopup.qml` | ✅ (extra nosso) |
| `Command Palette ⌘K` (não existe no Swift) | `PaletteController` + `CommandPalette.qml` | ✅ (extra nosso) |
| Tray icon (não existe no Swift) | `TrayController` | ✅ (extra nosso) |
| `AppState+LayoutTemplates` | `LayoutTemplatesController` | ✅ |
| **Áudio nativo** — `AVAudioRecorder` (macOS) | `MacVoiceCapture` (stub 23 LOC) + fallback `QtVoiceCapture` | 🟡 — fallback funciona, nativo faltando (SPEC-150) |
| **Áudio nativo** — WASAPI (Windows) | `WasapiVoiceCapture` (stub 35 LOC) + fallback `QtVoiceCapture` | 🟡 — fallback funciona, nativo faltando (SPEC-151) |
| Localização PT-BR + EN fallback | strings hardcoded PT-BR/EN; sem `tr()` consistente | 🟡 (SPEC-160) |

**Legenda**: ✅ feature-complete · 🟡 stub / parcial · ❌ não existe.

---

## 2. Persistência

| Arquivo JSON | Swift | Qt |
|---|---|---|
| `settings.json` | ✅ | ✅ |
| `session.json` | ✅ | ✅ |
| `favorites.json` | ✅ | ✅ |
| `snippets.json` | ✅ | ✅ |
| `credentials.json` | ✅ (plaintext, sem Keychain) | ✅ (mesma decisão — `credentials.json`) |
| `layout_templates.json` | ✅ | ✅ |
| `ai_providers.json` | ✅ | ❌ falta (SPEC-040) |
| `ai-history.json` | ❌ (não persiste chat) | ✅ (extra nosso, últimas 20 msgs) |

---

## 3. Atalhos de teclado — completude

Catálogo Swift completo no audit. Status Qt:

| Atalho | Swift | Qt | SPEC |
|---|---|---|---|
| ⌘T / Ctrl+T | ✅ | ✅ | – |
| ⌘W / Ctrl+W | ✅ (bloqueia se pinned) | ✅ (sem pin guard) | SPEC-091 |
| ⌘⇧W | ✅ (close pane) | ✅ | – |
| ⌘O / Ctrl+O | ✅ (open file → editor) | ❌ (sem editor) | SPEC-021 |
| ⌘S | ✅ (save editor) | ❌ | SPEC-021 |
| ⌘⌥S | ✅ (save all) | ❌ | SPEC-021 |
| ⌘D | ✅ split vertical | 🟡 mapeado pra `Ctrl+\` | SPEC-092 (rebind p/ ⌘D) |
| ⌘⇧D | ✅ split horizontal | 🟡 mapeado pra `Ctrl+Shift+\` | SPEC-092 |
| ⌘⇧[ / ⌘⇧] | ✅ prev/next tab | ✅ | – |
| ⌘1..⌘9 | ✅ jump to tab N | ❌ | SPEC-093 |
| ⌘⇧S | ✅ toggle left sidebar | ❌ | SPEC-094 |
| ⌘] | ✅ toggle right sidebar | 🟡 (botão BottomToolbar, não atalho) | SPEC-094 |
| ⌘R | ✅ refresh sidebar | ❌ | SPEC-095 |
| ⌘L | ✅ search favorites | ❌ | SPEC-096 |
| ⌘⌃M | ✅ toggle mic | ❌ | SPEC-097 |
| ⌘⇧K | ✅ clear line | ❌ | SPEC-098 |
| ⌘/ | ✅ cheatsheet | ❌ | SPEC-140 |
| ⌘. | ✅ exit focus | ❌ | SPEC-142 |
| **⌘K** | ❌ não existe | ✅ command palette (extra) | – |
| **⌘⇧A** | ❌ não existe | ✅ AI overlay (extra) | – |

---

## 4. Inventário de gaps por prioridade

> Cada gap vira uma issue/commit numerada. Versões alpha incrementam a cada gap fechado.

### P0 — bloqueia "produto premium" (fechar antes de qualquer outra cosmetic)

| ID | Título | Custo | Notas |
|---|---|---|---|
| SPEC-021 | **Editor pane** com syntax highlight | M (1 dia) | `QSyntaxHighlighter` ou `KSyntaxHighlighting` (Qt-friendly LGPL). Adiciona `EditorContent` ao Tab kind. ⌘O / ⌘S funcionam aí. |
| SPEC-100 | **32 themes faltando** (43 total) | S (2 h) | Portar `TerminalThemes.swift` → adicionar entries em `ThemeRegistry.cpp`. Mecânico. |
| SPEC-110 | **PaneNode recursivo** — N panes por aba | L (2 dias) | Refatorar `SplitContainer` p/ árvore binária. Permite splits `vertical(vertical(a,b), horizontal(c,d))`. |
| SPEC-120 | **TabChip features faltando** — pin, double-click rename, color picker no menu, scheme override | S (3 h) | Right-click menu + inline edit. AppState já persiste `pinned` (não enforça). |
| SPEC-130 | **EmojiPicker completo** — 1500+ emojis, categorias, recents | M (4 h) | Portar `EmojiCatalog.swift` (já tem o dado pronto). |
| SPEC-140 | **Cheatsheet popup** (⌘/) | S (2 h) | Tabela em QML. Conteúdo já está nesse SPEC. |
| SPEC-031 | **Mouse modes VT** (1000/1002/1003/1006) | S (2 h) | Apps como vim/htop mandam mouse events ao receber esses modos. |

### P1 — features visíveis do Swift que faltam

| ID | Título | Custo | Notas |
|---|---|---|---|
| SPEC-022 | **Browser pane** com QtWebEngine | L (1 dia + 150 MB no installer) | LGPL com restrições. Adicionar `BrowserContent` ao Tab kind. |
| SPEC-023 | **Video pane** com QtMultimedia | M (4 h + 30 MB) | Local files via `Video` QML element; YouTube via `WebEngineView` (se 022 entrar). |
| SPEC-024 | **Calculator pane** + engine | M (4 h) | Portar `CalculatorEngine.swift` (parser PEMDAS). UI segue layout Swift. |
| SPEC-040 | **AI Providers tab funcional** | M (4 h) | `AIProviderStore` + UI editor. Substitui a aba placeholder. |
| SPEC-060 | **Process stats indicator** (CPU/RAM real-time) | M (3 h) | Win: GetSystemTimes / GetProcessMemoryInfo. Mac: host_statistics + task_info. Sampling 1 s. |
| SPEC-070 | **Right Sidebar — Skills/Agents/MCPs** | L (1.5 dia) | Mirror `ClaudeResourceScanner` (YAML parse, file watch, async rescan) + 3 tabs. |
| SPEC-090 | **Atalhos faltando** (10 ⌘ shortcuts) | S (2 h) | Mapeamento em `Main.qml`. Ver tabela seção 3. |
| SPEC-141 | **About window** | S (1 h) | Versão, créditos, links, ícone. |
| SPEC-142 | **Focus mode** (⌘. exit, overlay pill) | S (2 h) | Esconde sidebars + tab bar + bottom toolbar. |
| SPEC-051 | **Git status** na file tree | S (2 h) | Branch + dirty/staged/untracked badges por pasta. |

### P2 — polish, polish, polish

| ID | Título | Custo | Notas |
|---|---|---|---|
| SPEC-050 | Browser sessions persistidas | M | Depende de SPEC-022. |
| SPEC-080 | **AutoFill engine** (`$VAR`, `$(…)`) | L (1 dia) | Variáveis dinâmicas em snippets/favoritos. Não bloqueia, mas é poderoso. |
| SPEC-081 | **Generators registry** (git, curl, jq templates) | M | Catálogo de comandos com hints. |
| SPEC-091 | Pin guard em close tab | XS | `closeTab` ignora se `.pinned`. |
| SPEC-092 | Rebind Cmd+D / Cmd+Shift+D (vez de Ctrl+\) | XS | `Main.qml` shortcut. |
| SPEC-093 | ⌘1..⌘9 jump to tab | S | – |
| SPEC-094 | ⌘⇧S / ⌘] toggle sidebars | XS | – |
| SPEC-095 | ⌘R refresh sidebar | XS | – |
| SPEC-096 | ⌘L focus search favorites | XS | – |
| SPEC-097 | ⌘⌃M toggle mic | XS | – |
| SPEC-098 | ⌘⇧K clear line | XS | – |
| SPEC-150 | **Mac AVFoundation capture** | L (1 dia) | Substituir `MacVoiceCapture` stub por `.mm` real. Não bloqueia (fallback Qt funciona). |
| SPEC-151 | **Win WASAPI capture** | L (1 dia) | Idem `WasapiVoiceCapture`. |
| SPEC-160 | i18n consistente (`tr()` + .ts files) | M | Cobertura PT-BR + EN. |
| SPEC-170 | Drag tab → grid slot wireado de verdade | M | `DropArea` já existe em `EmptySlot`; falta atribuir slot. |
| SPEC-171 | LayoutDesigner + SettingsPanel viram `MovablePopup` | S | Uniformidade. |
| SPEC-172 | PiP window (calculadora flutuante) | M | Depende de SPEC-024. |

---

## 5. Roadmap por fase

Foco em fechar P0 antes de tocar P1.

### Fase A — "produto utilizável de verdade" (alpha.17 → alpha.25)
1. ✅ SPEC-100 — 47 schemes (alpha.18, commit 9012844)
2. ✅ SPEC-140 — cheatsheet popup (alpha.19, commit 73e375e)
3. ✅ SPEC-120 + SPEC-091 — TabChip polish + pin guard (alpha.20, commit 1597e0e)
4. ✅ SPEC-090 (091..098) + SPEC-141 + SPEC-142 — atalhos + About + Focus (alpha.21, commit 9423e30)
5. ✅ SPEC-031 — mouse modes VT 1000/1002/1003/1006 (alpha.22, commit 339f67b)
6. ✅ SPEC-130 — EmojiPicker completo (~700 emojis, 13 cats, recents) (alpha.23, commit cb57e45)
7. ✅ SPEC-021 — Editor pane (sem syntax highlight ainda) (alpha.24, commit 6b2d9b6)
8. ⏳ SPEC-110 — PaneNode recursivo (N panes) — em andamento

### Fase B — "paridade visível" (alpha.24 → alpha.32)
8. SPEC-090 + 091–098 — todos os atalhos (alpha.24)
9. SPEC-141 — About (alpha.25)
10. SPEC-142 — Focus mode (alpha.26)
11. SPEC-060 — Process stats (alpha.27)
12. SPEC-024 — Calculator (alpha.28)
13. SPEC-023 — Video pane (alpha.29)
14. SPEC-040 — AI Providers tab (alpha.30)
15. SPEC-070 — Right Sidebar Skills/Agents/MCPs (alpha.31–32)

### Fase C — features grandes
16. SPEC-022 — Browser pane com QtWebEngine (beta.1)
17. SPEC-080 — AutoFill engine (beta.2)
18. SPEC-081 — Generators (beta.3)
19. SPEC-150/151 — Native audio capture (beta.4)
20. SPEC-051, 160, 170–172 — polish final (rc.1)

### Critério de pulo Fase A → B
Antes de começar B, todos os itens P0 desta lista devem estar ✅ na matriz da seção 1.

---

## 6. Princípios de implementação

1. **Cada commit cita o SPEC-ID que fecha** (`fix(SPEC-100): add 32 terminal themes`).
2. **Versão sempre sobe** — Toda mudança visível bumpa alpha.N+1 e gera installer. Memória `feedback_version_bump.md`.
3. **QML pinada em `import QtQuick 6.5`** — qualquer API 6.7+ é proibida (lembrar do bug do `topRightRadius` em alpha.16).
4. **Sem regressão visível** — antes de mergear, testar Mac local; se mexeu em CMake/Win, esperar CI verde + sanity em deploy/.
5. **CMakeLists não é onde se hackeia** — quando agentes geram código, todos NÃO tocam CMakeLists; integração final é manual.
6. **Não criar arquivos legacy paralelos** — se quero substituir `X.qml`, edito `X.qml`, não crio `X2.qml`.

---

## 7. Apêndice — Layout JSON files do Qt

```
%APPDATA%/Dante Testa/Dante CLI/             (Windows)
~/Library/Application Support/Dante Testa/Dante CLI/   (macOS)

├── session.json              # tabs + active + grid/split per tab
├── settings.json             # AppSettings (groqApiKey, fontSize, scheme, etc.)
├── favorites.json
├── snippets.json
├── credentials.json
├── layout-templates.json
├── ai-history.json
└── logs/dante-YYYY-MM-DD.log
```

---

**Data**: 2026-05-26 · **Versão deste SPEC**: 1 · **App**: v0.7.0-alpha.16 → próximo v0.7.0-alpha.17 (SPEC commit) → alpha.18 (SPEC-100 — 32 themes).
