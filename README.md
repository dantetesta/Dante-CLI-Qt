# Dante CLI — Qt port

Terminal premium para Windows, escrito em **C++20 + Qt 6 + QML**. Port do app
oficial em Swift (macOS) e do antigo MVP em Tauri.

> Status: **MVP em andamento (≈30–40 % do app Swift)**. UI e backbone funcionando,
> mas **sem ConPTY real** ainda (usa `QProcess` — apps tipo `vim`/`htop` não
> renderizam direito). Veja a seção "Gaps conhecidos" abaixo.

## Download (instalador Windows x64)

**Release atual:** [`v0.7.0-alpha.7`](https://github.com/dantetesta/Dante-CLI-Qt/releases/tag/v0.7.0-alpha.7) — 55 MB

[**▶ Baixar Dante-CLI-Setup-0.7.0-x64.exe**](https://github.com/dantetesta/Dante-CLI-Qt/releases/download/v0.7.0-alpha.7/Dante-CLI-Setup-0.7.0-x64.exe)

> Sem assinatura de código ainda — o SmartScreen do Windows vai pedir confirmação
> ("Mais informações → Executar assim mesmo"). Resolver isso exige certificado
> EV ou submeter o binário ao Microsoft Defender pra reputação.


---

## Por que mudei de stack

| Versão  | Stack             | Resultado                                                  |
|---------|-------------------|------------------------------------------------------------|
| v0.5.x  | Tauri + React + Rust | Funcionou; PTY no Win11 falhou com NUL bytes vazando para `CreateProcessW`. |
| v0.6.2  | Tauri + saneamento agressivo | Bug do NUL persistiu mesmo após dois patches. |
| **v0.7+** | **C++20 + Qt 6 + QML + CMake (MSVC)** | Stack nativa, sem camada extra, alinhada ao app Swift. |

A skill `low-latency-trading-windows` recomenda exatamente essa stack para apps
desktop premium no Windows, e foi seguida na arquitetura.

---

## Estrutura

```text
Dante-CLI-Qt/
├─ CMakeLists.txt                # raiz: Qt 6.5+, MSVC, C++20
├─ vcpkg.json                    # dependências (nlohmann/json, spdlog, …)
├─ core/                         # lib estática "dante-core"
│  ├─ domain/                    # Models, EventBus
│  ├─ persistence/               # AppPaths, JsonStore (atomic save)
│  ├─ terminal/                  # TerminalSession, TerminalRegistry, OSC parser
│  ├─ ai/                        # GroqClient (chat + Whisper)
│  └─ telemetry/                 # Logger
├─ infra/                        # lib estática "dante-infra"
│  └─ shell-adapters/            # ShellAdapter (QProcess MVP, ConPTY TODO)
├─ apps/desktop-qt/              # executável principal
│  ├─ src/                       # AppState, *Model, TerminalBridge, main.cpp
│  └─ CMakeLists.txt
├─ ui/qml/                       # Main.qml, TabBar, Sidebar, Terminal, Toolbar, Theme
├─ installer/dante-cli.iss       # Inno Setup
├─ .github/workflows/            # CI Windows (Qt + MSVC + Inno)
└─ resources/icons/              # app.ico / app.png
```

---

## Build local (Windows)

Pré-requisitos:

- Visual Studio 2022 (workload "Desktop development with C++")
- Qt 6.5+ Open Source (`qt-online-installer` → `MSVC 2019 64-bit`)
- CMake 3.21+, Ninja
- Inno Setup 6+ (apenas se quiser gerar o instalador localmente)

```cmd
:: dentro do "x64 Native Tools Command Prompt for VS 2022"
cmake -S . -B build -G "Ninja Multi-Config" ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.7.2\msvc2019_64"
cmake --build build --config Release --parallel
build\apps\desktop-qt\Release\Dante CLI.exe
```

`windeployqt` é chamado pelo CMake como POST_BUILD, copiando todas as DLLs
necessárias para a pasta do executável. Para gerar o instalador:

```cmd
iscc /DAppVersion=0.7.0 /DSourceDir=build\apps\desktop-qt\Release /DOutputDir=dist installer\dante-cli.iss
```

## Build no GitHub Actions

Push para `main` (ou tag `v*`) dispara `.github/workflows/windows-build.yml`:

1. Instala Qt 6.7.2 via `jurplel/install-qt-action`
2. Configura MSVC via `ilammy/msvc-dev-cmd`
3. Roda CMake/Ninja (Release)
4. Roda Inno Setup
5. Sobe os artefatos: `Dante-CLI-Setup-<v>-x64.exe` + portable

Para uma tag `vX.Y.Z`, o instalador é anexado ao GitHub Release automaticamente.

## Build no macOS (dev)

```sh
brew install qt cmake ninja
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel
open "build/apps/desktop-qt/Dante CLI.app"
```

---

## Persistência

Estado salvo em `%APPDATA%\Dante CLI\` (Win) / `~/Library/Application Support/Dante CLI/` (mac):

| Arquivo              | Conteúdo                                  |
|----------------------|-------------------------------------------|
| `session.json`       | abas abertas, aba ativa, layout           |
| `settings.json`      | tema, fonte, scrollback, API key Groq     |
| `favorites.json`     | pastas favoritas                          |
| `snippets.json`      | comandos rápidos                          |
| `credentials.json`   | credenciais (campos rotulados)            |
| `logs/dante-YYYY-MM-DD.log` | log estruturado                    |

Todas as gravações usam `QSaveFile` (atômicas) + debounce de 300–500 ms.

---

## Gaps conhecidos

Para sair de "MVP" para "paridade com Swift" ainda falta:

- [ ] **ConPTY real** — substituir `QProcess` em `ShellAdapter` por `CreatePseudoConsole` + `ReadFile`/`WriteFile` pipes. Sem isso, REPL funciona mas TUIs (vim, htop, fzf) não.
- [ ] Emulação VT100 / ANSI no `Terminal.qml` (atualmente um `TextArea` cru).
- [ ] Split panes (horizontal/vertical) e zoom de célula.
- [ ] AI Assistant flutuante (Groq chat) — backend pronto, falta UI.
- [ ] Gravação de voz (Whisper) — backend pronto, falta captura de áudio (WASAPI).
- [ ] Editor de credenciais / favoritos / snippets (atualmente só leitura/uso).
- [ ] 8+ esquemas de cor de terminal (atualmente 1 — Dark+).
- [ ] Pin/unpin, reordenação de abas por drag.
- [ ] Atalhos completos (split, navegar split, fechar split, palette ⌘K).
- [ ] Tray icon, autoupdate (winsparkle ou similar).
- [ ] Assinatura de código (EV / cert pago) para evitar SmartScreen warning.

A versão anterior em Tauri (`Dante-CLI-Tauri/`) ficou preservada como fallback
e contém o instalador `dist/Dante-CLI-Setup-0.6.2-x64.exe`.

---

## Licenciamento

- Código: MIT (próprio).
- Qt 6: **LGPLv3** — usado via *linkagem dinâmica* (DLLs separadas). Distribuir
  o app exige preservar avisos LGPL e permitir ao usuário trocar as DLLs do Qt.
  Esta build atende: as DLLs ficam ao lado do `.exe`, sem static linking.
- Inno Setup: livre (Mozilla Public License).
- Bibliotecas via vcpkg: nlohmann/json (MIT), spdlog (MIT), simdjson (Apache 2.0),
  SQLite (domínio público).
