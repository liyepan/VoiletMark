; VoiletMark Windows 安装包（未签名）
; 按当前用户安装（不需要管理员权限），默认装到 %LOCALAPPDATA%\Programs\VoiletMark
Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"

!ifndef APP_VERSION
  !define APP_VERSION "0.0.0"
!endif
!ifndef DEPLOY_DIR
  !error "需要 /DDEPLOY_DIR=<windeployqt 输出目录>"
!endif
!ifndef ICON_FILE
  !define ICON_FILE "packaging\voiletmark.ico"
!endif
!ifndef OUTPUT_FILE
  !define OUTPUT_FILE "dist\VoiletMark-setup.exe"
!endif

!define APP_NAME "VoiletMark"
!define APP_EXE "VoiletMark.exe"
!define APP_PUBLISHER "VoiletMark"
!define APP_ID "VoiletMark"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_ID}"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "${OUTPUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\${APP_NAME}"
InstallDirRegKey HKCU "Software\${APP_ID}" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma
CRCCheck on
BrandingText "${APP_NAME} ${APP_VERSION}"

!define MUI_ICON "${ICON_FILE}"
!define MUI_UNICON "${ICON_FILE}"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "${APP_NAME} ${APP_VERSION}"
!define MUI_WELCOMEPAGE_TEXT "把 ${APP_NAME} 装到当前用户目录，不需要管理员权限。$\r$\n$\r$\n安装后会把 .md 文件关联到 ${APP_NAME}，随时可以在 Windows 设置里改回去。"
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "立即运行 ${APP_NAME}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "English"

Section "VoiletMark" SecMain
  SetShellVarContext current
  SetOutPath "$INSTDIR"
  File /r "${DEPLOY_DIR}\*.*"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; 开始菜单 + 桌面快捷方式
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\卸载 ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"

  ; .md 关联（只写 HKCU，不动系统级关联）
  WriteRegStr HKCU "Software\Classes\.md" "" "${APP_ID}.md"
  WriteRegStr HKCU "Software\Classes\.md" "Content Type" "text/markdown"
  WriteRegStr HKCU "Software\Classes\${APP_ID}.md" "" "Markdown 文档"
  WriteRegStr HKCU "Software\Classes\${APP_ID}.md" "FriendlyTypeName" "Markdown 文档"
  WriteRegStr HKCU "Software\Classes\${APP_ID}.md\DefaultIcon" "" "$INSTDIR\${APP_EXE},0"
  WriteRegStr HKCU "Software\Classes\${APP_ID}.md\shell\open\command" "" '"$INSTDIR\${APP_EXE}" "%1"'

  ; 「打开方式」里也出现一份
  WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}" "FriendlyAppName" "${APP_NAME}"
  WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}\shell\open\command" "" '"$INSTDIR\${APP_EXE}" "%1"'
  WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}\SupportedTypes" ".md" ""

  ; 让资源管理器立刻刷新图标 / 关联
  System::Call 'shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'

  ; 控制面板「卸载程序」
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "${UNINST_KEY}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\${APP_EXE}"
  WriteRegStr HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1
  WriteRegStr HKCU "Software\${APP_ID}" "InstallDir" "$INSTDIR"
SectionEnd

Section "Uninstall"
  SetShellVarContext current

  Delete "$DESKTOP\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\卸载 ${APP_NAME}.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"

  ; 只在 .md 仍指向我们时才清掉默认值，别破坏用户后来改的关联
  ReadRegStr $0 HKCU "Software\Classes\.md" ""
  ${If} $0 == "${APP_ID}.md"
    DeleteRegValue HKCU "Software\Classes\.md" ""
  ${EndIf}
  DeleteRegKey HKCU "Software\Classes\${APP_ID}.md"
  DeleteRegKey HKCU "Software\Classes\Applications\${APP_EXE}"

  DeleteRegKey HKCU "${UNINST_KEY}"
  DeleteRegKey HKCU "Software\${APP_ID}"

  RMDir /r "$INSTDIR"

  System::Call 'shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
SectionEnd
