; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Icecast2 Win32
AppVerName=Icecast v2.0 beta 3
AppPublisherURL=http://www.icecast.org
AppSupportURL=http://www.icecast.org
AppUpdatesURL=http://www.icecast.org
DefaultDirName={pf}\Icecast2 Win32
DefaultGroupName=Icecast2 Win32
AllowNoIcons=yes
LicenseFile=..\COPYING
InfoAfterFile=..\README
OutputDir=.
OutputBaseFilename=icecast2_win32_2.0_beta3_setup
WizardImageFile=icecast2logo2.bmp
; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Dirs]
Name: "{app}\web"
Name: "{app}\admin"
Name: "{app}\doc"
Name: "{app}\logs"


[Files]
Source: "Release\Icecast2.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Release\icecast2console.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\doc\icecast2.chm"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "..\web\status.xsl"; DestDir: "{app}\web"; Flags: ignoreversion
Source: "..\web\status2.xsl"; DestDir: "{app}\web"; Flags: ignoreversion
Source: "..\admin\listclients.xsl"; DestDir: "{app}\admin"; Flags: ignoreversion
Source: "..\admin\listmounts.xsl"; DestDir: "{app}\admin"; Flags: ignoreversion
Source: "..\admin\moveclients.xsl"; DestDir: "{app}\admin"; Flags: ignoreversion
Source: "..\admin\response.xsl"; DestDir: "{app}\admin"; Flags: ignoreversion
Source: "..\admin\stats.xsl"; DestDir: "{app}\admin"; Flags: ignoreversion
Source: "..\..\pthreads\pthreadVSE.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\conf\icecast.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\iconv\lib\iconv.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\libxslt\lib\libxslt.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\libxml2\lib\libxml2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\curl\lib\Release\libcurl.dll"; DestDir: "{app}"; Flags: ignoreversion

[Icons]

Name: "{group}\Icecast2 Win32"; Filename: "{app}\Icecast2.exe";WorkingDir: "{app}";
Name: "{userdesktop}\Icecast2 Win32"; Filename: "{app}\Icecast2.exe"; MinVersion: 4,4; Tasks: desktopicon;WorkingDir: "{app}";

[Run]

