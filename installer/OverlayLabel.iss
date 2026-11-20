; Inno Setup Script for Overlay Label
; Build with Inno Setup 6 (ISCC.exe)

#define MyAppName "Overlay Label"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "minhhan.net"
#define MyAppURL "https://minhhan.net"
#define MyAppExeName "OverlayLabel.exe"

[Setup]
AppId={{B8F4C2A1-9E3D-4A7B-8C5F-2D1E6A9B4C01}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
VersionInfoVersion=1.0.0.0
VersionInfoProductVersion=1.0.0.0
VersionInfoProductName={#MyAppName}
VersionInfoDescription={#MyAppName} Setup
VersionInfoCompany={#MyAppPublisher}
VersionInfoCopyright=Copyright (C) 2026 {#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=OverlayLabel-Setup-{#MyAppVersion}
SetupIconFile=..\resources\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
CloseApplications=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut (Tao bieu tuong Desktop)"; GroupDescription: "Additional options:"
Name: "autostart"; Description: "Start Overlay Label automatically with Windows (Khoi dong cung Windows)"; GroupDescription: "Additional options:"

[Files]
Source: "..\build\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "OverlayLabel"; ValueData: """{app}\{#MyAppExeName}"""; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName} now"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\OverlayLabel.old.exe"

[Code]
procedure CloseRunningOverlayLabel();
var
  Wnd: HWND;
  Tries: Integer;
begin
  Wnd := FindWindowByClassName('OverlayLabelControllerClass');
  if Wnd <> 0 then
  begin
    PostMessage(Wnd, $0010, 0, 0); // WM_CLOSE
    Tries := 0;
    while (FindWindowByClassName('OverlayLabelControllerClass') <> 0) and (Tries < 30) do
    begin
      Sleep(100);
      Tries := Tries + 1;
    end;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  CloseRunningOverlayLabel();
  Result := '';
end;

function InitializeUninstall(): Boolean;
begin
  CloseRunningOverlayLabel();
  Result := True;
end;
