;======================================================================================================
;
; Password Safe Installation Script
;
; Copyright 2004, David Lacy Kusters (dkusters@yahoo.com)
; Copyright 2005-2007 Rony Shapiro <ronys@users.sourceforge.net>
; 2009 extended by Karel Van der Gucht for multiple language use
; This script may be redistributed and/or modified under the Artistic
; License 2.0 terms as available at 
; http://www.opensource.org/licenses/artistic-license-2.0.php
;
; This script is distributed AS IS.  All warranties are explicitly
; disclaimed, including, but not limited to, the implied warranties of
; MERCHANTABILITY and FITNESS FOR A PARTICULAR PURPOSE.
; 
; SYNOPSIS
;
; This script will create a self-extracting installer for the Password
; safe program.  It is intended to be used with the Nullsoft 
; Installation System, available at http://nsis.sf.net.   After use, 
; pwsafe-X.XX.exe will be placed in the current directory (where X.XX
; is the version number of Password Safe).  The generated file can be 
; placed on a publicly available location.
;
; DESCRIPTION
;
; This script will create a self-extracting installer for the Password
; Safe program.  Password Safe was designed to be self contained.
; In general, use of this installer is not mandatory. pwsafe.exe, the
; executable for Password Safe, can be placed in any location and run
; without any registration of DLLs, creation of directories, or entry
; of registry values.  When Password Safe intializes, any necessary
; setup will be performed by pwsafe.exe.  So, what is the purpose of
; this installer?
;
; This installer puts a familiar face on the installation process.
; Most Windows users are used to running a program to install 
; software, not copying a file or unzipping an archive.  Also, this
; installer performs some minor tasks that are common to many 
; Windows installers:
; 
; 1. The installer will allow the user to place icons on the desktop
;    or in the Start Menu, for easy access.  
;
; 2. The installer places four registry values in 
;    HKCU\Software\Password Safe\Password Safe.  These registry
;    values are for the use of the installer itself.  Password Safe
;    does not rely on these registry values.  If the installer is not
;    used, these registry values need not be created.
;
; 3. The installer will create an uninstaller and place an entry to
;    uninstall Password Safe in the Add or Remove Programs Wizard.
;
; As of PasswordSafe 3.05, this script allows users to choose
; between a "Regular" installation and a "Green" one, the difference
; being that the latter does not write app-specific data to the registry
; This is useful for installing to disk-on-key, and where company policy
; and/or user permissions disallow writing to the registry. Also, Green
; installation doesn't create an Uninstall.exe or entry in Add/Remove
; Software in the control panel - to unistall, just delete the install
; directory...
;
; USE
;
; To use this script, the following requirements must be satisfied:
;
; 1. Install NSIS (available at http://nsis.sf.net).  At least version
;    2.0 should be used.  This script is compatible with version 2.0
;    of NSIS.
;
; 2. Make sure that makensis.exe is on your path.  This is only to make
;    easier step 3 of the creation process detailed below.  This script
;    does not recursively call makensis.exe, so this step is merely for
;    convenience sake.
;
; After the above requirements are fulfilled, the following steps 
; should be followed each time you want to create a release:
;
; 1. Compile Password Safe in release mode.  The script relies on 
;    pwsafe.exe existing in the Release subdirectory.
;
; 2. Compile the help files for Password Safe.  The script relies on 
;    pwsafe.chm existing in the help/default subdirectory.
;
; 3. At the command line (or in a build script such as the .dsp file,
;    makefile, or other scripted build process), execute the following:
;
;        makensis.exe /DVERSION=X.XX pwsafe.nsi
;
;    where X.XX is the version number of the current build of Password
;    Safe.
;
; The output from the above process should be pwsafe-X.XX.exe.  This is
; the installer.  It can be placed, by itself, on a publicly available
; location.
; 
; the script is setup for several languages, and ready for others.
; Just remove the comments ";-L-" where appropriate.
; Additional languages can be easily added, the following "pieces"
; have to be provided:
; - a .\I18N\pwsafe_xx.lng file with the installation texts in the language
; - several MUI_LANGUAGE
; - several "File" for the language specific DLL
; - "Delete ...DLL" for each language (at install time)
; - 'Delete "$INSTDIR\pwsafeXX.dll"'  for each language (at uninstall time)
; - 'CreateShortCut "Password Safe Help XX.lnk" for each language (at install time)
; - "Push" in the "Language selection dialog"

;-----------------------------------------
; Set verbosity appropriate for a Makefile
!verbose 2

;--------------------------------
; Include Modern UI
  !include "MUI2.nsh"
  !include "InstallOptions.nsh"
 
;--------------------------------
; process detection support
; (requires nsProcess plugin, from
;  http://nsis.sourceforge.net/NsProcess_plugin)
!include "nsProcess.nsh"

;--------------------------------
; Version Info
;
; Hopefully, this file will be compiled via the following command line
; command:
;
; makensis.exe /DVERSION=X.XX pwsafe.nsi
;
; where X.XX is the version number of Password Safe.

  !ifndef VERSION
    !error "VERSION undefined. Usage: makensis.exe /DVERSION=X.XX pwsafe.nsi"
  !endif

;--------------------------------
;Variables

  Var INSTALL_TYPE
  Var HOST_OS
  Var PROG_LANGUAGE
  
  ;Request application privileges for Windows Vista, Windows 7
  RequestExecutionLevel admin

;--------------------------------
; Pages

  !insertmacro MUI_PAGE_LICENSE "..\LICENSE" ;$(myLicenseData)
  ; ask about installation type, "green" or "regular"
  Page custom GreenOrRegular
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; General

  ; Name and file
  Name "Password Safe ${VERSION}"
  BrandingText "PasswordSafe ${VERSION} Installer"

  OutFile "pwsafe-${VERSION}.exe"

  ; Default installation folder
  InstallDir "$PROGRAMFILES\Password Safe"
  
  ; Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Password Safe\Password Safe" "installdir"

;--------------------------------
; Languages
; To enable a language, remove the ";" in front, to disable: put a ";" in front

!define LANGUAGE_GERMAN
!define LANGUAGE_CHINESE
!define LANGUAGE_SPANISH
!define LANGUAGE_SWEDISH
;!define LANGUAGE_DUTCH
!define LANGUAGE_FRENCH
!define LANGUAGE_RUSSIAN
!define LANGUAGE_POLISH
;!define LANGUAGE_ITALIAN
!define LANGUAGE_DANISH

  !insertmacro MUI_LANGUAGE "English"
!ifdef LANGUAGE_GERMAN
  !insertmacro MUI_LANGUAGE "German"
  !include ".\I18N\pwsafe_de.lng"
!endif
!ifdef LANGUAGE_CHINESE
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !include ".\I18N\pwsafe_zh.lng"
!endif
!ifdef LANGUAGE_SPANISH
  !insertmacro MUI_LANGUAGE "Spanish"
  !include ".\I18N\pwsafe_es.lng"
!endif
!ifdef LANGUAGE_SWEDISH
  !insertmacro MUI_LANGUAGE "Swedish"
  !include ".\I18N\pwsafe_sv.lng"
!endif
!ifdef LANGUAGE_DUTCH
  !insertmacro MUI_LANGUAGE "Dutch"
  !include ".\I18N\pwsafe_nl.lng"
!endif
!ifdef LANGUAGE_FRENCH
  !insertmacro MUI_LANGUAGE "French"
  !include ".\I18N\pwsafe_fr.lng"
!endif
!ifdef LANGUAGE_RUSSIAN
  !insertmacro MUI_LANGUAGE "Russian"
  !include ".\I18N\pwsafe_ru.lng"
!endif
!ifdef LANGUAGE_POLISH
  !insertmacro MUI_LANGUAGE "Polish"
  !include ".\I18N\pwsafe_pl.lng"
!endif
!ifdef LANGUAGE_ITALIAN
  !insertmacro MUI_LANGUAGE "Italian"
  !include ".\I18N\pwsafe_it.lng"
!endif
!ifdef LANGUAGE_DANISH
  !insertmacro MUI_LANGUAGE "Danish"
  !include ".\I18N\pwsafe_dk.lng"
!endif

; English texts here
; Note that if we add a string, it needs to be added in all the
; .\I18N\pwsafe_xx.lng files

;Reserve Files
;LangString RESERVE_TITLE ${LANG_ENGLISH} "Choose Installation Type"
;LangString RESERVE_FIELD1 ${LANG_ENGLISH} "Regular (uses Registry, suitable for home or single user PC)"
;LangString RESERVE_FIELD2 ${LANG_ENGLISH} "Green (for Disk-on-Key; does not use host Registry)"

; The program itself
LangString PROGRAM_FILES ${LANG_ENGLISH} "Program Files"

; Start with Windows
LangString START_AUTO ${LANG_ENGLISH} "Start automatically"

; Start menu
LangString START_SHOW ${LANG_ENGLISH} "Show in start menu"

; Desktop shortcut
LangString START_SHORTCUT ${LANG_ENGLISH} "Install desktop shortcut"

; Uninstall shortcut
LangString UNINSTALL_SHORTCUT ${LANG_ENGLISH} "Uninstall shortcut"

; Descriptions
LangString DESC_ProgramFiles ${LANG_ENGLISH} "Installs the basic files necessary to run Password Safe."
LangString DESC_StartUp ${LANG_ENGLISH} "Starts Password Safe as part of Windows boot/login."
LangString DESC_StartMenu ${LANG_ENGLISH} "Creates an entry in the start menu for Password Safe."
LangString DESC_DesktopShortcut ${LANG_ENGLISH} "Places a shortcut to Password Safe on your desktop."
LangString DESC_UninstallMenu ${LANG_ENGLISH} "Places a shortcut in the start menu to Uninstall Password Safe."

; "LangString" (for "Function GreenOrRegular") are setup here because they cannot be defined in the function body
LangString TEXT_GC_TITLE ${LANG_ENGLISH} "Installation Type"
LangString TEXT_GC_SUBTITLE ${LANG_ENGLISH} "Choose Regular for use on a single PC, Green for portable installation. If you're not sure, 'Regular' is fine."
LangString PSWINI_TITLE ${LANG_ENGLISH} "Choose Installation Type"
LangString PSWINI_TEXT1 ${LANG_ENGLISH} "Regular (uses Registry, suitable for home - or single user PC)"
LangString PSWINI_TEXT2 ${LANG_ENGLISH} "Green (for Disk-on-Key; does not use - host Registry)"

; several messages on install, check, ...
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LangString RUNNING_INSTALL ${LANG_ENGLISH} "The installer is already running."
LangString RUNNING_APPLICATION ${LANG_ENGLISH} "Please exit all running instances of PasswordSafe before installing a new version"
LangString LANG_INSTALL ${LANG_ENGLISH} "Installation Language"
LangString LANG_SELECT ${LANG_ENGLISH} "Please select the language for the installation"
LangString LANG_PROGRAM ${LANG_ENGLISH} "Program Language"
LangString LANG_P_SELECT ${LANG_ENGLISH} "Please select the language for the program"
LangString SORRY_NO_95 ${LANG_ENGLISH} "Sorry, Windows 95 is no longer supported. Try PasswordSafe 2.16"
LangString SORRY_NO_98 ${LANG_ENGLISH} "Sorry, Windows 98 is no longer supported. Try PasswordSafe 2.16"
LangString SORRY_NO_ME ${LANG_ENGLISH} "Sorry, Windows ME is no longer supported. Try PasswordSafe 2.16"

;--------------------------------
; Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Reserve Files

  ; NSIS documentation states that it's a Good Idea to put the following
  ; two lines when using a custom dialog:  
  ReserveFile "pws-install.ini"
  ;!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS  : VdG for MUI-2 :  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS is no longer supported as InstallOptions

;-----------------------------------------------------------------
; The program itself

Section "$(PROGRAM_FILES)" ProgramFiles
  ;Read the chosen installation type: 1 means "Green", 0 - "Regular"
  !insertmacro INSTALLOPTIONS_READ $INSTALL_TYPE "pws-install.ini" "Field 2" "State"

  ; Make the program files mandatory
  SectionIn RO

  ; Set the directory to install to
  SetOutPath "$INSTDIR"
  
  ; Get all of the files.  This list should be modified when additional
  ; files are added to the release.
  File "..\build\bin\pwsafe\releasem\pwsafe.exe"
  File "..\build\bin\pwsafe\releasem\pws_at.dll"
  File "..\build\bin\pwsafe\releasem\pws_osk.dll"
;"no Win98"  File /oname=p98.exe "..\build\bin\pwsafe\nu-releasem\pwsafe.exe" 
  File "..\help\default\pwsafe.chm"
  File "..\LICENSE"
  File "..\README.TXT"
  File "..\docs\ReleaseNotes.txt"
  File "..\docs\ReleaseNotes.html"
  File "..\docs\ChangeLog.txt"
  File "..\xml\pwsafe.xsd"
  File "..\xml\pwsafe.xsl"
  File "..\xml\pwsafe_filter.xsd"

  
!ifdef LANGUAGE_CHINESE
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeZH.dll"
  File /nonfatal "..\help\pwsafeZH\pwsafeZH.chm"
!endif
!ifdef LANGUAGE_GERMAN
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeDE.dll"
  File /nonfatal "..\help\pwsafeDE\pwsafeDE.chm"
!endif
!ifdef LANGUAGE_SPANISH
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeES.dll"
  File /nonfatal "..\help\pwsafeES\pwsafeES.chm"
!endif
!ifdef LANGUAGE_SWEDISH
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeSV.dll"
  File /nonfatal "..\help\pwsafeSV\pwsafeSV.chm"
!endif
!ifdef LANGUAGE_DUTCH
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeNL.dll"
  File /nonfatal "..\help\pwsafeNL\pwsafeNL.chm"
!endif
!ifdef LANGUAGE_FRENCH
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeFR.dll"
  File /nonfatal "..\help\pwsafeFR\pwsafeFR.chm"
!endif
!ifdef LANGUAGE_RUSSIAN
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeRU.dll"
  File /nonfatal "..\help\pwsafeRU\pwsafeRU.chm"
!endif
!ifdef LANGUAGE_POLISH
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafePL.dll"
  File /nonfatal "..\help\pwsafePL\pwsafePL.chm"
!endif
!ifdef LANGUAGE_ITALIAN
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeIT.dll"
  File /nonfatal "..\help\pwsafeIT\pwsafeIT.chm"
!endif
!ifdef LANGUAGE_DANISH
  File /nonfatal "..\build\bin\pwsafe\I18N\pwsafeDA.dll"
  File /nonfatal "..\help\pwsafeDA\pwsafeDA.chm"
!endif

  Goto dont_install_Win98
  ; If installing under Windows98, delete pwsafe.exe, rename
  ; p98.exe pwsafe.exe
  ; Otherwise, delete p98.exe
  StrCmp $HOST_OS '98' is_98 isnt_98
  is_98:
    Delete $INSTDIR\pwsafe.exe
    Rename $INSTDIR\p98.exe $INSTDIR\pwsafe.exe
    Goto lbl_cont
  isnt_98:
    Delete $INSTDIR\p98.exe
  lbl_cont:
dont_install_Win98:

!ifdef LANGUAGE_CHINESE
  IntCmp $PROG_LANGUAGE 2052 languageChinese
!endif
!ifdef LANGUAGE_SPANISH
  IntCmp $PROG_LANGUAGE 1034 languageSpanish
!endif
!ifdef LANGUAGE_GERMAN
  IntCmp $PROG_LANGUAGE 1031 languageGerman
!endif
!ifdef LANGUAGE_SWEDISH
  IntCmp $PROG_LANGUAGE 1053 languageSwedish
!endif
!ifdef LANGUAGE_DUTCH
  IntCmp $PROG_LANGUAGE 1043 languageDutch
!endif
!ifdef LANGUAGE_FRENCH
  IntCmp $PROG_LANGUAGE 1036 languageFrench
!endif
!ifdef LANGUAGE_RUSSIAN
  IntCmp $PROG_LANGUAGE 1049 languageRussian
!endif
!ifdef LANGUAGE_POLISH
  IntCmp $PROG_LANGUAGE 1045 languagePolish
!endif
!ifdef LANGUAGE_ITALIAN
  IntCmp $PROG_LANGUAGE 1040 languageItalian
!endif
!ifdef LANGUAGE_DANISH
  IntCmp $PROG_LANGUAGE 1030 languageDanish
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
; if language = english or "other" : remove all languageXX_XX.DLL
; else : English or no specific language
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_GERMAN
languageGerman:
!ifdef LANGUAGE_GERMAN
;    Delete $INSTDIR\pwsafeDE.dll
;    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_CHINESE
languageCHINESE:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
;    Delete $INSTDIR\pwsafeZH.dll
;    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_SPANISH
languageSpanish:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
;    Delete $INSTDIR\pwsafeES.dll
;    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_SWEDISH
languageSwedish:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
;    Delete $INSTDIR\pwsafeSV.dll
;    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_DUTCH
languageDutch:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
;    Delete $INSTDIR\pwsafeNL.dll
;    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
 !ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
   goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_FRENCH
languageFrench:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
;    Delete $INSTDIR\pwsafeFR.dll
;    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_RUSSIAN
languageRussian:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
;    Delete $INSTDIR\pwsafeRU.dll
;    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_POLISH
languagePolish:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
;    Delete $INSTDIR\pwsafePL.dll
;    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_ITALIAN
languageItalian:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
;    Delete $INSTDIR\pwsafeIT.dll
;    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
    Delete $INSTDIR\pwsafeDA.dll
    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
!ifdef LANGUAGE_DANISH
languageDanish:
!ifdef LANGUAGE_GERMAN
    Delete $INSTDIR\pwsafeDE.dll
    Delete $INSTDIR\pwsafeDE.chm
!endif
!ifdef LANGUAGE_CHINESE
    Delete $INSTDIR\pwsafeZH.dll
    Delete $INSTDIR\pwsafeZH.chm
!endif
!ifdef LANGUAGE_SPANISH
    Delete $INSTDIR\pwsafeES.dll
    Delete $INSTDIR\pwsafeES.chm
!endif
!ifdef LANGUAGE_SWEDISH
    Delete $INSTDIR\pwsafeSV.dll
    Delete $INSTDIR\pwsafeSV.chm
!endif
!ifdef LANGUAGE_DUTCH
    Delete $INSTDIR\pwsafeNL.dll
    Delete $INSTDIR\pwsafeNL.chm
!endif
!ifdef LANGUAGE_FRENCH
    Delete $INSTDIR\pwsafeFR.dll
    Delete $INSTDIR\pwsafeFR.chm
!endif
!ifdef LANGUAGE_RUSSIAN
    Delete $INSTDIR\pwsafeRU.dll
    Delete $INSTDIR\pwsafeRU.chm
!endif
!ifdef LANGUAGE_POLISH
    Delete $INSTDIR\pwsafePL.dll
    Delete $INSTDIR\pwsafePL.chm
!endif
!ifdef LANGUAGE_ITALIAN
    Delete $INSTDIR\pwsafeIT.dll
    Delete $INSTDIR\pwsafeIT.chm
!endif
!ifdef LANGUAGE_DANISH
;    Delete $INSTDIR\pwsafeDA.dll
;    Delete $INSTDIR\pwsafeDA.chm
!endif
    goto languageDone
!endif
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
languageDone:

  ; skip over registry writes if 'Green' installation selected
  IntCmp $INSTALL_TYPE 1 GreenInstall

  ; Store installation folder
  WriteRegStr HKCU "Software\Password Safe\Password Safe" "installdir" $INSTDIR

  ; Store the version
  WriteRegStr HKCU "Software\Password Safe\Password Safe" "installversion" "${VERSION}"
  
  ; and the language
  WriteRegStr HKCU "Software\Password Safe\Password Safe" "Language" "$LANGUAGE"
  WriteRegStr HKCU "Software\Password Safe\Password Safe" "Program" "$PROG_LANGUAGE"
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Add the uninstaller to the Add/Remove Programs window.  If the 
  ; current user doesn't have permission to write to HKLM, then the
  ; uninstaller will not appear in the Add or Remove Programs window.
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Password Safe" \
        "DisplayName" "Password Safe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Password Safe" \
         "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Password Safe" \
    "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Password Safe" \
    "NoRepair" 1
GreenInstall:
SectionEnd

;--------------------------------
; Start with Windows
Section "$(START_AUTO)" StartUp
  CreateShortCut "$SMSTARTUP\Password Safe.lnk" "$INSTDIR\pwsafe.exe" "-s"
SectionEnd

;--------------------------------
; Start menu

Section "$(START_SHOW)" StartMenu

  ; Create the Password Safe menu under the programs part of the start menu
  CreateDirectory "$SMPROGRAMS\Password Safe"

  ; Create shortcuts
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe.lnk" "$INSTDIR\pwsafe.exe"

  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help.lnk" "$INSTDIR\pwsafe.chm"
  
!ifdef LANGUAGE_CHINESE
  IntCmp $PROG_LANGUAGE 2052 useLanguageChinese
!endif
!ifdef LANGUAGE_SPANISH
  IntCmp $PROG_LANGUAGE 1034 useLanguageSpanish
!endif
!ifdef LANGUAGE_GERMAN
  IntCmp $PROG_LANGUAGE 1031 useLanguageGerman
!endif
!ifdef LANGUAGE_SWEDISH
  IntCmp $PROG_LANGUAGE 1053 useLanguageSwedish
!endif
!ifdef LANGUAGE_DUTCH
  IntCmp $PROG_LANGUAGE 1043 useLanguageDutch
!endif
!ifdef LANGUAGE_FRENCH
  IntCmp $PROG_LANGUAGE 1036 useLanguageFrench
!endif
!ifdef LANGUAGE_RUSSIAN
  IntCmp $PROG_LANGUAGE 1049 useLanguageRussian
!endif
!ifdef LANGUAGE_POLISH
  IntCmp $PROG_LANGUAGE 1045 useLanguagePolish
!endif
!ifdef LANGUAGE_ITALIAN
  IntCmp $PROG_LANGUAGE 1040 useLanguageItalian
!endif
!ifdef LANGUAGE_DANISH
  IntCmp $PROG_LANGUAGE 1030 useLanguageDanish
!endif
  Goto languageChoicedone

!ifdef LANGUAGE_CHINESE
useLanguageChinese:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help ZH.lnk" "$INSTDIR\pwsafeZH.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_SPANISH
useLanguageSpanish:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help ES.lnk" "$INSTDIR\pwsafeES.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_GERMAN
useLanguageGerman:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help DE.lnk" "$INSTDIR\pwsafeDE.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_SWEDISH
useLanguageSwedish:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help SV.lnk" "$INSTDIR\pwsafeSV.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_DUTCH
useLanguageDutch:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help NL.lnk" "$INSTDIR\pwsafeNL.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_FRENCH
useLanguageFrench:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help FR.lnk" "$INSTDIR\pwsafeFR.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_RUSSIAN
useLanguageRussian:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help RU.lnk" "$INSTDIR\pwsafeRU.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_POLISH
useLanguagePolish:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help PL.lnk" "$INSTDIR\pwsafePL.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_ITALIAN
useLanguageItalian:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help IT.lnk" "$INSTDIR\pwsafeIT.chm"
  Goto languageChoicedone
!endif
!ifdef LANGUAGE_DANISH
useLanguageDanish:
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Help DA.lnk" "$INSTDIR\pwsafeDA.chm"
  Goto languageChoicedone
!endif

languageChoicedone:
SectionEnd

;--------------------------------
; PasswordSafe Uninstall

Section "$(UNINSTALL_SHORTCUT)" UninstallMenu

  ; Create the Password Safe menu under the programs part of the start menu
  ; should be already available (START_SHOW)
  CreateDirectory "$SMPROGRAMS\Password Safe"

  ; Create Uninstall icon
  CreateShortCut "$SMPROGRAMS\Password Safe\Password Safe Uninstall.lnk" "$INSTDIR\Uninstall.exe"

SectionEnd

;--------------------------------
; Desktop shortcut

Section "$(START_SHORTCUT)" DesktopShortcut

  ; Create desktop icon
  CreateShortCut "$DESKTOP\Password Safe.lnk" "$INSTDIR\pwsafe.exe"

SectionEnd

;--------------------------------
; Descriptions
  
  ; Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${ProgramFiles} $(DESC_ProgramFiles)
    !insertmacro MUI_DESCRIPTION_TEXT ${StartUp} $(DESC_StartUp)
    !insertmacro MUI_DESCRIPTION_TEXT ${StartMenu} $(DESC_StartMenu)
    !insertmacro MUI_DESCRIPTION_TEXT ${UninstallMenu} $(DESC_UninstallMenu)
    !insertmacro MUI_DESCRIPTION_TEXT ${DesktopShortcut} $(DESC_DesktopShortcut)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller Section

Section "Uninstall"

; Now protect against running instance of pwsafe
	${nsProcess::FindProcess} "pwsafe.exe" $R0
	StrCmp $R0 0 0 +3
	MessageBox MB_OK $(RUNNING_APPLICATION)
  Abort
	${nsProcess::Unload}
  ; Delete all installed files in the directory
  Delete "$INSTDIR\pwsafe.exe"
  Delete "$INSTDIR\pws_at.dll"
  Delete "$INSTDIR\pws_osk.dll"
  Delete "$INSTDIR\pwsafe.chm"
  Delete "$INSTDIR\pwsafe.xsd"
  Delete "$INSTDIR\pwsafe.xsl"
  Delete "$INSTDIR\pwsafe_filter.xsd"
  Delete "$INSTDIR\LICENSE"
  Delete "$INSTDIR\README.TXT"
  Delete "$INSTDIR\ReleaseNotes.txt"
  Delete "$INSTDIR\ReleaseNotes.html"
  Delete "$INSTDIR\ChangeLog.txt"
!ifdef LANGUAGE_GERMAN
  Delete "$INSTDIR\pwsafeDE.dll"
  Delete "$INSTDIR\pwsafeDE.chm"
!endif
!ifdef LANGUAGE_CHINESE
  Delete "$INSTDIR\pwsafeZH.dll"
  Delete "$INSTDIR\pwsafeZH.chm"
!endif
!ifdef LANGUAGE_SPANISH
  Delete "$INSTDIR\pwsafeES.dll"
  Delete "$INSTDIR\pwsafeES.chm"
!endif
!ifdef LANGUAGE_SWEDISH
  Delete "$INSTDIR\pwsafeSV.dll"
  Delete "$INSTDIR\pwsafeSV.chm"
!endif
!ifdef LANGUAGE_DUTCH
  Delete "$INSTDIR\pwsafeNL.dll"
  Delete "$INSTDIR\pwsafeNL.chm"
!endif
!ifdef LANGUAGE_FRENCH
  Delete "$INSTDIR\pwsafeFR.dll"
  Delete "$INSTDIR\pwsafeFR.chm"
!endif
!ifdef LANGUAGE_RUSSIAN
  Delete "$INSTDIR\pwsafeRU.dll"
  Delete "$INSTDIR\pwsafeRU.chm"
!endif
!ifdef LANGUAGE_POLISH
  Delete "$INSTDIR\pwsafePL.dll"
  Delete "$INSTDIR\pwsafePL.chm"
!endif
!ifdef LANGUAGE_ITALIAN
  Delete "$INSTDIR\pwsafeIT.dll"
  Delete "$INSTDIR\pwsafeIT.chm"
!endif
!ifdef LANGUAGE_DANISH
  Delete "$INSTDIR\pwsafeDA.dll"
  Delete "$INSTDIR\pwsafeDA.chm"
!endif

  ; remove directory if it's empty
  RMDir  "$INSTDIR"

  ; Delete the registry key for Password Safe
  DeleteRegKey HKCU "Software\Password Safe\Password Safe"

  ; Delete the registry key for the Add or Remove Programs window.  If
  ; the current user doesn't have permission to delete registry keys
  ; from HKLM, then the entry in the Add or Remove Programs window will
  ; remain.  The next time a user tries to click on the uninstaller,
  ; they will be prompted to remove the entry.
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Password Safe"
  ; Delete shortcuts, if created
  Delete "$SMPROGRAMS\Password Safe\Password Safe.lnk"
  Delete "$SMPROGRAMS\Password Safe\Password Safe Help.lnk"
  RMDir /r "$SMPROGRAMS\Password Safe"
  Delete "$DESKTOP\Password Safe.lnk"
  Delete "$SMSTARTUP\Password Safe.lnk"

SectionEnd

;-----------------------------------------
; Functions
Function .onInit

 ;Extract InstallOptions INI files
 !insertmacro INSTALLOPTIONS_EXTRACT "pws-install.ini"
 Call GetWindowsVersion
 Pop $R0
; Strcpy $R0 '98' ; ONLY for test
 StrCmp $R0 '95' is_win95
 StrCmp $R0 '98' is_win98
 StrCmp $R0 'ME' is_winME
 StrCpy $HOST_OS $R0

; Following tests should really be done
; after .onInit, since language is initialized
; then. However, there's no easy way to do this,
; so these error messages will only be in English. Sorry. 
; Protect against multiple instances
; of the installer

 System::Call 'kernel32::CreateMutexA(i 0, i 0, t "pwsInstallMutex") i .r1 ?e'
 Pop $R0 
 StrCmp $R0 0 +3
   MessageBox MB_OK|MB_ICONEXCLAMATION $(RUNNING_INSTALL)
   Abort

; Now protect against running instance of pwsafe
	${nsProcess::FindProcess} "pwsafe.exe" $R0
	StrCmp $R0 0 0 +3
	MessageBox MB_OK $(RUNNING_APPLICATION)
  Abort
	${nsProcess::Unload}

  ;Language selection dialog
  ; (1) Installation language

!ifdef LANGUAGE_GERMAN
  goto extraLanguage
!endif
!ifdef LANGUAGE_CHINESE
  goto extraLanguage
!endif
!ifdef LANGUAGE_SPANISH
  goto extraLanguage
!endif
!ifdef LANGUAGE_SWEDISH
  goto extraLanguage
!endif
!ifdef LANGUAGE_DUTCH
  goto extraLanguage
!endif
!ifdef LANGUAGE_FRENCH
  goto extraLanguage
!endif
!ifdef LANGUAGE_RUSSIAN
  goto extraLanguage
!endif
!ifdef LANGUAGE_POLISH
  goto extraLanguage
!endif
!ifdef LANGUAGE_ITALIAN
  goto extraLanguage
!endif
!ifdef LANGUAGE_DANISH
  goto extraLanguage
!endif
  goto NOextraLanguage
 
extraLanguage:  
  Push ""
  Push ${LANG_ENGLISH}
  Push English
!ifdef LANGUAGE_GERMAN
  Push ${LANG_GERMAN}
  Push Deutsch
!endif
!ifdef LANGUAGE_CHINESE
  Push ${LANG_SIMPCHINESE}
  Push Chinese
!endif
!ifdef LANGUAGE_SPANISH
  Push ${LANG_SPANISH}
  Push Espanol
!endif
!ifdef LANGUAGE_SWEDISH
  Push ${LANG_SWEDISH}
  Push Svensk
!endif
!ifdef LANGUAGE_DUTCH
  Push ${LANG_DUTCH}
  Push Dutch
!endif
!ifdef LANGUAGE_FRENCH
  Push ${LANG_FRENCH}
  Push Francais
!endif
!ifdef LANGUAGE_RUSSIAN
  Push ${LANG_RUSSIAN}
  Push Russian
!endif
!ifdef LANGUAGE_POLISH
  Push ${LANG_POLISH}
  Push Polska
!endif
!ifdef LANGUAGE_ITALIAN
  Push ${LANG_ITALIAN}
  Push Italiano
!endif
!ifdef LANGUAGE_DANISH
  Push ${LANG_DANISH}
  Push Dansk
!endif
  Push A ; A means auto count languages
         ; for the auto count to work the first empty push (Push "") must remain
  LangDLL::LangDialog $(LANG_INSTALL) $(LANG_SELECT)

  Pop $LANGUAGE
  StrCmp $LANGUAGE "cancel" 0 +2
  Abort
  
  ; (2) Password Safe language
!ifdef LANGUAGE_GERMAN
  goto programLanguage
!endif
!ifdef LANGUAGE_CHINESE
  goto programLanguage
!endif
!ifdef LANGUAGE_SPANISH
  goto programLanguage
!endif
!ifdef LANGUAGE_SWEDISH
  goto programLanguage
!endif
!ifdef LANGUAGE_DUTCH
  goto programLanguage
!endif
!ifdef LANGUAGE_FRENCH
  goto programLanguage
!endif
!ifdef LANGUAGE_RUSSIAN
  goto programLanguage
!endif
!ifdef LANGUAGE_POLISH
  goto programLanguage
!endif
!ifdef LANGUAGE_ITALIAN
  goto programLanguage
!endif
!ifdef LANGUAGE_DANISH
  goto programLanguage
!endif
  goto NOprogramLanguage
 
programLanguage:  
  Push ""
  Push ${LANG_ENGLISH}
  Push English
!ifdef LANGUAGE_GERMAN
  Push ${LANG_GERMAN}
  Push Deutsch
!endif
!ifdef LANGUAGE_CHINESE
  Push ${LANG_SIMPCHINESE}
  Push Chinese
!endif
!ifdef LANGUAGE_SPANISH
  Push ${LANG_SPANISH}
  Push Espanol
!endif
!ifdef LANGUAGE_SWEDISH
  Push ${LANG_SWEDISH}
  Push Svensk
!endif
!ifdef LANGUAGE_DUTCH
  Push ${LANG_DUTCH}
  Push Dutch
!endif
!ifdef LANGUAGE_FRENCH
  Push ${LANG_FRENCH}
  Push Francais
!endif
!ifdef LANGUAGE_RUSSIAN
  Push ${LANG_RUSSIAN}
  Push Russian
!endif
!ifdef LANGUAGE_POLISH
  Push ${LANG_POLISH}
  Push Polska
!endif
!ifdef LANGUAGE_ITALIAN
  Push ${LANG_ITALIAN}
  Push Italiano
!endif
!ifdef LANGUAGE_DANISH
  Push ${LANG_DANISH}
  Push Dansk
!endif
  Push A ; A means auto count languages
         ; for the auto count to work the first empty push (Push "") must remain
  LangDLL::LangDialog $(LANG_PROGRAM) $(LANG_P_SELECT)

  Pop $PROG_LANGUAGE
  StrCmp $PROG_LANGUAGE "cancel" 0 +2
  Abort
  
  Return
  ; - - - - - - - - - - - - - - - - - - - - - - - - - - - -
is_win95:
  MessageBox MB_OK|MB_ICONSTOP $(SORRY_NO_95)
  Quit
is_win98:
  MessageBox MB_OK|MB_ICONSTOP $(SORRY_NO_98)
  Quit
is_winME:
  MessageBox MB_OK|MB_ICONSTOP $(SORRY_NO_ME)
  Quit
NOextraLanguage:
NOprogramLanguage:

FunctionEnd

Function GreenOrRegular
  !insertmacro MUI_HEADER_TEXT "$(TEXT_GC_TITLE)" "$(TEXT_GC_SUBTITLE)"
  ; english is in "pws-install.ini" by default, so no writing necesarry
  !insertmacro INSTALLOPTIONS_WRITE "pws-install.ini" "Settings" "Title" $(PSWINI_TITLE)
  !insertmacro INSTALLOPTIONS_WRITE "pws-install.ini" "Field 1" "Text" $(PSWINI_TEXT1)
  !insertmacro INSTALLOPTIONS_WRITE "pws-install.ini" "Field 2" "Text" $(PSWINI_TEXT2)
  !insertmacro INSTALLOPTIONS_DISPLAY "pws-install.ini"
FunctionEnd
  
 ;--------------------------------
 ;
 ; Based on Yazno's function, http://yazno.tripod.com/powerpimpit/
 ; Updated by Joost Verburg
 ;
 ; Returns on top of stack
 ;
 ; Windows Version (95, 98, ME, NT x.x, 2000, XP, 2003, Vista)
 ; or
 ; '' (Unknown Windows Version)
 ;
 ; Usage:
 ;   Call GetWindowsVersion
 ;   Pop $R0
 ;   ; at this point $R0 is "NT 4.0" or whatnot
 
 Function GetWindowsVersion
 
   Push $R0
   Push $R1
 
   ClearErrors
 
   ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion

   IfErrors 0 lbl_winnt
   
   ; we are not NT
   ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber
 
   StrCpy $R1 $R0 1
   StrCmp $R1 '4' 0 lbl_error
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '4.0' lbl_win32_95
   StrCmp $R1 '4.9' lbl_win32_ME lbl_win32_98
 
   lbl_win32_95:
     StrCpy $R0 '95'
   Goto lbl_done
 
   lbl_win32_98:
     StrCpy $R0 '98'
   Goto lbl_done
 
   lbl_win32_ME:
     StrCpy $R0 'ME'
   Goto lbl_done
 
   lbl_winnt:
 
   StrCpy $R1 $R0 1
 
   StrCmp $R1 '3' lbl_winnt_x
   StrCmp $R1 '4' lbl_winnt_x
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '5.0' lbl_winnt_2000
   StrCmp $R1 '5.1' lbl_winnt_XP
   StrCmp $R1 '5.2' lbl_winnt_2003
   StrCmp $R1 '6.0' lbl_winnt_vista lbl_error
 
   lbl_winnt_x:
     StrCpy $R0 "NT $R0" 6
   Goto lbl_done
 
   lbl_winnt_2000:
     Strcpy $R0 '2000'
   Goto lbl_done
 
   lbl_winnt_XP:
     Strcpy $R0 'XP'
   Goto lbl_done
 
   lbl_winnt_2003:
     Strcpy $R0 '2003'
   Goto lbl_done
 
   lbl_winnt_vista:
     Strcpy $R0 'Vista'
   Goto lbl_done
 
   lbl_error:
     Strcpy $R0 ''
   lbl_done:
 
   Pop $R1
   Exch $R0
 
 FunctionEnd
