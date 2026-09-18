!include "MUI2.nsh"

!define PRODUCT_NAME "NOVA-STUDIO"
!define PRODUCT_VERSION "9.8.34"
!define PRODUCT_PUBLISHER "NOVA-STUDIO Team"
!define PRODUCT_WEB_SITE "https://github.com/thelandy03-boop/NOVA-STUDIO"

; Compresor solido rapido
SetCompressor /SOLID zlib

Name "${PRODUCT_NAME} v${PRODUCT_VERSION}"
OutFile "NOVA-STUDIO-Setup-v${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES64\NOVA-STUDIO"
RequestExecutionLevel admin
ShowInstDetails show

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\NOVA-STUDIO.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Ejecutar NOVA-STUDIO ahora"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Spanish"

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Copiar carpeta stage completa optimizada
  File /r "C:\msys64\tmp\nova_stage\*.*"

  ; Crear accesos directos
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\NOVA-STUDIO"
  CreateShortCut "$SMPROGRAMS\NOVA-STUDIO\NOVA-STUDIO.lnk" "$INSTDIR\NOVA-STUDIO.exe" "" "$INSTDIR\NOVA-STUDIO.exe" 0
  CreateShortCut "$DESKTOP\NOVA-STUDIO.lnk" "$INSTDIR\NOVA-STUDIO.exe" "" "$INSTDIR\NOVA-STUDIO.exe" 0
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName" "${PRODUCT_NAME} v${PRODUCT_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString" "$INSTDIR\uninst.exe"
SectionEnd

Section Uninstall
  Delete "$DESKTOP\NOVA-STUDIO.lnk"
  Delete "$SMPROGRAMS\NOVA-STUDIO\NOVA-STUDIO.lnk"
  RMDir "$SMPROGRAMS\NOVA-STUDIO"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
SectionEnd