#define MyAppName "uShader"
#define MyAppPublisher "SANDEFJORD DEVELOPMENT"
#define MyAppURL "https://github.com/Patrickjaillet/MicroShader"
#define MyAppExeName "ushader.exe"

#ifndef MyAppVersion
  #define MyAppVersion "0.0.0.0"
#endif

[Setup]
AppId={{ED47A87A-7963-428F-8433-0F4E02859B23}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\uShader
DefaultGroupName=uShader
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=uShader-Setup-{#MyAppVersion}
SetupIconFile=..\assets\icons\installer.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
LicenseFile=..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\build\ushader.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\assets\fonts\*"; DestDir: "{app}\assets\fonts"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\assets\branding\*"; DestDir: "{app}\assets\branding"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\uShader"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall uShader"; Filename: "{uninstallexe}"
Name: "{autodesktop}\uShader"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch uShader"; Flags: nowait postinstall skipifsilent
