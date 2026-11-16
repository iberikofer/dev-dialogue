; Dev Dialogue - Windows installer.
; Build it with: scripts\package-desktop.ps1 (or: iscc installer\dev-dialogue.iss)
; Source files come from ..\dist\stage (see scripts\package-desktop.ps1).

#define AppName "Dev Dialogue"
#define AppVersion "1.0.0"
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
; Install for the current user by default (no admin rights), but let the user
; pick a machine-wide install (Program Files) from the wizard.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\dist\stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Leftovers from older versions (removed DLLs, plugins) should not pile up.
Type: filesandordirs; Name: "{app}\*"
