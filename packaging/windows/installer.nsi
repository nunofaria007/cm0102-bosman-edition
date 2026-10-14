; The Windows installer of CM 01/02: Re-Signed Edition (NSIS 3, made on the Mac with makensis).
; packaging/windows/windows.cmake runs it with these defines:
;   VERSION   the project version, for example 0.1.0
;   EXE       the 64-bit cm0102_recomp.exe
;   GAME_DIR  the folder of the clean files (cm0102.exe, Data, Pictures and the rest)
;   OUT_FILE  the setup exe to write
;
; Layout: $INSTDIR holds the program and the uninstaller, $INSTDIR\game the clean files. The program
; finds the game folder beside itself. Saves go to %APPDATA%\cm0102, which an update keeps. The
; uninstaller asks before it deletes the saves.

!ifndef VERSION
  !error "Give VERSION with -DVERSION=..."
!endif
!ifndef EXE
  !error "Give EXE with -DEXE=..."
!endif
!ifndef GAME_DIR
  !error "Give GAME_DIR with -DGAME_DIR=..."
!endif
!ifndef OUT_FILE
  !error "Give OUT_FILE with -DOUT_FILE=..."
!endif
!if "${GAME_DIR}" == ""
  !error "Set CM0102_GAME_DIR to the folder of the clean files"
!endif

Unicode true
SetCompressor /SOLID lzma
ManifestDPIAware true

!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"

!define PRODUCT "CM 01/02: Re-Signed Edition"
!define FILE_NAME "CM 01-02 Re-Signed Edition"
!define PROGRAM "${FILE_NAME}.exe"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\CM0102ReSignedEdition"
!define APP_KEY "Software\CM0102ReSignedEdition"

Name "${PRODUCT}"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES64\${FILE_NAME}"
InstallDirRegKey HKLM "${APP_KEY}" "InstallDir"
RequestExecutionLevel admin
BrandingText "${PRODUCT} ${VERSION}"

VIProductVersion "${VERSION}.0"
VIFileVersion "${VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "FileDescription" "${PRODUCT} ${VERSION} Setup"
VIAddVersionKey /LANG=1033 "Comments" "An unofficial port of Championship Manager 01/02"
VIAddVersionKey /LANG=1033 "LegalCopyright" "An unofficial fan project"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PROGRAM}"
!define MUI_FINISHPAGE_RUN_TEXT "Start ${PRODUCT}"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "${PRODUCT} needs 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
FunctionEnd

Function un.onInit
  SetRegView 64
FunctionEnd

Section "Install"
  SetOutPath "$INSTDIR"
  File "/oname=${PROGRAM}" "${EXE}"

  ; An update replaces the clean files as a whole.
  RMDir /r "$INSTDIR\game"
  SetOutPath "$INSTDIR\game"
  File /r "${GAME_DIR}/*"
  SetOutPath "$INSTDIR"

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  SetShellVarContext all
  CreateShortCut "$SMPROGRAMS\${FILE_NAME}.lnk" "$INSTDIR\${PROGRAM}" "" "$INSTDIR\${PROGRAM}" 0
  CreateShortCut "$DESKTOP\${FILE_NAME}.lnk" "$INSTDIR\${PROGRAM}" "" "$INSTDIR\${PROGRAM}" 0

  WriteRegStr HKLM "${APP_KEY}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayName" "${PRODUCT}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "Publisher" "An unofficial fan project"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\${PROGRAM}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "${UNINSTALL_KEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "$SMPROGRAMS\${FILE_NAME}.lnk"
  Delete "$DESKTOP\${FILE_NAME}.lnk"

  Delete "$INSTDIR\${PROGRAM}"
  RMDir /r "$INSTDIR\game"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

  DeleteRegKey HKLM "${UNINSTALL_KEY}"
  DeleteRegKey HKLM "${APP_KEY}"

  ; The saves stay unless the player says so. A silent uninstall keeps them.
  SetShellVarContext current
  ${IfNot} ${Silent}
  ${AndIf} ${FileExists} "$APPDATA\cm0102\*"
    MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION \
      "Also delete your saved games and settings in $APPDATA\cm0102?" IDNO keep_saves
    RMDir /r "$APPDATA\cm0102"
    keep_saves:
  ${EndIf}
SectionEnd
