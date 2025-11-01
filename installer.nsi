; Copyright (c) 2025 INTcoin Core (Maddison Lane)
; Distributed under the MIT software license, see the accompanying
; file COPYING or http://www.opensource.org/licenses/mit-license.php.

; INTcoin NSIS Installer Script

!define PRODUCT_NAME "INTcoin"
!define PRODUCT_VERSION "0.1.0"
!define PRODUCT_PUBLISHER "INTcoin Core (Maddison Lane)"
!define PRODUCT_WEB_SITE "https://international-coin.org"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\intcoin-qt.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!include "MUI2.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "resources\icons\intcoin.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "COPYING"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\intcoin-qt.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "INTcoin-Setup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES64\INTcoin"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

RequestExecutionLevel admin

Section "INTcoin Core (required)" SEC01
  SectionIn RO

  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  ; Main executables
  File "build\Release\intcoind.exe"
  File "build\Release\intcoin-cli.exe"
  File "build\Release\intcoin-qt.exe"

  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\INTcoin"
  CreateShortCut "$SMPROGRAMS\INTcoin\INTcoin Wallet.lnk" "$INSTDIR\intcoin-qt.exe"
  CreateShortCut "$SMPROGRAMS\INTcoin\INTcoin Daemon.lnk" "$INSTDIR\intcoind.exe"
  CreateShortCut "$DESKTOP\INTcoin Wallet.lnk" "$INSTDIR\intcoin-qt.exe"
SectionEnd

Section "CPU Miner" SEC02
  SetOutPath "$INSTDIR"
  File "build\Release\intcoin-miner.exe"
  CreateShortCut "$SMPROGRAMS\INTcoin\INTcoin Miner.lnk" "$INSTDIR\intcoin-miner.exe"
SectionEnd

Section "Block Explorer" SEC03
  SetOutPath "$INSTDIR"
  File "build\Release\intcoin-explorer.exe"
  CreateShortCut "$SMPROGRAMS\INTcoin\Block Explorer.lnk" "$INSTDIR\intcoin-explorer.exe"
SectionEnd

Section "Configuration Files" SEC04
  SetOutPath "$INSTDIR\config"
  File /r "config\*.*"
SectionEnd

Section "Documentation" SEC05
  SetOutPath "$INSTDIR\docs"
  File "README.md"
  File "COPYING"
  File "ROADMAP.md"
  File "DESIGN-DECISIONS.md"
  File /nonfatal /r "docs\*.md"

  CreateShortCut "$SMPROGRAMS\INTcoin\Documentation.lnk" "$INSTDIR\docs"
SectionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\INTcoin\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\INTcoin\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\intcoin-qt.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\intcoin-qt.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Core INTcoin wallet and daemon (required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "CPU miner for mining INT coins"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "Block explorer for viewing blockchain data"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} "Sample configuration files for mainnet and testnet"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC05} "Documentation and guides"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\intcoin-qt.exe"
  Delete "$INSTDIR\intcoind.exe"
  Delete "$INSTDIR\intcoin-cli.exe"
  Delete "$INSTDIR\intcoin-miner.exe"
  Delete "$INSTDIR\intcoin-explorer.exe"

  Delete "$SMPROGRAMS\INTcoin\Uninstall.lnk"
  Delete "$SMPROGRAMS\INTcoin\Website.lnk"
  Delete "$SMPROGRAMS\INTcoin\INTcoin Wallet.lnk"
  Delete "$SMPROGRAMS\INTcoin\INTcoin Daemon.lnk"
  Delete "$SMPROGRAMS\INTcoin\INTcoin Miner.lnk"
  Delete "$SMPROGRAMS\INTcoin\Block Explorer.lnk"
  Delete "$SMPROGRAMS\INTcoin\Documentation.lnk"
  Delete "$DESKTOP\INTcoin Wallet.lnk"

  RMDir "$SMPROGRAMS\INTcoin"
  RMDir /r "$INSTDIR\docs"
  RMDir /r "$INSTDIR\config"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
