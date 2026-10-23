; Subtitle Generator AI - Windows installer (Inno Setup 6)
; Built by scripts/package-windows.sh, which bundles Qt, the compiler runtime
; and FFmpeg into the source folder first, so the installed app needs nothing
; else on the user's PC.

#ifndef MyAppVersion
  #define MyAppVersion "2.0"
#endif
#ifndef MyAppSourceDir
  #define MyAppSourceDir "build-release\bin"
#endif
#define MyAppName "Subtitle Generator AI"
#define MyAppPublisher "Inbora Studio"
#define MyAppExeName "SubtitleGeneratorAI.exe"
#define MyAppURL "https://github.com/InboraStudio/Subtitle-Generator-AI"
#define MyDiscordURL "https://discord.gg/pVA8NZ5mru"

[Setup]
; Keep this AppId forever so new versions upgrade in place.
AppId={{F3A1C2D4-8B5E-4F7A-9C6D-2E0B1A3F4D8C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyDiscordURL}
AppUpdatesURL={#MyAppURL}/releases/latest
AppCopyright=Copyright (C) Inbora Studio

; Per-user install by default: no administrator prompt, and the app can
; always write its logs. Users may still pick "all users" on the first page.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=auto
DisableReadyPage=yes
AllowNoIcons=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

; Upgrades: close a running copy instead of failing on locked files.
CloseApplications=yes
RestartApplications=no

OutputDir=installer_output
OutputBaseFilename=SubtitleGeneratorAI_Setup
SetupIconFile=resources\icons\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes
WizardStyle=modern
WizardSizePercent=100

VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Setup
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel2=This will install [name/ver] by Inbora Studio.%n%nEverything it needs (AI engine, FFmpeg, runtime) is included - no extra downloads or setup.%n%nAfter installing, pick a Whisper model in the app's Model Manager (one-time download).

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
Source: "{#MyAppSourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Inbora Studio Discord"; Filename: "{#MyDiscordURL}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\logs"
