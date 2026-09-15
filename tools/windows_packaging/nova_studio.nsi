!include "MUI2.nsh"

!define PRODUCT_NAME "NOVA-STUDIO"
!define PRODUCT_PUBLISHER "NOVA-STUDIO Team"
!define PRODUCT_WEB_SITE "https://github.com/thelandy03-boop/NOVA-STUDIO"

SetCompressor /SOLID lzma

Name "${PRODUCT_NAME}"
OutFile "NOVA-STUDIO-Setup.exe"
InstallDir "$PROGRAMFILES64\NOVA-STUDIO"
ShowInstDetails show

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\NOVA-STUDIO.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Ejecutar NOVA-STUDIO ahora"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "Spanish"

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  ; Copiar carpeta stage completa
  File /r "C:\msys64\tmp\nova_stage\*.*"

  ; Crear accesos directos apuntando a $INSTDIR
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\NOVA-STUDIO"
  CreateShortCut "$SMPROGRAMS\NOVA-STUDIO\NOVA-STUDIO.lnk" "$INSTDIR\NOVA-STUDIO.exe" "" "$INSTDIR\NOVA-STUDIO.exe" 0
  CreateShortCut "$DESKTOP\NOVA-STUDIO.lnk" "$INSTDIR\NOVA-STUDIO.exe" "" "$INSTDIR\NOVA-STUDIO.exe" 0
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString" "$INSTDIR\uninst.exe"
SectionEnd

Section Uninstall
  Delete "$DESKTOP\NOVA-STUDIO.lnk"
  Delete "$SMPROGRAMS\NOVA-STUDIO\NOVA-STUDIO.lnk"
  RMDir "$SMPROGRAMS\NOVA-STUDIO"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
SectionEnd
