; PotatoClient installer (Inno Setup 6)
; Build with installer\build-installer.ps1, or: iscc installer\PotatoClient.iss
; Inputs: build\Release\client.dll and build\Release\injector.exe

; AppVersion = the release (CI passes /DAppVersion=1.2.3 from the v1.2.3 tag), GameVersion = the supported game.
; Running a newer Setup over an install updates it in place (same AppId)
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#define GameVersion "1.26.52"

[Setup]
AppId={{7E3B6A52-4C1D-4F0B-9A8E-5D2C1B0A9F31}
AppName=PotatoClient
AppVersion={#AppVersion}
AppVerName=PotatoClient {#AppVersion} (Minecraft Bedrock {#GameVersion})
VersionInfoVersion={#AppVersion}
AppPublisher=PotatoClient contributors
AppPublisherURL=https://github.com/meihao550/potatoClient
; Per-user install: injector.exe writes client_loaded_*.dll next to client.dll, so the folder must be writable
PrivilegesRequired=lowest
DefaultDirName={localappdata}\Programs\PotatoClient
DefaultGroupName=PotatoClient
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile=..\LICENSE
InfoBeforeFile=README.txt
OutputDir=Output
OutputBaseFilename=PotatoClient-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\injector.exe

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\build\Release\client.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\injector.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\PotatoClient Injector"; Filename: "{app}\injector.exe"; WorkingDir: "{app}"
Name: "{group}\README"; Filename: "{app}\README.txt"
Name: "{group}\{cm:UninstallProgram,PotatoClient}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\PotatoClient Injector"; Filename: "{app}\injector.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[UninstallDelete]
; Copies made by injector.exe at inject time
Type: files; Name: "{app}\client_loaded_*.dll"
