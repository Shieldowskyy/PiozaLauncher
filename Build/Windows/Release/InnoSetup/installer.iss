#define MyAppName "Pioza Launcher"
#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#define MyAppPublisher "DashoGames"
#define MyAppURL "https://github.com/Shieldowskyy/PiozaLauncher"
#define MyAppExeName "PiozaGameLauncher.exe"

[Setup]
AppId={{1A744037-C332-47E6-AF81-6DE74F636069}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
LicenseFile=..\..\..\..\LICENSE
OutputDir=..\..\..\..
OutputBaseFilename=PiozaGL-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "openwebsite"; Description: "Visit project page on GitHub"; GroupDescription: "Additional Options"; Flags: unchecked

[Files]
Source: "..\..\..\..\build_temp\Windows\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\..\..\build_temp\Windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "Binaries\Win64\D3D12\D3D12Core.dll"
Source: "..\..\..\..\build_temp\Windows\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe"; DestDir: "{tmp}"; Flags: ignoreversion dontcopy

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCR; Subkey: "pioza"; ValueType: string; ValueName: ""; ValueData: "URL: Pioza Custom Protocol"; Flags: uninsdeletekey
Root: HKCR; Subkey: "pioza"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCR; Subkey: "pioza\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName}"; Flags: uninsdeletekey
Root: HKCR; Subkey: "pioza\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
Filename: "{app}\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe"; Parameters: "/quiet /q /silent /norestart"; Flags: nowait skipifsilent
Filename: "{cmd}"; Parameters: "/c start https://github.com/Shieldowskyy/PiozaGameLauncher"; Description: "Open project page on GitHub!"; Flags: postinstall skipifsilent unchecked
