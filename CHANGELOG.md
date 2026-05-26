# Changelog

Todas as mudanças notáveis seguem [Keep a Changelog](https://keepachangelog.com/)
e o projeto usa [Semantic Versioning](https://semver.org/).

## v0.7.0-alpha.5 — 2026-05-26 (sprint noturno multi-agente)

**Salto grande — 8 agentes paralelos fecharam o bloco de features que separava
o MVP do "produto vendável". App agora roda no Mac e o instalador Windows
está em [v0.7.0-alpha.5](https://github.com/dantetesta/Dante-CLI-Qt/releases/tag/v0.7.0-alpha.5).**

### Adicionado

- **PTY real** (`infra/shell-adapters/ConPtyAdapter` no Windows via
  `CreatePseudoConsole`, `infra/shell-adapters/UnixPtyAdapter` no Mac/Linux
  via `forkpty`). Reader em QThread, queued signals, NUL-byte scrub.
- **Emulador VT100/ANSI** — Williams VT500 state machine completa em
  `core/terminal/VTParser`, com SGR (16+256+true-color), CSI cursor moves,
  ED/EL, IL/DL/DCH/ECH, DECSTBM, DECSCUSR, DEC private modes (1049 alt
  screen, 25 cursor visible, 2004 bracketed paste). Buffer 50k-line ring,
  CJK/emoji wide-char.
- **Terminal view nativo** — `TerminalView` (QQuickPaintedItem) substituiu
  o TextArea cru. Run-length batched `drawText`, encoder completo de teclas
  (arrows/F1-F12/Ctrl-letters/Alt-prefix/Shift-Tab), mouse wheel scrollback.
- **AI Assistant overlay** (`AIController` + `AIOverlay.qml`) — slide-in
  glass panel, markdown subset (`**bold**`, ``` `code` ```, fences),
  histórico persistido em `ai-history.json`. Atalho `Ctrl+Shift+A` e
  botão ✨ AI. Quick actions "Explicar comando" e "Sugerir comando".
- **CRUD editors** — `FavoriteEditor`, `SnippetEditor`, `CredentialEditor`
  (com templates dinâmicos por kind: SSH/FTP/API/Database/Custom),
  `EmojiPicker` compartilhado. Right-click em cada linha da sidebar →
  menu Editar/Excluir. Botão "+" na sidebar abre o editor adequado por modo.
- **11 esquemas de cor** — `core/themes/TerminalScheme` + `ThemeRegistry`:
  TokyoNight (default), Dracula, OneDark, SolarizedDark/Light,
  GruvboxDark, CatppuccinMocha, Monokai, Nord, RosePine, Kanagawa.
  Palettes idênticas ao Swift sibling. `SchemePicker.qml` UI.
- **Microinterações** — `Behavior on color/scale/x/width` em TabChip,
  Sidebar mode-picker (hover + select + press), sliding underline na
  tab bar ativa, splash fade-in, splitter hover/press, transições de
  sidebar width animadas.
- **Split panes** (`SplitContainer.qml` + `PaneView.qml`) — até 2 panes
  por tab (vertical/horizontal). Drag splitter, click-to-focus, atalhos
  `Ctrl+\\` `Ctrl+Shift+\\` `Ctrl+Shift+W` `Ctrl+Alt+←/→`. Botão Layout
  no toolbar abre popup com modos.
- **Voz/Whisper** (`VoiceController` + `QtVoiceCapture` via QAudioSource +
  `VoiceWidget.qml`) — captura mono s16le 16 kHz → temp WAV → Groq
  Whisper. HUD com waveform 16-bar driven by EMA RMS. Botão 🎤 toggle.
- **Command palette** (`PaletteController` + `CommandPalette.qml`) — modal
  com fuzzy matching, ~28 comandos hardcoded + dinâmico (1 por favorito,
  1 por snippet, 1 por tema). Atalho `Ctrl+K`. `↑↓` navega, Enter
  executa, Esc fecha.

### Bugfixes no caminho

- `TerminalBuffer::resize` crashava no construtor por copy loop indexando
  `cells` vazio com `rows=24,cols=80` (defaults da struct). Guard adicionado.
- `TerminalView::sendKey` estava declarado Q_INVOKABLE mas sem impl.
- `Qt6::Gui` precisou ser adicionado a `dante-core` por causa do `QColor`
  em `TerminalScheme.h`.
- `qtmultimedia` precisou ser adicionado ao `install-qt-action` no CI.

### Caveats honestos

- Connections em `Main.qml` apontando pra `palette` mostram warnings
  "no signal of the target matches" — quirk do qmlcache estático com
  context properties (funciona em runtime).
- ConPTY nunca foi testado em hardware Windows real — só compila e o
  app abre. Real run-test depende do user.
- Voz: WASAPI/AVFoundation são stubs; fallback usa `QtVoiceCapture`
  (QAudioSource cross-platform). Permissão de microfone macOS dispara
  prompt do sistema na primeira gravação.
- AI streaming não wireado — `GroqClient::chat` retorna resposta inteira.
- Splits limitados a 2 panes (não recursivo). `splitFraction` não
  persiste (reset 0.5 no relaunch).
- Editores: `FolderDialog` (Qt.labs.platform) não foi importado; usa
  `FileDialog` em modo file e deriva o dir pai.
- 4 warnings de `Unable to assign [undefined]` em CommandPalette.qml
  (bindings reativas antes do PaletteController inicializar — cosmético).

---

## v0.7.0-alpha.3 — 2026-05-26

**Primeiro build CI verde com installer publicado no GitHub Release.**

### Adicionado

- **Stack nova:** rewrite completo em C++20 + Qt 6 + QML + CMake/Ninja.
- **Backbone:** `core/` (domain models, JSON persistence atomic-save, OSC parser,
  TerminalSession/Registry, Groq client) e `infra/` (ShellAdapter QProcess MVP).
- **Bridge para QML:** `AppState`, `TabsModel`, `FavoritesModel`,
  `SnippetsModel`, `CredentialsModel`, `TerminalBridge` — todos como
  `QAbstractListModel`/`QObject` expostos via context properties.
- **UI QML:** Main, TabBar, TabChip, Sidebar (4 modos), Terminal, BottomToolbar,
  Theme (singleton com tokens de design Dark+).
- **CI Windows:** `.github/workflows/windows-build.yml` com `windows-latest`,
  Qt 6.7.2, MSVC, Ninja, Inno Setup via chocolatey. Sobe artifact + attach ao
  Release quando tag `v*` é empurrada.
- **Installer Inno:** `installer/dante-cli.iss` produz
  `Dante-CLI-Setup-0.7.0-x64.exe` (~48 MB).

### Bugs corrigidos no caminho do MVP

- `TerminalRegistry` usava `QHash<QString, unique_ptr<Session>>` — não compila
  no MSVC porque `QHash` exige `CopyAssignable`. Trocado por
  `std::unordered_map` com hash adapter de `QString`.
- `TabChip.qml` declarava `property color color: ...` — colide com
  `Rectangle.color`. Renomeado para `tint`.
- Workflow inicial referenciava `jrsoftware/issetup-action@v1` (inexistente);
  trocado por `choco install innosetup`.
- `permissions: contents: write` no workflow para o `GITHUB_TOKEN` poder
  criar releases (repo está com default `read`).
- Artifact upload marcado `continue-on-error: true` + `retention-days: 5`
  para não bloquear o pipeline quando a cota de storage da conta estoura.

### Gaps deliberados (TODO p/ paridade com Swift)

- ConPTY real (atualmente `QProcess`, sem PTY de verdade)
- Emulação VT100/ANSI no `Terminal.qml`
- Split panes, zoom de célula, navegação por split
- AI assistant overlay (backend Groq pronto, falta UI)
- Captura de voz Whisper (backend pronto, falta WASAPI)
- Editor de favoritos/snippets/credenciais (só leitura/uso por enquanto)
- Múltiplos esquemas de cor (atualmente só Dark+)
- Pin/unpin, reordenar abas por drag
- Atalhos completos + paleta de comandos ⌘K
- Tray icon + autoupdate
- Assinatura de código (EV cert) p/ evitar SmartScreen

---

## v0.6.2 (Tauri) — 2026-05-19

Última versão da stack anterior (Tauri + React + Rust). Preservada em
`../Dante-CLI-Tauri/` como fallback. Bug do PTY no Win11 (NUL bytes vazando
para `CreateProcessW`) motivou o pivot pra C++/Qt nesta v0.7.
