#define AppName "Photo Snap"
#define AppPublisher "Photo Snap"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

[Setup]
AppId={{5FBB6D27-5BC4-4DC6-A238-BB6C7A5DBB82}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\photo-snap.exe
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
Source: "..\output\bin\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "..\resources\photosnap.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\photo-snap.exe"; IconFilename: "{app}\photosnap.ico"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\photo-snap.exe"; IconFilename: "{app}\photosnap.ico"; Tasks: desktopicon
