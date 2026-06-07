; Inno Setup Script for OverlayKit
; Defines the deployment packages, shortcuts, and startup options.

[Setup]
AppName=OverlayKit
AppVersion=1.0.0
AppPublisher=OverlayKit Team
DefaultDirName={autopf}\OverlayKit
DefaultGroupName=OverlayKit
DisableProgramGroupPage=yes
OutputBaseFilename=OverlayKitSetup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startup"; Description: "Run OverlayKit automatically when Windows starts"; GroupDescription: "Startup Options": Flags: unchecked

[Files]
Source: "..\build\overlaykit.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\OverlayKit"; Filename: "{app}\overlaykit.exe"
Name: "{autodesktop}\OverlayKit"; Filename: "{app}\overlaykit.exe"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "OverlayKit"; ValueData: """{app}\overlaykit.exe"""; Tasks: startup; Flags: uninsdeletevalue

[Run]
Filename: "{app}\overlaykit.exe"; Description: "{cm:LaunchProgram,OverlayKit}"; Flags: nowait postinstall skipifsilent
