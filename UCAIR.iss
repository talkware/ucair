; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{66C2290F-EFE0-4232-A5A5-D850BCDD7006}
AppName=UCAIR
AppVerName=UCAIR Build 20100404
AppPublisher=UIUC Timan Group
AppPublisherURL=http://code.google.com/p/ucair/
AppSupportURL=http://code.google.com/p/ucair/
AppUpdatesURL=http://code.google.com/p/ucair/
DefaultDirName={pf}\UCAIR
DefaultGroupName=UCAIR
OutputDir=setup
OutputBaseFilename=UCAIR-Setup
Compression=lzma
SolidCompression=yes

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; Flags: unchecked
Name: runatstartup; Description: Run at Windows start up

[Files]
Source: Release\UCAIR09.exe; DestDir: {app}; Flags: ignoreversion
Source: static_files\*; DestDir: {app}\static_files; Flags: ignoreversion recursesubdirs createallsubdirs
Source: system_files\*; DestDir: {app}\system_files; Flags: ignoreversion recursesubdirs createallsubdirs
Source: templates\*; DestDir: {app}\templates; Flags: ignoreversion recursesubdirs createallsubdirs
Source: user_files\*; DestDir: {app}\user_files; Flags: ignoreversion recursesubdirs createallsubdirs uninsneveruninstall onlyifdoesntexist
Source: config.ini; DestDir: {app}; Flags: ignoreversion
Source: default_pref.ini; DestDir: {app}; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: RunningOnWindows.URL; DestDir: {app}; Flags: ignoreversion

[Icons]
Name: {group}\UCAIR; Filename: {app}\UCAIR09.exe; WorkingDir: {app}
Name: {commondesktop}\UCAIR; Filename: {app}\UCAIR09.exe; Tasks: desktopicon; WorkingDir: {app}
Name: {commonstartup}\UCAIR; Filename: {app}\UCAIR09.exe; WorkingDir: {app}; Tasks: runatstartup

[Run]
Filename: {app}\UCAIR09.exe; Description: {cm:LaunchProgram,UCAIR}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: {app}\RunningOnWindows.URL; Description: Please read this first; Flags: nowait postinstall skipifsilent shellexec

[Dirs]
Name: {app}\; Permissions: everyone-modify
