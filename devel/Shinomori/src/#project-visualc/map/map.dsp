# Microsoft Developer Studio Project File - Name="map" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=map - Win32 Debug
!MESSAGE Dies ist kein gultiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und fuhren Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "map.mak".
!MESSAGE 
!MESSAGE Sie konnen beim Ausfuhren von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "map.mak" CFG="map - Win32 Debug"
!MESSAGE 
!MESSAGE Fur die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "map - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "map - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "map - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../common" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "TXT_ONLY" /FR /YX /FD /c /Tp
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib common.lib zlib.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libcmt.lib msvcrt.lib libcd.lib libcmtd.lib msvcrtd.lib" /out:"../map.exe" /libpath:".."

!ELSEIF  "$(CFG)" == "map - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../common" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "TXT_ONLY" /YX /FD /GZ /c /Tp
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib commond.lib zlib1.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libc.lib libcmt.lib msvcrt.lib libcmtd.lib msvcrtd.lib" /out:"../map.exe" /pdbtype:sept /libpath:".."

!ENDIF 

# Begin Target

# Name "map - Win32 Release"
# Name "map - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\map\atcommand.c
# End Source File
# Begin Source File

SOURCE=..\..\map\battle.c
# End Source File
# Begin Source File

SOURCE=..\..\map\charcommand.c
# End Source File
# Begin Source File

SOURCE=..\..\map\chat.c
# End Source File
# Begin Source File

SOURCE=..\..\map\chrif.c
# End Source File
# Begin Source File

SOURCE=..\..\map\clif.c
# End Source File
# Begin Source File

SOURCE=..\..\map\guild.c
# End Source File
# Begin Source File

SOURCE=..\..\map\intif.c
# End Source File
# Begin Source File

SOURCE=..\..\map\itemdb.c
# End Source File
# Begin Source File

SOURCE=..\..\map\log.c
# End Source File
# Begin Source File

SOURCE=..\..\map\map.c
# End Source File
# Begin Source File

SOURCE=..\..\map\mob.c
# End Source File
# Begin Source File

SOURCE=..\..\map\npc.c
# End Source File
# Begin Source File

SOURCE=..\..\map\npc_chat.c
# End Source File
# Begin Source File

SOURCE=..\..\map\party.c
# End Source File
# Begin Source File

SOURCE=..\..\map\path.c
# End Source File
# Begin Source File

SOURCE=..\..\map\pc.c
# End Source File
# Begin Source File

SOURCE=..\..\map\pet.c
# End Source File
# Begin Source File

SOURCE=..\..\map\script.c
# End Source File
# Begin Source File

SOURCE=..\..\map\skill.c
# End Source File
# Begin Source File

SOURCE=..\..\map\status.c
# End Source File
# Begin Source File

SOURCE=..\..\map\storage.c
# End Source File
# Begin Source File

SOURCE=..\..\map\trade.c
# End Source File
# Begin Source File

SOURCE=..\..\map\vending.c
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\map\atcommand.h
# End Source File
# Begin Source File

SOURCE=..\..\map\battle.h
# End Source File
# Begin Source File

SOURCE=..\..\map\charcommand.h
# End Source File
# Begin Source File

SOURCE=..\..\map\chat.h
# End Source File
# Begin Source File

SOURCE=..\..\map\chrif.h
# End Source File
# Begin Source File

SOURCE=..\..\map\clif.h
# End Source File
# Begin Source File

SOURCE=..\..\map\guild.h
# End Source File
# Begin Source File

SOURCE=..\..\map\intif.h
# End Source File
# Begin Source File

SOURCE=..\..\map\itemdb.h
# End Source File
# Begin Source File

SOURCE=..\..\map\log.h
# End Source File
# Begin Source File

SOURCE=..\..\map\map.h
# End Source File
# Begin Source File

SOURCE=..\..\map\mob.h
# End Source File
# Begin Source File

SOURCE=..\..\map\npc.h
# End Source File
# Begin Source File

SOURCE=..\..\map\party.h
# End Source File
# Begin Source File

SOURCE=..\..\map\pc.h
# End Source File
# Begin Source File

SOURCE=..\..\map\pet.h
# End Source File
# Begin Source File

SOURCE=..\..\map\script.h
# End Source File
# Begin Source File

SOURCE=..\..\map\skill.h
# End Source File
# Begin Source File

SOURCE=..\..\map\status.h
# End Source File
# Begin Source File

SOURCE=..\..\map\storage.h
# End Source File
# Begin Source File

SOURCE=..\..\map\trade.h
# End Source File
# Begin Source File

SOURCE=..\..\map\vending.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
