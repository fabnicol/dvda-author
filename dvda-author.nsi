;___________________________________________________________________________________________

;NSIS installer script for dvda-author Copyright Fabrice Nicol 2008 <fabnicol@users.sourceforge.net>
;see COPYING for lincense. This script is part of the dvda-author package

;_____________________________________________________________________________________________


; Compile from without this directory
; Modern interface settings

!include "MUI2.nsh"
!include "x64.nsh"

!define version  ".win32"
!define prodname "dvda-author"
!define setup    "${prodname}-${version}.installer.exe"
!define srcdir   "${prodname}-${version}"
!define website  "http://dvd-audio.sourceforge.net"
!define project  "${srcdir}\CB_project"
!define utils "${srcdir}\libutils"
!define source   "${srcdir}\src"
!define images   "${srcdir}\images"
!define libats2wav   "${srcdir}\libats2wav"
!define libfixwav   "${srcdir}\libfixwav"
!define m4       "${srcdir}\m4"
!define m4extra  "${srcdir}\m4.extra"
!define m4extradvdauthor  "${srcdir}\m4.extra.dvdauthor"
!define libiberty "${srcdir}\libiberty"
!define libs "${srcdir}\libs"
!define bin "${srcdir}\bin"
!define menu "${srcdir}\menu"
!define exec     "${prodname}-launch.bat"
!define binary   "${prodname}.exe"
!define icon     "${prodname}.ico"
!define regkey   "Software\${prodname}-${version}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}-${version}"
!define startmenu   "$SMPROGRAMS\${prodname}-${version}"
!define uninstaller "uninstall.exe"
!define notefile    "${srcdir}\README"

; put MUI_ICON before all pages

!define MUI_ICON "${srcdir}\${prodname}.ico"

; welcome pages here for some odd buggy reason (version 2.39)

!define MUI_WELCOMEFINISHPAGE
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH
!define MUI_WELCOMEFINISHPAGE_BITMAP  "${srcdir}\images\welcome.bmp"
!define MUI_WELCOMEPAGE_TEXT  $(wizard1)
!define MUI_WELCOMEPAGE_TITLE $(wizard2)

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${srcdir}\images\headerLeft.bmp" ; optional
!define MUI_ABORTWARNING


!insertmacro MUI_PAGE_WELCOME  ; must appear BEFORE MUI_LANGUAGE for some odd reason.
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French" ; must appear BEFORE language strings


; Language strings

 LangString  wizard1 ${LANG_ENGLISH} "This wizard will guide you through the installation of ${prodname} version ${version}. Click next to continue."
 LangString  wizard1 ${LANG_FRENCH}  "Installation de ${prodname} version ${version}. Appuyer sur suivant pour continuer."
 LangString  wizard2 ${LANG_ENGLISH} "${prodname} installation"
 LangString  wizard2 ${LANG_FRENCH}  "Installation de ${prodname}"
 LangString title1 ${LANG_ENGLISH}  "Installation completed"
 LangString title1 ${LANG_FRENCH}   "Installation termin�e"
 LangString title1 ${LANG_ENGLISH} "Installation completed"
 LangString title1 ${LANG_FRENCH}  "Installation termin�e"
 LangString text1 ${LANG_ENGLISH}  "${prodname} ${version} sucessfully installed to $INSTDIR"
 LangString text1 ${LANG_FRENCH}  "${prodname} ${version} a �t� install� dans $INSTDIR"
 LangString text2 ${LANG_ENGLISH} "Show README file"
 LangString text2 ${LANG_FRENCH}  "Afficher le fichier README"
 LangString link1 ${LANG_ENGLISH} "Browse DVD audio tools webpage"
 LangString link1 ${LANG_FRENCH} "Aller au site internet de DVD audio Tools"
 LangString uninstall ${LANG_ENGLISH}    "${prodname} uninstall"
 LangString uninstall ${LANG_FRENCH}     "d�sinstallation de ${prodname} "
 LangString completed ${LANG_ENGLISH}    "Installation completed"
 LangString completed ${LANG_FRENCH}    "Installation termin�e"

 LangString Sec1Name ${LANG_ENGLISH} "${prodname}"
 LangString Sec2Name ${LANG_ENGLISH} "Code::Blocks project"
 LangString Sec3Name ${LANG_ENGLISH} "source code"
 LangString Sec4Name ${LANG_ENGLISH} "GNU build system"
 LangString Message  ${LANG_ENGLISH} "Click on Yes to install ${prodname}"

 LangString Sec1Name ${LANG_FRENCH} "${prodname}"
 LangString Sec2Name ${LANG_FRENCH} "projet Code::Blocks"
 LangString Sec3Name ${LANG_FRENCH} "code source"
 LangString Sec4Name ${LANG_FRENCH} "syst�me de compilation GNU"
 LangString Message  ${LANG_FRENCH} "Appuyer sur Oui pour installer ${prodname}"

 LangString DESC_sec1 ${LANG_ENGLISH} "Install binary"
 LangString DESC_sec2 ${LANG_ENGLISH} "Install Code::Blocks project"
 LangString DESC_sec3 ${LANG_ENGLISH} "Install source code"
 LangString DESC_sec4 ${LANG_ENGLISH} "Install GNU build system"

 LangString DESC_sec1 ${LANG_FRENCH} "Installer l'ex�cutable"
 LangString DESC_sec2 ${LANG_FRENCH} "Installer le projet Code::Blocks"
 LangString DESC_sec3 ${LANG_FRENCH} "Installer le code source"
 LangString DESC_sec4 ${LANG_FRENCH} "Installer le syst�me de compilation GNU"


 LicenseLangString myLicenseData ${LANG_ENGLISH} "${srcdir}\COPYING"
 LicenseLangString myLicenseData ${LANG_FRENCH} "${srcdir}\COPYING"
 LicenseData $(myLicenseData)
 LangString Name ${LANG_ENGLISH} "${prodname} English version"
 LangString Name ${LANG_FRENCH}  "${prodname} version fran�aise"
 Name $(Name)

; MUI macros


!define MUI_FINISHPAGE_TITLE $(title1)
!define MUI_FINISHPAGE_TEXT  $(text1)
!define MUI_FINISHPAGE_BUTTON  "OK"
!define MUI_FINISHPAGE_CANCEL_ENABLED
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT $(text2)
!define MUI_FINISHPAGE_RUN_FUNCTION "Launch_${prodname}"
!define MUI_FINISHPAGE_LINK $(link1)
!define MUI_FINISHPAGE_LINK_LOCATION "http://dvd-audio.sourceforge.net"



!insertmacro MUI_PAGE_LICENSE "${srcdir}\COPYING"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
UninstallText  $(uninstall)
CompletedText  $(completed)

;XPStyle on
ShowInstDetails show
ShowUninstDetails show
RequestExecutionLevel user

Caption "${prodname}"

OutFile "${setup}"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal

InstallDir "$PROGRAMFILES\${prodname}-${version}"
InstallDirRegKey HKLM "${regkey}" ""


AutoCloseWindow false
ShowInstDetails show


;--------------------------------
; Functions and subs
;--------------------------------


Function Launch_${prodname}
  Exec '"notepad" "$INSTDIR\README" '
FunctionEnd

Function .onInit
     !define MUI_LANGDLL_WINDOWTITLE "Installation language/Langue d'installation"
     !define MUI_LANGDLL_INFO "Select language/S�lectionner la langue"
     !define MUI_LANGDLL_ALWAYSSHOW

  SetOutPath $TEMP
  File /oname=spltmp.bmp "${images}\spltmp.bmp"

  advsplash::show 1300 600 400 -1 $TEMP\spltmp

  Pop $0 ; $0 has '1' if the user closed the splash screen early,
         ; '0' if everything closed normally, and '-1' if some error occurred.

  Delete $TEMP\spltmp.bmp

;Calling format
;       advsplash::show Delay FadeIn FadeOut KeyColor FileName

  !insertmacro MUI_LANGDLL_DISPLAY


;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "HKCU"
  !define MUI_LANGDLL_REGISTRY_KEY "Software\Modern UI Test"
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

FunctionEnd




Section
MessageBox MB_YESNO|MB_ICONINFORMATION $(Message)  IDNO Fin IDYES End
Fin: Abort
End:

CreateDirectory "${startmenu}"

SectionEnd




Section  ; allways done
  !ifdef icon
    CreateShortCut "${startmenu}\${prodname}.lnk" "$INSTDIR\${exec}" "" "$INSTDIR\${icon}"
  !else
    CreateShortCut "${startmenu}\${prodname}.lnk" "$INSTDIR\${exec}"
  !endif

  WriteRegStr HKCR "${prodname}\Shell\open\command\" "" '"$INSTDIR\${exec} "%1"'

  !ifdef icon
    WriteRegStr HKCR "${prodname}\DefaultIcon" "" "$INSTDIR\${icon}"
  !endif

  CreateShortCut "${startmenu}\${uninstaller}.lnk" "$INSTDIR\${uninstaller}"

  !ifdef website
  WriteINIStr "${startmenu}\${prodname} website.url" "InternetShortcut" "URL" ${website}
  !endif
  WriteRegStr HKLM "${regkey}" "Install_Dir" "$INSTDIR"
  ; write uninstall strings
  WriteRegStr HKLM "${uninstkey}" "DisplayName" "${prodname} (uninstall only)"
  WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'
  SetOutPath $INSTDIR

  WriteUninstaller "${uninstaller}"

  SetOutPath $INSTDIR ; for working directory
  File "${srcdir}\README" "${srcdir}\INSTALL" "${srcdir}\AUTHORS" "${srcdir}\NEWS" "${srcdir}\ChangeLog"
  File "${srcdir}\${srcdir}.html"

SectionEnd


Section  $(Sec1Name) sec1 ; binary

  SetOutPath $INSTDIR ; for working directory
  File  "${project}\${prodname}\bin\Release\${binary}"   "${srcdir}\${prodname}-launch.bat"

SectionEnd

Section $(Sec2Name) sec2 ;Code::Blocks project
  SetOutPath $INSTDIR ; for working directory
  File  /r "${project}"
SectionEnd




Section $(Sec3Name) sec3 ; source code

  SetOutPath $INSTDIR ; for working directory
  File  /r "${source}"
  File  /r "${utils}"
  File  /r "${images}"
  File  /r "${libfixwav}"
  File  /r "${libats2wav}"
  File  /r "${m4}"
  File  /r "${m4extra}"
  File  /r "${m4extradvdauthor}"
  File  /r "${libiberty}"
  File  /r "${libs}"
  File  /r "${menu}"
  File  /r "${bin}"


SectionEnd


Section $(Sec4Name) sec4  ; GNU build system

 SetOutPath $INSTDIR ; for working directory
 ; one can only exclude by name (limitation reported in bugs site)
 File  /x "${project}\${prodname}\bin\Release\${binary}" /x "${srcdir}\${prodname}-launch.bat"  "${srcdir}\*.*"

SectionEnd

; to be placed after definition of sec1, ..., sec4 hence here.
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${sec1} $(DESC_sec1)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec2} $(DESC_sec2)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec3} $(DESC_sec3)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec4} $(DESC_sec4)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"

  DeleteRegKey HKLM "${uninstkey}"
  DeleteRegKey HKLM "${regkey}"

  Delete "${startmenu}\*.*"
  Delete "${startmenu}"

  Delete "$INSTDIR\*.*"
  RMDir /r "$INSTDIR"


SectionEnd

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE
FunctionEnd

BrandingText "${prodname}.${version}"

; eof
