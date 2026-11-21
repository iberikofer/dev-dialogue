; Dev Dialogue - online bootstrapper.
;
; A small installer that does not contain the application: it downloads the
; real installer and runs it. Build it with scripts\package-desktop.ps1, which
; passes the payload location and its SHA-256 via ISCC defines:
;
;   ISCC /DPayloadUrl="..." /DPayloadSha256="..." /DPayloadSize=... installer\dev-dialogue-bootstrap.iss
;
; Overridable defaults (edit or pass via ISCC):
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef PayloadUrl
  #define PayloadUrl "https://github.com/iberikofer/dev-dialogue/releases/download/latest/DevDialogue-Setup-x64.exe"
#endif
#ifndef PayloadSha256
  #define PayloadSha256 ""
#endif
#ifndef PayloadSize
  #define PayloadSize "120000000"
#endif
#ifndef PayloadFile
  #define PayloadFile "DevDialogue-Setup-x64.exe"
#endif
#ifndef PayloadArgs
  #define PayloadArgs ""
#endif

[Setup]
AppId={{2B0B1F3C-9A41-4E7B-9C6E-5D1A7F2E4B80}
AppName=Dev Dialogue
AppVersion={#AppVersion}
AppPublisher=VNTU
DefaultDirName={tmp}\DevDialogueBootstrap
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableFinishedPage=yes
Uninstallable=no
OutputDir=..\dist
OutputBaseFilename=DevDialogue-Bootstrap-{#AppVersion}-x64
SetupIconFile=..\apps\desktop\resources\app\app-icon.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
SetupLogging=yes

[Files]
; The application itself is fetched during the install step; Hash pins the
; SHA-256 of the payload, so a tampered download is rejected.
Source: "{#PayloadUrl}"; DestName: "{#PayloadFile}"; DestDir: "{app}"; \
  Flags: external download ignoreversion; \
  Hash: "{#PayloadSha256}"; \
  ExternalSize: {#PayloadSize}

[Run]
Filename: "{app}\{#PayloadFile}"; Parameters: "{code:PayloadParams}"; \
  Flags: nowait postinstall

[Code]
function PayloadParams(Param: String): String;
var
  Args: String;
begin
  { Inno does not expose its own switches (/VERYSILENT and friends) to the
    script, but it does expose user defined ones. Everything passed as
    /payloadargs="..." is handed over to the real installer, e.g.
    /payloadargs="/VERYSILENT /SUPPRESSMSGBOXES /NORESTART" }
  Args := ExpandConstant('{param:payloadargs}');
  if (Length(Args) > 1) and (Args[1] = '"') and (Args[Length(Args)] = '"') then
    Args := Copy(Args, 2, Length(Args) - 2);
  if Args = '' then
    Args := '{#PayloadArgs}';
  Result := Args;
end;
