; Inno Setup Script for LA Studio
; See https://jrsoftware.org/isinfo.php for details

[Setup]
AppName=LA Studio
AppVersion=0.1.0
AppPublisher=LA Studio Team
AppPublisherURL=https://github.com/dduongtrandai/LA-Studio
AppSupportURL=https://github.com/dduongtrandai/LA-Studio/issues
AppUpdatesURL=https://github.com/dduongtrandai/LA-Studio/releases
DefaultDirName={autopf}\LA Studio
DefaultGroupName=LA Studio
DisableProgramGroupPage=yes
; Remove the comment below if you have a license file
; LicenseFile=..\LICENSE
OutputDir=..\out
OutputBaseFilename=LA-Studio-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\bin\LA Studio.exe
SetupIconFile=..\icons\app_icon.ico
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\out\stage\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\LA Studio"; Filename: "{app}\bin\LA Studio.exe"
Name: "{autodesktop}\LA Studio"; Filename: "{app}\bin\LA Studio.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\LA Studio.exe"; Description: "{cm:LaunchProgram,LA Studio}"; Flags: nowait postinstall skipifsilent
