#define AppName "Photo Snap"
#define AppPublisher "Photo Snap"
#ifndef AppVersion
  #define AppVersion "1.1.0"
#endif

[Setup]
AppId={{5FBB6D27-5BC4-4DC6-A238-BB6C7A5DBB82}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\bin\photo-snap.exe
OutputDir=..\output
OutputBaseFilename=Photo-Snap-Setup
SetupIconFile=..\resources\photosnap.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
Source: "..\output\bin\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs
Source: "..\output\plugins\*"; DestDir: "{app}\plugins"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\resources\photosnap.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\bin\photo-snap.exe"; IconFilename: "{app}\photosnap.ico"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\bin\photo-snap.exe"; IconFilename: "{app}\photosnap.ico"; Tasks: desktopicon
