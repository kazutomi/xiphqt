; Script generated by the HM NIS Edit Script Wizard.

; Location of Visual Studio runtime libraries on the compiling system
;   ************* Change this to match the path where msvcp71.dll and msvcr71.dll live ******************
; !define VS_RUNTIME_LOCATION "c:\Program Files\Microsoft Visual Studio .NET 2003\SDK\v1.1\Bin"


; !define VS_RUNTIME_LOCATION "C:\Program Files\Microsoft Visual Studio 8\VC\redist\x86\Microsoft.VC80.CRT"
; !define VS_RUNTIME_LOCATION_PREFIX "C:\Program Files\Microsoft Visual Studio 8\VC\redist\x86\Microsoft.VC80.CRT\MSVC"


;  To use the unicows enabled versions, use these rebuilt crt's

!define VS_RUNTIME_LOCATION ..\..\..\bin
!define VS_RUNTIME_LOCATION_PREFIX ..\..\..\bin\MSLU
;   *****************************************************************************************************





; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "oggcodecs"

;	CHANGE EVERY VERSION
!define PRODUCT_VERSION "0.72.1638"					

!define PRODUCT_PUBLISHER "illiminable"
!define PRODUCT_WEB_SITE "http://www.illiminable.com/ogg/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\OOOggDump.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"


; Path from .nsi to oggcodecs root
!define OGGCODECS_ROOT_DIR "..\..\.."

; Local Build Path for configuration
!define OGGCODECS_CONFIG_PATH "Release"
!define OGGCODECS_VORBIS_CONFIG_PATH "Vorbis_Dynamic_Release"






SetCompressor lzma






; MUI 1.67 compatible ------
!include "MUI.nsh"

; Include for library registration
!include "Library.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!define MUI_LICENSEPAGE_CHECKBOX
!insertmacro MUI_PAGE_LICENSE "${OGGCODECS_ROOT_DIR}\COPYRIGHTS.rtf"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; COMPONENTS
!insertmacro MUI_PAGE_COMPONENTS

; Start menu page
var ICONS_GROUP
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "illiminable\oggcodecs"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Polish"
; !insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Turkish"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------







Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "oggcodecs_${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\illiminable\oggcodecs"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show







Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY

  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the existing version or `Cancel` to cancel this installation." IDOK uninst
  Abort

;Run the uninstaller
uninst:
  ClearErrors
  ; Copy the uninstaller to a temp location
  GetTempFileName $0
  CopyFiles $R0 $0
  ;Start the uninstaller using the option to not copy itself
  ExecWait '$0 _?=$INSTDIR'
 
  IfErrors no_remove_uninstaller
    ; In most cases the uninstall is successful at this point.
    ; You may also consider using a registry key to check whether 
    ; the user has chosen to uninstall. If you are using an uninstaller
    ; components page, make sure all sections are uninstalled.
    goto done
  no_remove_uninstaller:
    MessageBox MB_ICONEXCLAMATION \
    "Unable to remove previous version of ${PRODUCT_NAME}"
    Abort
  
done:
  ; remove the copied uninstaller
  Delete '$0'

FunctionEnd






Section "Oggcodecs Core Files" SEC_CORE
  SectionIn 1 RO

  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Runtime libraries from visual studio - 3
  File "${VS_RUNTIME_LOCATION_PREFIX}r80.dll"
  File "${VS_RUNTIME_LOCATION_PREFIX}p80.dll"
  File "${VS_RUNTIME_LOCATION}\Microsoft.VC80.CRT.manifest"

  ; Unicows for old windows with no unicode - 1
  File "${VS_RUNTIME_LOCATION}\unicows.dll"


  ; Libraries - 11
  File "${OGGCODECS_ROOT_DIR}\src\lib\core\ogg\libOOOgg\${OGGCODECS_CONFIG_PATH}\libOOOgg.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\core\ogg\libOOOggSeek\${OGGCODECS_CONFIG_PATH}\libOOOggSeek.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\cmml\libCMMLTags\${OGGCODECS_CONFIG_PATH}\libCMMLTags.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\cmml\libCMMLParse\${OGGCODECS_CONFIG_PATH}\libCMMLParse.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\vorbis\libs\libvorbis\win32\${OGGCODECS_VORBIS_CONFIG_PATH}\vorbis.dll"
 
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\theora\libs\libOOTheora\${OGGCODECS_CONFIG_PATH}\libOOTheora.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\flac\libs\libflac\obj\${OGGCODECS_CONFIG_PATH}\bin\libFLAC.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\flac\libs\libflac\obj\${OGGCODECS_CONFIG_PATH}\bin\libFLAC++.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\helper\libfishsound\win32\${OGGCODECS_CONFIG_PATH}\libfishsound.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\core\ogg\libVorbisComment\${OGGCODECS_CONFIG_PATH}\libVorbisComment.dll"

  File "${OGGCODECS_ROOT_DIR}\src\lib\helper\libTemporalURI\${OGGCODECS_CONFIG_PATH}\libTemporalURI.dll"



  ; Utilites - 4
  File "${OGGCODECS_ROOT_DIR}\src\tools\OOOggDump\${OGGCODECS_CONFIG_PATH}\OOOggDump.exe"
  File "${OGGCODECS_ROOT_DIR}\src\tools\OOOggStat\${OGGCODECS_CONFIG_PATH}\OOOggStat.exe"
  File "${OGGCODECS_ROOT_DIR}\src\tools\OOOggValidate\${OGGCODECS_CONFIG_PATH}\OOOggValidate.exe"
  File "${OGGCODECS_ROOT_DIR}\src\tools\OOOggCommentDump\${OGGCODECS_CONFIG_PATH}\OOOggCommentDump.exe"


  ; Text files - 7
  File "${OGGCODECS_ROOT_DIR}\ABOUT.txt"
  File "${OGGCODECS_ROOT_DIR}\VERSIONS"
  File "${OGGCODECS_ROOT_DIR}\README"
  File "${OGGCODECS_ROOT_DIR}\COPYRIGHTS.rtf"
  File "${OGGCODECS_ROOT_DIR}\COPYRIGHTS"

  File "${OGGCODECS_ROOT_DIR}\AUTHORS"
  File "${OGGCODECS_ROOT_DIR}\HISTORY"


  ; Install Filters - 16
  

  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\flac\filters\dsfFLACEncoder\${OGGCODECS_CONFIG_PATH}\dsfFLACEncoder.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\speex\filters\dsfSpeexEncoder\${OGGCODECS_CONFIG_PATH}\dsfSpeexEncoder.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\theora\filters\dsfTheoraEncoder\${OGGCODECS_CONFIG_PATH}\dsfTheoraEncoder.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\vorbis\filters\dsfVorbisEncoder\${OGGCODECS_CONFIG_PATH}\dsfVorbisEncoder.dll"

  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\flac\filters\dsfNativeFLACSource\${OGGCODECS_CONFIG_PATH}\dsfNativeFLACSource.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\speex\filters\dsfSpeexDecoder\${OGGCODECS_CONFIG_PATH}\dsfSpeexDecoder.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\theora\filters\dsfTheoraDecoder\${OGGCODECS_CONFIG_PATH}\dsfTheoraDecoder.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\flac\filters\dsfFLACDecoder\${OGGCODECS_CONFIG_PATH}\dsfFLACDecoder.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\vorbis\filters\dsfVorbisDecoder\${OGGCODECS_CONFIG_PATH}\dsfVorbisDecoder.dll"

  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\ogm\filters\dsfOGMDecoder\${OGGCODECS_CONFIG_PATH}\dsfOGMDecoder.dll"

  File "${OGGCODECS_ROOT_DIR}\src\lib\core\directshow\dsfOggDemux2\${OGGCODECS_CONFIG_PATH}\dsfOggDemux2.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\core\directshow\dsfOggMux\${OGGCODECS_CONFIG_PATH}\dsfOggMux.dll"

  ; File "${OGGCODECS_ROOT_DIR}\src\lib\core\directshow\dsfSeeking\${OGGCODECS_CONFIG_PATH}\dsfSeeking.dll"

  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\cmml\dsfCMMLDecoder\${OGGCODECS_CONFIG_PATH}\dsfCMMLDecoder.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\codecs\cmml\dsfCMMLRawSource\${OGGCODECS_CONFIG_PATH}\dsfCMMLRawSource.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\core\directshow\dsfSubtitleVMR9\${OGGCODECS_CONFIG_PATH}\dsfSubtitleVMR9.dll"

  ; File "${OGGCODECS_ROOT_DIR}\src\lib\core\directshow\dsfAnxDemux\${OGGCODECS_CONFIG_PATH}\dsfAnxDemux.dll"
  File "${OGGCODECS_ROOT_DIR}\src\lib\core\directshow\dsfAnxMux\${OGGCODECS_CONFIG_PATH}\dsfAnxMux.dll"

  ; Register libraries - 16

  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfFLACEncoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfSpeexEncoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfTheoraEncoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfVorbisEncoder.dll"'
  
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfNativeFLACSource.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfSpeexDecoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfTheoraDecoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfFLACDecoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfVorbisDecoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfOGMDecoder.dll"'

  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfOggDemux2.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfOggMux.dll"'

  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfCMMLDecoder.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfCMMLRawSource.dll"'
  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfSubtitleVMR9.dll"'

  ExecWait 'regsvr32 "/s" "$INSTDIR\dsfAnxMux.dll"'

  ; ExecWait 'regsvr32 "/s" "$INSTDIR\dsfAnxDemux.dll"'
  
  
  
  
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Registry Entries for directshow and WMP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;	*	Media Group Entries for WMP
;;;			-	flac (audio)
;;;			-	oga
;;;			-	ogv
;;;			-	axa
;;;			-	axv
;;;			-	spx
;;;			-	ogm(????? TODO:::)
;;;			-	ogg(TODO::: Check if can have no group)
;;;	*	Mime Type Entries for WMP
;;;	*	Extension Entries for WMP - TODO::: Other entries, icons
;;;	*	Media Type Entries/Filter association for Directshow
;;;	*	MLS(?) Entries for WMP






;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Media Group Entries - 6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\FLAC" "" "FLAC File (flac)"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\FLAC" "Extensions" ".flac"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\FLAC" "MIME Types" "audio/x-flac"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\OGA" "" "Ogg File (oga)"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\OGA" "Extensions" ".oga"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\OGA" "MIME Types" "audio/x-ogg"
  
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\OGV" "" "Ogg File (ogv)"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\OGV" "Extensions" ".ogv"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\OGV" "MIME Types" "video/x-ogg"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\AXA" "" "Annodex File (axa)"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\AXA" "Extensions" ".axa"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\AXA" "MIME Types" "audio/x-annodex"
  
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\AXV" "" "Annodex File (axv)"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\AXV" "Extensions" ".axv"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\AXV" "MIME Types" "video/x-annodex"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\SPX" "" "Ogg File (spx)"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\SPX" "Extensions" ".spx"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\SPX" "MIME Types" "audio/x-ogg"
  
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;	WMP Mime type entries - 7
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;





  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/x-annodex" "" "Annodex File"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/x-annodex" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/x-annodex" "Extension.Key" ".anx"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/x-annodex" "Extensions.CommaSep" "anx,axa,axv"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/x-annodex" "Extensions.SpaceSep" ".anx .axa .axv"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/ogg" "" "Ogg File"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/ogg" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/ogg" "Extension.Key" ".ogg"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/ogg" "Extensions.CommaSep" "ogg,oga,ogv,spx"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/ogg" "Extensions.SpaceSep" ".ogg .oga .ogv .spx"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-flac" "" "FLAC Audio File"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-flac" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-flac" "Extension.Key" ".flac"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-ogg" "" "Ogg Audio File"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-ogg" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-ogg" "Extension.Key" ".oga"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-ogg" "Extensions.CommaSep" "oga,spx"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-ogg" "Extensions.SpaceSep" ".oga .spx"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-ogg" "" "Ogg Video File"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-ogg" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-ogg" "Extension.Key" ".ogv"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-annodex" "" "Annodex Audio File"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-annodex" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-annodex" "Extension.Key" ".axa"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-annodex" "Extensions.CommaSep" "axa"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-annodex" "Extensions.SpaceSep" ".axa"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-annodex" "" "Annodex Video File"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-annodex" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-annodex" "Extension.Key" ".axv"
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;	WMP extension entries - 8
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.anx" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.anx" "MediaType.Description" "Annodex File"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.anx" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.anx" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.anx" "Extension.MIME" "application/x-annodex"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axa" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axa" "MediaType.Description" "Annodex File"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axa" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axa" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axa" "PerceivedType" "audio"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axa" "Extension.MIME" "audio/x-annodex"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axv" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axv" "MediaType.Description" "Annodex File"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axv" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axv" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axv" "PerceivedType" "video"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axv" "Extension.MIME" "video/x-annodex"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.flac" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.flac" "MediaType.Description" "FLAC Audio"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.flac" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.flac" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.flac" "PerceivedType" "audio"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.flac" "Extension.MIME" "audio/x-flac"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.oga" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.oga" "MediaType.Description" "Ogg Audio"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.oga" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.oga" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.oga" "PerceivedType" "audio"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.oga" "Extension.MIME" "audio/x-ogg"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogg" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogg" "MediaType.Description" "Ogg File"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogg" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogg" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogg" "Extension.MIME" "application/ogg"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogv" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogv" "MediaType.Description" "Ogg Video"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogv" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogv" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogv" "PerceivedType" "video"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogv" "Extension.MIME" "video/x-ogg"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.spx" "AlreadyRegistered" "yes"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.spx" "MediaType.Description" "Ogg Speex Audio"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.spx" "Permissions" 0x0000000f
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.spx" "Runtime" 0x00000007
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.spx" "PerceivedType" "audio"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.spx" "Extension.MIME" "audio/x-ogg"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;





;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;	Directshow extension to filter mapping - 8
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;






  WriteRegStr HKCR "Media Type\Extensions\.anx" "Source Filter" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKCR "Media Type\Extensions\.axa" "Source Filter" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKCR "Media Type\Extensions\.axv" "Source Filter" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKCR "Media Type\Extensions\.flac" "Source Filter" "{6DDA37BA-0553-499a-AE0D-BEBA67204548}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKCR "Media Type\Extensions\.oga" "Source Filter" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKCR "Media Type\Extensions\.ogg" "Source Filter" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKCR "Media Type\Extensions\.ogv" "Source Filter" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKCR "Media Type\Extensions\.spx" "Source Filter" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"





;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;	Directshow extension to filter mapping for HTTP - 7
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



  WriteRegStr HKCR "http\Extensions" ".OGG" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"
  WriteRegStr HKCR "http\Extensions" ".OGV" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"
  WriteRegStr HKCR "http\Extensions" ".OGA" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"
  WriteRegStr HKCR "http\Extensions" ".SPX" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"
  WriteRegStr HKCR "http\Extensions" ".ANX" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"
  WriteRegStr HKCR "http\Extensions" ".AXV" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"
  WriteRegStr HKCR "http\Extensions" ".AXA" "{C9361F5A-3282-4944-9899-6D99CDC5370B}"
  
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;	MLS Perceived type - 6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  WriteRegStr HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "ogv" "video"
  WriteRegStr HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "oga" "audio"
  WriteRegStr HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "axv" "video"
  WriteRegStr HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "axa" "audio"
  WriteRegStr HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "spx" "audio"
  WriteRegStr HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "flac" "audio"  
  
  
  
  
  
  
  



  ;Sleep 10000
; Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd



Section ".ogg defaults to audio" SEC_OGG_AUDIO_DEFAULT
  SectionIn 1
  
  
  ; Make .ogg recognised as audio
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\OGG" "" "Ogg File (ogg)"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\OGG" "Extensions" ".ogg"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\OGG" "MIME Types" "application/ogg"  
  
  
  WriteRegStr HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogg" "PerceivedType" "audio"
  
  
  WriteRegStr HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "ogg" "audio"  
SectionEnd

Section "Open Ogg files with WMP" SEC_USE_WMP_FOR_OGG

  SectionIn 1
  Var /GLOBAL WMP_LOCATION

  
 
  ReadRegStr $WMP_LOCATION HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer" "Player.Path"
  StrCmp $WMP_LOCATION "" fail_wmp 0
  
  ; Point the extension to the handlers
  WriteRegStr HKCR ".ogg" "" "WMP.OggFile"
  WriteRegStr HKCR ".oga" "" "WMP.OgaFile"
  WriteRegStr HKCR ".ogv" "" "WMP.OgvFile"
  
  
  ; Handler key for ogg
  WriteRegStr HKCR "WMP.OggFile" "" "Ogg File"
  WriteRegStr HKCR "WMP.OggFile\shell" "" "open"
  WriteRegStr HKCR "WMP.OggFile\shell\open" "" "&Open"
  WriteRegStr HKCR "WMP.OggFile\shell\open\command" "" "$WMP_LOCATION /Open $\"%L$\""
  
  WriteRegStr HKCR "WMP.OggFile\shell\play" "" "&Play"
  WriteRegStr HKCR "WMP.OggFile\shell\play\command" "" "$WMP_LOCATION /Play $\"%L$\""    
  
  ; Handler key for oga
  WriteRegStr HKCR "WMP.OgaFile" "" "Oga File"
  WriteRegStr HKCR "WMP.OgaFile\shell" "" "open"
  WriteRegStr HKCR "WMP.OgaFile\shell\open" "" "&Open"
  WriteRegStr HKCR "WMP.OgaFile\shell\open\command" "" "$WMP_LOCATION /Open $\"%L$\""
  
  WriteRegStr HKCR "WMP.OgaFile\shell\play" "" "&Play"
  WriteRegStr HKCR "WMP.OgaFile\shell\play\command" "" "$WMP_LOCATION /Play $\"%L$\""    
  
  ; Handler key for ogv
  WriteRegStr HKCR "WMP.OgvFile" "" "Ogv File"
  WriteRegStr HKCR "WMP.OgvFile\shell" "" "open"
  WriteRegStr HKCR "WMP.OgvFile\shell\open" "" "&Open"
  WriteRegStr HKCR "WMP.OgvFile\shell\open\command" "" "$WMP_LOCATION /Open $\"%L$\""
  
  WriteRegStr HKCR "WMP.OgvFile\shell\play" "" "&Play"
  WriteRegStr HKCR "WMP.OgvFile\shell\play\command" "" "$WMP_LOCATION /Play $\"%L$\""    
   
  goto done_wmp
  
fail_wmp:
MessageBox MB_OK|MB_ICONEXCLAMATION "A recognised version of Windows Media Player was not found. $\n File extenstion association must be done manually." IDOK done_wmp

done_wmp:
  
  
SectionEnd




LangString DESC_OggCoreSection ${LANG_ENGLISH} "Core files for oggcodecs"
LangString DESC_OggExtensionAudioByDefault ${LANG_ENGLISH} "Makes files with .ogg extension default to the audio section in Windows Media Player Library. Note: This means that ogg theora files with .ogg extension will also be in audio section. .ogv defaults to video."
LangString DESC_OggOpensInWMP ${LANG_ENGLISH} "Associates Ogg Files with Windows Media Player, so you can double click them in explorer. Uncheck this if you don't want to use WMP for ogg files."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CORE} $(DESC_OggCoreSection)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_OGG_AUDIO_DEFAULT} $(DESC_OggExtensionAudioByDefault)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_USE_WMP_FOR_OGG} ${DESC_OggOpensInWMP}
!insertmacro MUI_FUNCTION_DESCRIPTION_END



Section -AdditionalIcons
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\uninst.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd







Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\OOOggDump.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\OOOggDump.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"



  
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SectionEnd



Function un.onUninstSuccess
;  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd



Function un.onInit
!insertmacro MUI_UNGETLANGUAGE
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd



Section Uninstall


  ; Unregister libraries - 16

  ; Unregister core annodex libraries
  
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfSubtitleVMR9.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfCMMLDecoder.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfCMMLRawSource.dll"'
  
  ; ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfAnxDemux.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfAnxMux.dll"'

  
  ; Unregister core ogg libraries
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfOggDemux2.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfOggMux.dll"'


  ; Unregister encoders
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfFLACEncoder.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfSpeexEncoder.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfTheoraEncoder.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfVorbisEncoder.dll"'

  
  ; Unregister decoders
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfNativeFLACSource.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfSpeexDecoder.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfTheoraDecoder.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfFLACDecoder.dll"'
  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfVorbisDecoder.dll"'

  ExecWait 'regsvr32 "/s" "/u" "$INSTDIR\dsfOGMDecoder.dll"'





; Get rid of all the registry keys we made for directshow and WMP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  ; Media Type Groups entries - 6

  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\FLAC"  
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\OGA"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\SPX"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Audio\AXA"

  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\OGV"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Groups\Video\AXV"


  ; MIME Type entries	- 7

  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/ogg"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-flac"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-ogg"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-ogg"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\application/x-annodex"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\audio/x-annodex"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\MIME Types\video/x-annodex"


  ; File Extension Entries - 8

  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.flac"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.oga"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogg"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.ogv"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.spx"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.anx"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axa"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions\.axv"

  
  ; Extension to filter mapping - 8

  DeleteRegKey HKCR "Media Type\Extensions\.anx"
  DeleteRegKey HKCR "Media Type\Extensions\.axa"
  DeleteRegKey HKCR "Media Type\Extensions\.axv"
  DeleteRegKey HKCR "Media Type\Extensions\.flac"
  DeleteRegKey HKCR "Media Type\Extensions\.oga"
  DeleteRegKey HKCR "Media Type\Extensions\.ogg"
  DeleteRegKey HKCR "Media Type\Extensions\.ogv"
  DeleteRegKey HKCR "Media Type\Extensions\.spx"


  ; Extension to filter mapping for http - 7
  DeleteRegValue HKCR "http\Extensions" ".OGG"
  DeleteRegValue HKCR "http\Extensions" ".OGV"
  DeleteRegValue HKCR "http\Extensions" ".OGA"
  DeleteRegValue HKCR "http\Extensions" ".SPX"
  DeleteRegValue HKCR "http\Extensions" ".ANX"
  DeleteRegValue HKCR "http\Extensions" ".AXA"
  DeleteRegValue HKCR "http\Extensions" ".AXV"
  ; TODO::: FLAC
  

  ; MLS Perceived type - 6
  DeleteRegValue HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "ogv"
  DeleteRegValue HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "oga"
  DeleteRegValue HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "axa"
  DeleteRegValue HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "axv"
  DeleteRegValue HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "spx"
  DeleteRegValue HKLM "SOFTWARE\Microsoft\MediaPlayer\MLS\Extensions" "flac"  

  



  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP

  ; Delete utils - 4
  Delete "$INSTDIR\OOOggCommentDump.exe"
  Delete "$INSTDIR\OOOggValidate.exe"
  Delete "$INSTDIR\OOOggStat.exe"
  Delete "$INSTDIR\OOOggDump.exe"


  ; Delete libraries - 11
  Delete "$INSTDIR\libFLAC++.dll"
  Delete "$INSTDIR\libFLAC.dll"
  Delete "$INSTDIR\libfishsound.dll"
  Delete "$INSTDIR\libOOTheora.dll"
  Delete "$INSTDIR\vorbis.dll"

  Delete "$INSTDIR\libCMMLParse.dll"
  Delete "$INSTDIR\libCMMLTags.dll"
  Delete "$INSTDIR\libVorbisComment.dll"
  Delete "$INSTDIR\libOOOggSeek.dll"
  Delete "$INSTDIR\libOOOgg.dll"

  Delete "$INSTDIR\libTemporalURI.dll"


  ;Delete Filters - 16
  Delete "$INSTDIR\dsfVorbisEncoder.dll"
  Delete "$INSTDIR\dsfTheoraEncoder.dll"
  Delete "$INSTDIR\dsfSpeexEncoder.dll"
  Delete "$INSTDIR\dsfFLACEncoder.dll"

  Delete "$INSTDIR\dsfVorbisDecoder.dll"
  Delete "$INSTDIR\dsfFLACDecoder.dll"
  Delete "$INSTDIR\dsfTheoraDecoder.dll"
  Delete "$INSTDIR\dsfSpeexDecoder.dll"
  Delete "$INSTDIR\dsfOGMDecoder.dll"

  Delete "$INSTDIR\dsfNativeFLACSource.dll"

  Delete "$INSTDIR\dsfCMMLDecoder.dll"
  Delete "$INSTDIR\dsfCMMLRawSource.dll"

  Delete "$INSTDIR\dsfSubtitleVMR9.dll"
  
  Delete "$INSTDIR\dsfOggDemux2.dll"
  Delete "$INSTDIR\dsfOggMux.dll"

  ; Delete "$INSTDIR\dsfSeeking.dll"
  

  Delete "$INSTDIR\dsfAnxMux.dll"
  ; Delete "$INSTDIR\dsfAnxDemux.dll"



  ; Delete text files - 7
  Delete "$INSTDIR\ABOUT.txt"
  Delete "$INSTDIR\VERSIONS"
  Delete "$INSTDIR\README"
  Delete "$INSTDIR\COPYRIGHTS.rtf"
  Delete "$INSTDIR\COPYRIGHTS"

  Delete "$INSTDIR\AUTHORS"
  Delete "$INSTDIR\HISTORY"


  ; Delete runtimes - 3
  Delete "$INSTDIR\msvcr80.dll"
  Delete "$INSTDIR\msvcp80.dll"
  Delete "$INSTDIR\Microsoft.VC80.CRT.manifest"

  ; Delete unicows - 1
  Delete "$INSTDIR\unicows.dll"

  ;Delete accesory files, links etc.
  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  ; Remove the "illiminable" start menu group (but only if it's empty)
  RMDir "$SMPROGRAMS\$ICONS_GROUP\.."

  ; Need to change the working directory to something else (anything) besides
  ; the output directory, so we can rmdir it
  SetOutPath "$TEMP"
  RMDir "$INSTDIR"

  ; Remove the "illiminable" parent directory (but only if it's empty)
  RMDir "$INSTDIR\.."

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
;  SetAutoClose true
SectionEnd
