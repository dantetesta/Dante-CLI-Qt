; Dante CLI — Inno Setup script
; AppVersion / SourceDir / OutputDir come from the CI workflow (-D flags).
; Local invocation: iscc /DAppVersion=0.7.0 /DSourceDir=..\build\Release /DOutputDir=..\dist installer\dante-cli.iss

#ifndef AppVersion
  #define AppVersion "0.7.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\deploy"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

#define MyAppName "Dante CLI"
#define MyAppPublisher "Dante Testa"
#define MyAppURL "https://dantetesta.com.br/dante-cli"
#define MyAppExeName "Dante CLI.exe"

[Setup]
AppId={{2A3F8C12-9B6E-4D71-A33F-CA9F0DD4E411}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppVerName={#MyAppName} {#AppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=Dante-CLI-Setup-{#AppVersion}-x64
#if FileExists(AddBackslash(SourcePath) + "..\resources\icons\app.ico")
SetupIconFile=..\resources\icons\app.ico
#endif
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}";    Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; Install the Visual C++ Redistributable (silent). Required for Qt 6 +
; MSVC builds — without it the app crashes immediately on launch with no
; error dialog on machines that don't already have it.
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; \
    StatusMsg: "Instalando Visual C++ Redistributable…"; \
    Check: NeedsVCRedist; \
    Flags: waituntilterminated

Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
function NeedsVCRedist: Boolean;
var
  Installed: Cardinal;
begin
  // VS 2015–2022 share the same runtime — the registry key is at
  //   HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64\Installed
  // == 1 when the redist is present.
  Result := True;
  if RegQueryDWordValue(
       HKEY_LOCAL_MACHINE,
       'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64',
       'Installed',
       Installed) then
  begin
    if Installed = 1 then Result := False;
  end;
end;
