; License: BSD Zero Clause License file
; Copyright:
;   (C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>

; NSIS2 Script
; Compatible with NSIS Unicode 2.45.

!define PRODUCT_VERSION "@CMAKE_PROJECT_VERSION@"
!define PRODUCT_NAME "GSmartControl"
!define PRODUCT_NAME_SMALL "gsmartcontrol"
!define PRODUCT_PUBLISHER "Alexander Shaduri"
!define PRODUCT_WEB_SITE "https://gsmartcontrol.shaduri.dev"

;!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\AppMainExe.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"


!include "FileFunc.nsh"  ; GetOptions



; --------------- General Settings


; This is needed for proper start menu item manipulation (for all users) in vista
RequestExecutionLevel admin

; This compressor gives us the best results
SetCompressor /SOLID lzma

; Do a CRC check before installing
CRCCheck On

; This is used in titles
Name "${PRODUCT_NAME}"  ;  ${PRODUCT_VERSION}

; Output File Name
OutFile "${PRODUCT_NAME_SMALL}-${PRODUCT_VERSION}-@WINDOWS_SUFFIX@.exe"


; The Default Installation Directory:
; Try to install to the same directory as runtime.
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
; If already installed, try here
InstallDirRegKey HKLM "SOFTWARE\${PRODUCT_NAME}" "InstallationDirectory"


ShowInstDetails show
ShowUnInstDetails show





; --------------------- MUI INTERFACE

; MUI 2.0 compatible install
!include "MUI2.nsh"
!include "InstallOptions.nsh"  ; section description macros

; Backgound Colors. uncomment to enable fullscreen.
; BGGradient 0000FF 000000 FFFFFF

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "nsi_install.ico"
!define MUI_UNICON "nsi_uninstall.ico"


; Things that need to be extracted on first (keep these lines before any File command!).
; Only useful for BZIP2 compression.
;!insertmacro MUI_RESERVEFILE_LANGDLL
;!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS


; Language Selection Dialog Settings
;!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
;!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
;!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

!define LICENSE_FILE "doc\distribution.txt"

; Pages to show during installation
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${LICENSE_FILE}"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

;!define MUI_FINISHPAGE_RUN "$INSTDIR\gsmartcontrol.exe"
;!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\Example.file"
;!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!insertmacro MUI_PAGE_FINISH



; Uninstaller page
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES



; Language files
!insertmacro MUI_LANGUAGE "English"


; --------------- END MUI

LangString TEXT_IO_TITLE ${LANG_ENGLISH} "GSmartControl"


var install_option_removeold  ; uninstall the old version first (if present): yes (default), no.



; ----------------- INSTALLATION TYPES

; InstType "Recommended"
; InstType "Full"
; InstType "Minimal"




Section "!GSmartControl" SecMain
SectionIn 1 RO
	SetShellVarContext all  ; use all user variables as opposed to current user
	SetOutPath "$INSTDIR"

	SetOverwrite Off ; don't overwrite the config file
	; File "gsmartcontrol2.conf"

	SetOverwrite On
	File *.exe
	File *.dll
	; File gsmartcontrol.exe.manifest
	File gsmartcontrol.ico
	File icon_*.png
	File drivedb.h

	; Include the "doc" directory completely.
	File /r doc

	; Include the "ui" directory completely.
	File /r ui

	; GTK and stuff
	File /r etc
	File /r share

	; Add Shortcuts (this inherits the exe's run permissions)
	CreateShortCut "$SMPROGRAMS\GSmartControl.lnk" "$INSTDIR\gsmartcontrol.exe" "" \
		"$INSTDIR\gsmartcontrol.ico" "" SW_SHOWNORMAL "" "GSmartControl - Hard disk drive and SSD health inspection tool"

SectionEnd



; Executed on installation start
Function .onInit
	SetShellVarContext all  ; use all user variables as opposed to current user

	${GetOptions} "$CMDLINE" "/removeold=" $install_option_removeold

	Call PreventMultipleInstances

	Call DetectPrevInstallation
FunctionEnd



; ------------------ POST INSTALL


Section -post
	SetShellVarContext all  ; use all user variables as opposed to current user

	WriteRegStr HKLM "SOFTWARE\${PRODUCT_NAME}" "InstallationDirectory" "$INSTDIR"
	WriteRegStr HKLM "SOFTWARE\${PRODUCT_NAME}" "Vendor" "${PRODUCT_PUBLISHER}"

	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\gsmartcontrol_uninst.exe"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\gsmartcontrol.ico"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1

	; We don't need this, MUI takes care for us
	; WriteRegStr HKCU "Software\${PRODUCT_NAME}" "Installer Language" $sUAGE

	; write out uninstaller
	WriteUninstaller "$INSTDIR\gsmartcontrol_uninst.exe"

	; uninstall shortcut
	;CreateDirectory "$SMPROGRAMS\GSmartControl"
	;CreateShortCut "$SMPROGRAMS\GSmartControl\Uninstall GSmartControl.lnk" "$INSTDIR\gsmartcontrol_uninst.exe" "" ""

SectionEnd ; post





; ---------------- UNINSTALL



Function un.onUninstSuccess
	HideWindow
	MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer." /SD IDOK
FunctionEnd




Section Uninstall
	SetShellVarContext all  ; use all user variables as opposed to current user
	SetAutoClose false

	; add delete commands to delete whatever files/registry keys/etc you installed here.
	Delete "$INSTDIR\gsmartcontrol_uninst.exe"

	DeleteRegKey HKLM "SOFTWARE\${PRODUCT_NAME}"
	DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"

	Delete "$INSTDIR\*.exe"
	Delete "$INSTDIR\*.dll"
	; Delete "$INSTDIR\gsmartcontrol.exe.manifest"
	Delete "$INSTDIR\gsmartcontrol.ico"
	Delete "$INSTDIR\icon_*.png"

	; update-smart-drivedb may leave these
	Delete "$INSTDIR\drivedb.h"
	Delete "$INSTDIR\drivedb.h.*"

	RMDir /r "$INSTDIR\doc"

	RMDir /r "$INSTDIR\ui"

	; GTK and stuff
	RMDir /r "$INSTDIR\etc"
	RMDir /r "$INSTDIR\share"

	; clean up generated stuff
	Delete "$INSTDIR\*stdout.txt"
	Delete "$INSTDIR\*stderr.txt"

	RMDir "$INSTDIR"  ; only if empty

	Delete "$SMPROGRAMS\GSmartControl.lnk"
	;Delete "$SMPROGRAMS\GSmartControl\Uninstall GSmartControl.lnk"

SectionEnd ; end of uninstall section



; --------------- Helpers

; Detect previous installation
Function DetectPrevInstallation
	; if /removeold=no option is given, don't check anything.
	StrCmp $install_option_removeold "no" old_detect_done

	SetShellVarContext all  ; use all user variables as opposed to current user
	push $R0

	; detect previous installation
	ReadRegStr $R0 HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
	StrCmp $R0 "" old_detect_done

	MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
		"${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
		previous version or `Cancel` to continue anyway." \
		/SD IDOK IDOK old_uninst
		; Abort
		goto old_detect_done

	; Run the old uninstaller
	old_uninst:
		ClearErrors
		IfSilent old_silent_uninst old_nosilent_uninst

		old_nosilent_uninst:
			ExecWait '$R0'
			goto old_uninst_continue

		old_silent_uninst:
			ExecWait '$R0 /S _?=$INSTDIR'

		old_uninst_continue:

		IfErrors old_no_remove_uninstaller

		; You can either use Delete /REBOOTOK in the uninstaller or add some code
		; here to remove to remove the uninstaller. Use a registry key to check
		; whether the user has chosen to uninstall. If you are using an uninstaller
		; components page, make sure all sections are uninstalled.
		old_no_remove_uninstaller:

	old_detect_done: ; old installation not found, all ok

	pop $R0
FunctionEnd



; Prevent running multiple instances of the installer
Function PreventMultipleInstances
	Push $R0
	System::Call 'kernel32::CreateMutexA(i 0, i 0, t ${PRODUCT_NAME}) ?e'
	Pop $R0
	StrCmp $R0 0 +3
		MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running." /SD IDOK
		Abort
	Pop $R0
FunctionEnd



; eof
