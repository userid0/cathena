# Microsoft Developer Studio Project File - Name="common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=common - Win32 Debug
!MESSAGE Dies ist kein gultiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und fuhren Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "common.mak".
!MESSAGE 
!MESSAGE Sie konnen beim Ausfuhren von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "common.mak" CFG="common - Win32 Debug"
!MESSAGE 
!MESSAGE Fur die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "common - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "common - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "common - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../zlib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /c /Tp
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../common.lib"

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../zlib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c /Tp
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../commond.lib"

!ENDIF 

# Begin Target

# Name "common - Win32 Release"
# Name "common - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\common\core.c
# End Source File
# Begin Source File

SOURCE=..\..\common\db.c
# End Source File
# Begin Source File

SOURCE=..\..\common\grfio.c
# End Source File
# Begin Source File

SOURCE=..\..\common\lock.c
# End Source File
# Begin Source File

SOURCE=..\..\common\malloc.c
# End Source File
# Begin Source File

SOURCE=..\..\common\md5calc.c
# End Source File
# Begin Source File

SOURCE=..\..\common\nullpo.c
# End Source File
# Begin Source File

SOURCE=..\..\common\showmsg.c
# End Source File
# Begin Source File

SOURCE=..\..\common\socket.c
# End Source File
# Begin Source File

SOURCE=..\..\common\strlib.c
# End Source File
# Begin Source File

SOURCE=..\..\common\timer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\utils.c
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\common\base.h
# End Source File
# Begin Source File

SOURCE=..\..\common\core.h
# End Source File
# Begin Source File

SOURCE=..\..\common\db.h
# End Source File
# Begin Source File

SOURCE=..\..\common\grfio.h
# End Source File
# Begin Source File

SOURCE=..\..\common\lock.h
# End Source File
# Begin Source File

SOURCE=..\..\common\malloc.h
# End Source File
# Begin Source File

SOURCE=..\..\common\md5calc.h
# End Source File
# Begin Source File

SOURCE=..\..\common\mmo.h
# End Source File
# Begin Source File

SOURCE=..\..\common\nullpo.h
# End Source File
# Begin Source File

SOURCE=..\..\common\showmsg.h
# End Source File
# Begin Source File

SOURCE=..\..\common\socket.h
# End Source File
# Begin Source File

SOURCE=..\..\common\strlib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\timer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\utils.h
# End Source File
# Begin Source File

SOURCE=..\..\common\version.h
# End Source File
# End Group
# End Target
# End Project
