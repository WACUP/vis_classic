Name "Classic Spectrum Analyzer"
CRCCheck on
ComponentText "This will install Classic Spectrum Analyzer for Winamp.$\r$\nIf you are upgrading, you should close Winamp."
OutFile "vis_classic.exe"
SetCompress auto
SetCompressor lzma

; The default installation directory
InstallDir $PROGRAMFILES\Winamp

; detect winamp path from uninstall string if available
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

DirText "Locate your Winamp directory"

LicenseText "Classic Spectrum Analyzer License"
LicenseData license.txt

SetDatablockOptimize on

Section "Classic Spectrum Analyzer"
SetOverwrite on
SetOutPath $INSTDIR\Plugins
File Release\vis_classic.dll
SectionEnd

Section "Sample Profiles"
SetOverwrite on
SetOutPath $INSTDIR\Plugins
File vis_classic.ini
SetOutPath $INSTDIR\Plugins\vis_classic
File "profiles\Aurora Borealis.ini"
File "profiles\BackAMP StoneAge.ini"
File "profiles\Blue Flames.ini"
File "profiles\Blue on Grey.ini"
File "profiles\ChaNinja.ini"
File "profiles\City Night.ini"
File "profiles\Classic LED.ini"
File "profiles\Classic.ini"
File "profiles\Default Red & Yellow.ini"
File "profiles\Flames.ini"
File "profiles\flock darkmateria.ini"
File "profiles\Lavender Pink Tips.ini"
File "profiles\LCD.ini"
File "profiles\Lightning.ini"
File "profiles\Matches.ini"
File "profiles\Northern Lights.ini"
File "profiles\poo.ini"
File "profiles\Purple Neon.ini"
File "profiles\Rainbow.ini"
File "profiles\Trippy.ini"
File "profiles\Twilight.ini"
SectionEnd

InstallColors /windows
ShowInstDetails show

Function .onVerifyInstDir
	IfFileExists $INSTDIR\Winamp.exe PathGood
		Abort ; if $INSTDIR is not a winamp directory, don't let us install there
PathGood:
FunctionEnd
	
!packhdr tmp.exe "upx.exe --best tmp.exe"
