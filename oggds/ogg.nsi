; The name of the installer
Name "Ogg Vorbis DirectShow filter collection"

; The file to write
OutFile "OggDS.exe"

;License page
LicenseText "This installer will install the Ogg Vorbis DirectShow filter collection. Please read the license below."
LicenseData license.txt

; hide the "show details" box
ShowInstDetails show

SetOverwrite ifnewer 

; The stuff to install
Section "ThisNameIsIgnoredSoWhyBother?"
  SetOutPath $SYSDIR

  ; File to extract
  File "C:\WINDOWS\SYSTEM32\msvcr70.DLL"
  File "release\ogg.DLL"
  File "release\vorbis.DLL"
  File "release\vorbisenc.DLL"
  File "release\OggDS.DLL"
  Exec "regsvr32 $SYSDIR\OggDS.DLL /s"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OggDS" "DisplayName" "Direct Show Ogg Vorbis Filter (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OggDS" "UninstallString" '"$SYSDIR\OggDSuninst.exe"'
SectionEnd

UninstallText "This will uninstall Direct Show Ogg Vorbis Filter."
UninstallExeName "$SYSDIR\OggDSuninst.exe"

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OggDS"
  Exec "regsvr32 /u /s $SYSDIR\OggDS.DLL"
  Delete $SYSDIR\OggDS.DLL
  Delete $SYSDIR\OggDSUninst.exe
SectionEnd

; eof
