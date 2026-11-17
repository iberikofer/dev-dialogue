; Dev Dialogue - Windows installer.
; Build it with: scripts\package-desktop.ps1 (or: iscc installer\dev-dialogue.iss)
; Source files come from ..\dist\stage (see scripts\package-desktop.ps1).

#define AppName "Dev Dialogue"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define AppPublisher "VNTU"
#define AppExeName "DevDialogue.exe"

[Setup]
AppId={{01a1e59f-4207-7f02-acc0-6d714b73f514}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=DevDialogue-Setup-{#AppVersion}-x64
SetupIconFile=..\apps\desktop\resources\app\app-icon.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
CloseApplications=force
CloseApplicationsFilter=DevDialogue.exe,DevDialogue.Api.exe
RestartApplications=yes
; Install for the current user by default (no admin rights), but let the user
; pick a machine-wide install (Program Files) from the wizard.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "installlocalai"; Description: "Download & configure local AI (gemma4:e4b) + Speech-to-Text (Parakeet TDT 0.6B v3) — required before starting interviews"; GroupDescription: "Local AI & Voice Models:"

[Files]
Source: "..\dist\stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "..\scripts\install-local-model.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "powershell.exe"; \
  Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\install-local-model.ps1"" -Model ""gemma4:e4b"" -SttDir ""{localappdata}\DevDialogue\models\parakeet-tdt-0.6b"""; \
  Description: "Downloading and setting up local AI (gemma4:e4b) and Speech-to-Text (Parakeet TDT 0.6B v3)..."; \
  Tasks: installlocalai; \
  Flags: waituntilterminated
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[Code]
function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
  MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
var
  Info: String;
begin
  Info := '';
  if MemoDirInfo <> '' then
    Info := Info + MemoDirInfo + NewLine + NewLine;
  if MemoTasksInfo <> '' then
    Info := Info + MemoTasksInfo + NewLine + NewLine;
  Info := Info + 'Local AI & Speech-to-Text Models Notice:' + NewLine +
    Space + 'Dev Dialogue requires both gemma4:e4b (Ollama, ~5.5 GB) and Parakeet TDT 0.6B v3 STT (~650 MB).' + NewLine +
    Space + 'If skipped during setup, the app will require downloading both before starting an interview.' + NewLine;
  Result := Info;
end;

[UninstallDelete]
; Leftovers from older versions (removed DLLs, plugins) should not pile up.
Type: filesandordirs; Name: "{app}\*"
