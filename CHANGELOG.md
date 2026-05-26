# Changelog

Todas as mudanças notáveis seguem [Keep a Changelog](https://keepachangelog.com/)
e o projeto usa [Semantic Versioning](https://semver.org/).

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
