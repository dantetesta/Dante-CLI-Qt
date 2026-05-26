# Dante CLI

Terminal premium para Windows e macOS — terminal real (ConPTY/forkpty), assistente
IA com streaming, gravação de voz com Whisper, paleta de comandos, file tree,
layout designer multi-painel, 11 esquemas de cor, autoupdate. Escrito em
**C++20 + Qt 6 + QML + CMake**.

[![windows-build](https://github.com/dantetesta/Dante-CLI-Qt/actions/workflows/windows-build.yml/badge.svg)](https://github.com/dantetesta/Dante-CLI-Qt/actions/workflows/windows-build.yml)

---

## Download

**Release atual:** [`v0.7.0-alpha.12`](https://github.com/dantetesta/Dante-CLI-Qt/releases/latest) — 58 MB

[**▶ Baixar Dante-CLI-Setup-0.7.0-x64.exe (Windows 10/11)**](https://github.com/dantetesta/Dante-CLI-Qt/releases/download/v0.7.0-alpha.12/Dante-CLI-Setup-0.7.0-x64.exe)

> Sem assinatura de código ainda — o SmartScreen do Windows vai pedir
> confirmação na primeira execução ("Mais informações → Executar assim mesmo").
> Resolver isso exige certificado EV (~$300/ano).

Build de macOS: ainda não é distribuído via .dmg; quem tem ambiente Qt
instalado pode compilar local (instruções abaixo).

---

## O que está dentro

### Terminal real
- **PTY nativo**: `CreatePseudoConsole` no Windows (10 1809+), `forkpty` no
  macOS/Linux. Reader thread isolada com sinais Qt queued — UI nunca bloqueia.
- **Emulador VT100/ANSI** próprio (Williams VT500 state machine): SGR
  completo (cores ANSI 16 + 256 + RGB truecolor), CSI cursor moves, ED/EL,
  IL/DL/DCH/ECH, DECSTBM, DECSCUSR, DEC private modes (1049 alt screen, 25
  cursor, 2004 bracketed paste), OSC 0/1/2 (título) + OSC 7 (cwd).
- **Buffer**: 50 mil linhas de scrollback (ring), primary + alt screens,
  CJK/emoji wide-char support.
- **Renderização**: `QQuickPaintedItem` custom com `drawText` batched por
  run-length (cor de fundo em uma pass, foreground em outra). Coalescer de
  repaint em 16 ms.
- **Teclado**: encoder completo — setas, F1–F12, Home/End, PgUp/PgDn,
  Ctrl+letras (control codes), Alt-prefix, Shift-Tab (CSI Z).
- **Mouse**: wheel rola scrollback; middle-click cola (clipboard X11
  selection).

### Assistente IA
- Cliente **Groq** (Llama 3.3 70B padrão) com **streaming SSE** —
  resposta cresce em tempo real na bolha do assistente.
- Quick actions: "Explicar último comando", "Sugerir comando".
- Histórico persistido em `ai-history.json` (últimas 20 mensagens).
- Markdown subset: `**bold**`, `` `code` ``, blocos triple-backtick.
- Atalho: `Ctrl+Shift+A` (ou botão ✨ AI da toolbar).

### Voz / Whisper
- Captura de áudio cross-platform via `QAudioSource` (16 kHz mono s16le).
- Upload multipart pro Groq Whisper, transcrição injetada no terminal ativo.
- HUD com 16 barras de waveform pulsando (EMA do RMS).
- Idiomas: pt/en/es/fr/auto. Modelo configurável (`whisper-large-v3-turbo`
  padrão, opção do `large-v3` regular).

### Command Palette
- `Ctrl+K` abre paleta com **fuzzy matching** (~28 comandos hardcoded +
  dinâmicos: 1 por favorito, 1 por snippet, 1 por tema).
- Subsequence + word-boundary boost, animação scale+fade na abertura.

### Layout multi-painel
- **2-pane SplitContainer**: vertical/horizontal com seam arrastável,
  click-to-focus, atalhos `Ctrl+\\`, `Ctrl+Shift+\\`, `Ctrl+Shift+W`,
  `Ctrl+Alt+←/→`. `splitFraction` persistido por aba.
- **GridWorkspace** (até 6×6): designer visual com setas ↔/↕ entre células
  pra mesclar (cellSpan), × no centro pra desfazer merge. Templates salvos
  em `layout-templates.json`. Slots vazios mostram atalhos coloridos:
  Terminal (verde), Navegador (azul), Anônimo (roxo), Vídeo (vermelho).
- Browser/Anônimo abrem no navegador padrão do SO; Vídeo aceita YouTube/
  Vimeo/arquivo local e abre externamente. *Player/browser embeddado in-app
  requer QtMultimedia/QtWebEngine — TODO.*

### File tree (sidebar Pastas)
- Acesso à **árvore inteira do sistema operacional** via `QFileSystemModel`.
- Quick-places no topo: Início, Desktop, Documentos, Downloads, `/`, mais
  `/Volumes/*` no macOS / drives no Windows.
- Toggle 👁 mostrar/ocultar arquivos ocultos.
- **Right-click menu**: Abrir, Abrir no terminal (`cd`), Inserir caminho,
  Definir como raiz, Revelar no Finder/Explorer, Copiar caminho, Nova
  pasta, Novo arquivo, Renomear, Duplicar, Mover para lixeira.
- **Drag**: arrasta o arquivo/pasta — DropAreas dos slots de grid aceitam
  (mime `text/uri-list`).
- Double-click em pasta = vira raiz da árvore (drill-in).
- Icones contextuais por extensão (.js 🟨, .py 🐍, .rs 🦀, .mp4 🎬…).

### Sidebar: 4 modos
- **★ Favoritos**: pastas marcadas com nome/emoji/cor/comando inicial.
- **📁 Pastas**: file tree completo (descrito acima).
- **⚡ Snippets**: trechos de comando, multi-linha, com tags.
- **🔑 Chaves**: credenciais SSH/FTP/API/Database/Custom com campos
  dinâmicos rotulados, fields mascarados (echoMode password), inserção
  formatada `# Credencial — nome — campo=valor · campo=valor` no terminal.
- Botão "+" e right-click "Editar / Excluir" em cada linha.

### Editores (modais arrastáveis)
- `MovablePopup`: pattern reusável com header arrastável + grip de resize
  no canto inferior direito + parent overlay (centraliza na janela).
- Editor de Favorito, Snippet, Credencial e diálogo de Vídeo usam esse
  pattern — todos podem ser movidos e redimensionados livremente.

### 11 esquemas de cor de terminal
TokyoNight (default), Dracula, OneDark, Solarized Dark, Solarized Light,
Gruvbox Dark, Catppuccin Mocha, Monokai, Nord, Rose Pine, Kanagawa.
Paletas idênticas ao app oficial em Swift — IDs portáveis entre Mac e Win.

### Microinterações
- `Behavior on color/scale/x/width` em TabChips (hover/select/press),
  sidebar mode picker, splitter, sliding underline animado sob a aba
  ativa, splash fade-in (200 ms), splitter hover/press tint, transições
  de largura de sidebar.
- Motion tokens centralizados em `Theme.qml` (motionFast 140 / motionStd
  220 / motionSlow 320 ms, easing OutCubic).

### Settings (5 abas)
- **Geral**: shell padrão, scrollback (10k/25k/50k/100k/250k linhas),
  restaurar abas no próximo launch.
- **Aparência**: window appearance (Sistema/Escuro/Claro), tema do
  terminal, fonte (nome + tamanho com slider live).
- **Voz**: API key Groq, modelo Whisper, idioma, submeter Enter automático.
- **AI Providers**: lista preview dos providers da toolbar (Claude/Gemini/
  Codex). Edição completa = backlog.
- **Atualizações**: versão atual, botão "Verificar agora", toggle
  auto-check, status (verde/amber) com botão "Atualizar agora" se houver.

### Tray icon
- Ícone "D" gerado programaticamente (procedural QPixmap, 64×64 com
  device pixel ratio 2 pra retina).
- Menu de contexto: Mostrar Dante CLI, Nova aba, Verificar atualizações,
  Sair.
- Notificação nativa quando autoupdate detecta nova versão.

### Autoupdate
- Manifest JSON em `https://dantetesta.com.br/dante-cli/winupdate.json`:
  ```json
  {
    "version":      "0.7.0-alpha.13",
    "url":          "https://dantetesta.com.br/dante-cli/Dante-CLI-Setup-...-x64.exe",
    "notes":        "Free-form changelog",
    "publishedAt":  "2026-05-26T10:00:00Z",
    "mandatory":    false
  }
  ```
- Check on launch (2 s delay) + a cada 4 h. Comparador semver-lite
  ordena `alpha < beta < rc < release`.
- Banner desliza do topo quando há update. "Atualizar" → download pra
  `%TEMP%` → spawn Inno com `/SILENT /CLOSEAPPLICATIONS /RESTARTAPPLICATIONS`
  → app fecha pra ser substituído.
- No Mac: abre o link no browser (não tem .exe pra silent-install).

### Persistência atômica
Tudo grava via `QSaveFile` (rename atômico) + debounce de 300–500 ms.
Arquivos em `%APPDATA%\Dante CLI\` (Windows) ou `~/Library/Application
Support/Dante CLI/` (Mac):

| Arquivo | Conteúdo |
|---|---|
| `session.json` | abas abertas + estado de split/grid por aba + aba ativa |
| `settings.json` | tema, fonte, shell, API key, scrollback, voz, autoupdate, etc. |
| `favorites.json` | pastas favoritas |
| `snippets.json` | comandos rápidos |
| `credentials.json` | credenciais com campos |
| `layout-templates.json` | layouts de grid salvos |
| `ai-history.json` | últimas 20 mensagens do AI |
| `logs/dante-YYYY-MM-DD.log` | log estruturado |

---

## Stack

| Camada | Tech |
|---|---|
| Linguagem | C++20 (MSVC `/W4 /permissive- /utf-8 /Zc:__cplusplus /EHsc`) |
| UI | Qt 6.5+ Quick + Controls + QML, Fusion style |
| Render terminal | `QQuickPaintedItem` custom |
| Build | CMake 3.21+, Ninja, AUTOMOC/AUTORCC |
| Net | `QNetworkAccessManager` (chat SSE + Whisper multipart) |
| Áudio | `QAudioSource` (cross-platform) — *WASAPI/AVFoundation são stubs* |
| Persistência | `QSaveFile` + `QJsonDocument`, debounced via `QTimer` |
| Installer | Inno Setup 6 (via `iscc`, instalado em CI por choco) |
| CI | GitHub Actions `windows-latest` + `jurplel/install-qt-action` |

---

## Estrutura de pastas

```text
Dante-CLI-Qt/
├─ CMakeLists.txt                  # raiz: Qt 6.5+, MSVC, C++20
├─ vcpkg.json                      # dependências opcionais
├─ core/                           # lib estática "dante-core" (sem UI)
│  ├─ domain/                      # Models.h, EventBus
│  ├─ persistence/                 # JsonStore (atomic save), AppPaths
│  ├─ terminal/                    # IPtyAdapter, TerminalSession,
│  │                               # TerminalRegistry, VTParser,
│  │                               # TerminalBuffer
│  ├─ themes/                      # TerminalScheme, ThemeRegistry (11 schemes)
│  ├─ ai/                          # GroqClient (chat + chatStream SSE +
│  │                               #             whisper multipart)
│  ├─ update/                      # UpdateChecker (HTTP + semver compare)
│  └─ telemetry/                   # Logger (file + stderr mirror)
├─ infra/                          # lib estática "dante-infra"
│  ├─ shell-adapters/              # ShellAdapter facade,
│  │                               # ConPtyAdapter (Win), UnixPtyAdapter (POSIX)
│  └─ audio-adapters/              # IVoiceCapture, QtVoiceCapture,
│                                  # WasapiVoiceCapture (stub),
│                                  # MacVoiceCapture (stub)
├─ apps/desktop-qt/                # executável principal
│  ├─ src/                         # AppState, *Model (4), TerminalBridge,
│  │                               # TerminalView, AIController,
│  │                               # PaletteController, VoiceController,
│  │                               # UpdateController, TrayController,
│  │                               # FileTreeController,
│  │                               # LayoutTemplatesController, main.cpp
│  └─ CMakeLists.txt
├─ ui/qml/                         # UI declarativa
│  ├─ Main.qml, Theme.qml (singleton)
│  ├─ TabBar / TabChip / Sidebar / Terminal / BottomToolbar
│  ├─ SplitContainer / PaneView    # 2-pane
│  ├─ GridWorkspace / EmptySlot    # multi-pane
│  ├─ LayoutDesigner / SettingsPanel
│  ├─ MovablePopup                 # base p/ modais movíveis
│  ├─ AIOverlay / AIMessage
│  ├─ CommandPalette / VoiceWidget
│  ├─ FileTreeView
│  ├─ SchemePicker / UpdateBanner / VideoOpenDialog
│  └─ editors/                     # EmojiPicker, FavoriteEditor,
│                                  # SnippetEditor, CredentialEditor
├─ installer/dante-cli.iss         # Inno Setup script
└─ .github/workflows/              # CI Windows (Qt + MSVC + Inno)
```

---

## Build

### Windows (recomendado: via CI)

Push pra `main` ou tag `v*` dispara
`.github/workflows/windows-build.yml`:

1. Instala Qt 6.7.2 (módulos `qtshadertools qtmultimedia`)
2. Configura MSVC via `ilammy/msvc-dev-cmd`
3. CMake + Ninja Release
4. `iscc` (instalado via chocolatey) → installer
5. Tag `v*` anexa o `.exe` ao GitHub Release

### Local

Pré-requisitos:
- **Windows**: Visual Studio 2022 + Qt 6.5+ (`qt-online-installer` →
  `MSVC 2019 64-bit + Qt Multimedia`), CMake 3.21+, Ninja, Inno Setup 6.
- **macOS**: `brew install qt cmake ninja`.

```sh
# macOS
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel
open "build/apps/desktop-qt/Dante CLI.app"
```

```cmd
:: Windows ("x64 Native Tools Command Prompt for VS 2022")
cmake -S . -B build -G Ninja ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.7.2\msvc2019_64"
cmake --build build --parallel
"build\apps\desktop-qt\Dante CLI.exe"

:: Gerar installer (opcional)
iscc /DAppVersion=0.7.0-alpha.12 /DSourceDir=build\apps\desktop-qt /DOutputDir=dist installer\dante-cli.iss
```

`windeployqt` roda como POST_BUILD no Windows, copiando as DLLs.

---

## Atalhos de teclado

| Atalho | Ação |
|---|---|
| `Ctrl+T` (`⌘T` Mac) | Nova aba |
| `Ctrl+W` | Fechar aba ativa |
| `Ctrl+Shift+]` / `[` | Próxima / anterior aba |
| `Ctrl+K` | Command palette (fuzzy) |
| `Ctrl+Shift+A` | Toggle AI overlay |
| `Ctrl+\` | Split vertical |
| `Ctrl+Shift+\` | Split horizontal |
| `Ctrl+Shift+W` | Fechar pane focado |
| `Ctrl+Alt+←` / `→` | Focar pane esquerdo/direito |
| `Esc` | Fechar modal aberto |

---

## Roadmap (TODO honesto)

- [ ] Browser embedded in-app (QtWebEngine, +150 MB no installer — LGPL c/ restrições)
- [ ] Player de vídeo embedded (QtMultimedia QML, +30 MB)
- [ ] WASAPI / AVFoundation com baixa latência (atualmente cai em `QtVoiceCapture`)
- [ ] AI Providers tab no Settings com edição completa de toolbar
- [ ] Drag-and-drop de tab → slot do grid
- [ ] Per-pane spawn config (working-dir, comando inicial)
- [ ] Reflow do scrollback no resize de janela
- [ ] Code signing EV pra silenciar SmartScreen
- [ ] App icon .icns próprio (mac) e .ico custom (atualmente programático no tray)
- [ ] Update sparkle-style com diff/patch (atualmente baixa installer inteiro)
- [ ] LayoutDesigner + SettingsPanel também usando `MovablePopup`
- [ ] Localizações além de pt-BR
- [ ] Testes (Catch2 ou GoogleTest) — `tests/` ainda vazio
- [ ] Telemetria opt-in
- [ ] Documentação de extensão (custom themes via JSON drop-in, plugin host)

---

## História do projeto

O Dante CLI nasceu em Swift pra macOS. Esta repo é o **port pra Windows**
em Qt — após tentativas anteriores em Avalonia/C# (v2.x) e Tauri/Rust
(v0.6.x) que esbarraram em problemas de PTY no Windows. O Qt foi
escolhido pra ter:

- Render UI consistente entre Win e Mac sem WebView (latência),
- PTY nativo cross-platform (ConPTY + forkpty),
- Custom scene-graph item pro terminal (50k linhas a 60 fps),
- LGPL com linkagem dinâmica (sem royalties pra closed-source).

A primeira release pública foi `v0.7.0-alpha.3` (~30 % do app Swift).
Hoje (`v0.7.0-alpha.12`) é ~70–80 % das features visíveis, faltando
principalmente embedding nativo de browser/vídeo e polimento da edição
de AI Providers no Settings.

---

## Licenciamento

- **Código próprio**: MIT.
- **Qt 6**: LGPLv3 — usado via linkagem dinâmica (DLLs separadas no
  Windows, frameworks no Mac). Distribuir o app exige preservar avisos
  LGPL e permitir ao usuário trocar as DLLs do Qt — esta build atende.
- **Inno Setup**: livre (Mozilla Public License).
- **Bibliotecas vcpkg** (opcionais): nlohmann/json (MIT), spdlog (MIT),
  simdjson (Apache 2.0), SQLite (domínio público).
- **Fontes Segoe/SF**: instaladas no SO do usuário, não redistribuídas.

---

## Stack inspirations

- `low-latency-trading-windows` skill — arquitetura de alta performance
  pra desktop Windows profissional (origem da decisão pelo Qt).
- `native-windows-premium-app` — princípios de polish premium pra apps
  desktop nativos.
- App oficial em Swift — referência visual e funcional (toolbar, sidebar,
  layout designer, voice HUD, settings tabs).

Co-Authored-By: Claude Opus 4.7.
