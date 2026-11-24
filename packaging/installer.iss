#ifndef ReleaseVersion
  #define ReleaseVersion "0.1.0"
#endif

[Setup]
AppId=Spout2-iGPU-Capture
AppName=Spout2 iGPU Capture
AppVersion={#ReleaseVersion}
AppPublisher=chomiles
AppPublisherURL=https://github.com/chomiles/spout2-igpu-capture
AppSupportURL=https://github.com/chomiles/spout2-igpu-capture/issues
DefaultDirName={commonappdata}\obs-studio\plugins\warudo-spout-bridge
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
CloseApplicationsFilter=warudo-spout-bridge.dll
RestartApplications=no
OutputDir=..\dist
OutputBaseFilename=Spout2-iGPU-Capture-{#ReleaseVersion}-Setup-x64
Compression=lzma2
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Files]
Source: "..\build\Release\warudo-spout-bridge.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}\data"; DestName: "GPL-2.0.txt"; Flags: ignoreversion
Source: "..\third_party\spout\LICENSE"; DestDir: "{app}\data"; DestName: "Spout-LICENSE.txt"; Flags: ignoreversion
