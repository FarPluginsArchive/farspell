!ifndef TARGET
TARGET = Debug
!endif            
!message Target is $(TARGET)

LEMON = ..\make-tools\lemon.exe
COMMONFLAGS = -nologo -Fo$(OUTDIR)\  -I..\..\..\PlugDoc\Headers.c
CLIB = lib -nologo 

!if "$(TARGET)"=="Release"
LEMONFLAGS =
CC = cl $(COMMONFLAGS) -GL -Og -Ox -DNDEBUG 
OUTDIR = .msvc.release
!endif

!if "$(TARGET)"=="Aided Release"
LEMONFLAGS = -c
CC = cl $(COMMONFLAGS) -GL -Og -Ox -DNDEBUG
CLIB = lib -nologo
OUTDIR = .msvc.aided-release
!endif

!if "$(TARGET)"=="Debug"
LEMONFLAGS = -c
CC = cl $(COMMONFLAGS) -Od -Fd -D_DEBUG
CLIB = lib -nologo -debug
LINKFLAGS = -debug
OUTDIR = .msvc.debug
!endif

all: executables $(OUTDIR)\dialogres.pdll
debug: all
release:
	-@$(MAKE) -nologo TARGET=Release all
aided:
	-@$(MAKE) -nologo "TARGET=Aided Release" all

!if "$(TARGET)"!="Release"
# Prevents from build of something useless cdialog.exe
executables: cdialog.exe$(OUTDIR)
$(OUTDIR)\cdialog.exe: cdialog.c $(OUTDIR)\dialogres_static.lib
	$(CC) -o $@ cdialog.c $(OUTDIR)\dialogres_static.lib $(EXEFLAGS)
cdialog.exe$(OUTDIR): $(OUTDIR)\cdialog.exe
	-del cdialog.exe.*.aided-release
	-del cdialog.exe.*.release
	-del cdialog.exe.*.debug
	copy "$(OUTDIR)\cdialog.exe" cdialog.exe
	echo Build stamp > cdialog.exe$(OUTDIR)
!else
executables: cdialog.exe.msvc.aided-release
cdialog.exe.msvc.aided-release:
	$(MAKE) TARGET="Aided Release" cdialog.exe.msvc.aided-release
!endif

$(OUTDIR)\dialogres.pdll: $(OUTDIR)\dialogres_static.lib $(OUTDIR)\dialogres_dll.obj dialogres.def
	link -nologo -dll -out:$@ $(OUTDIR)\dialogres_dll.obj \
		$(OUTDIR)\dialogres_static.lib \
		-def:dialogres.def \
		-map:$(OUTDIR)\dialogres.map \
		-pdb:$(OUTDIR)\dialogres.pdb $(LINKFLAGS)

$(OUTDIR)\dialogres_static.lib: $(OUTDIR)\parse.obj $(OUTDIR)\tokenize.obj $(OUTDIR)\dialogres.obj
	$(CLIB) -out:"$@" $**

$(OUTDIR)\dialogres_dll.obj: dialogres_dll.c 
	$(CC) -c dialogres_dll.c 
	
dialogresInt.h: dialogres.h
$(OUTDIR)\dialogres.obj: dialogres.c dialogresInt.h
	$(CC) -c dialogres.c 
$(OUTDIR)\tokenize.obj: tokenize.c $(OUTDIR)\parse.h keywordhash.h
	$(CC) -c tokenize.c  -I$(OUTDIR)
$(OUTDIR)\parse.obj: $(OUTDIR)\parse.c $(OUTDIR)\parse.h dialogresInt.h
	$(CC) -c $(OUTDIR)\parse.c -I$(OUTDIR)

keywordhash.h: $(OUTDIR)\mkkeywordhash.exe keywords.txt
	$(OUTDIR)\mkkeywordhash.exe < keywords.txt > keywordhash.h 2>nul

$(OUTDIR)\parse.c $(OUTDIR)\parse.h: parse.y 
	$(MAKE) -nologo $(LEMON)
	$(LEMON) $(LEMONFLAGS) parse.y
	-mkdir $(OUTDIR)
	move parse.c $(OUTDIR)\parse.c
	move parse.h $(OUTDIR)\parse.h

$(OUTDIR)\mkkeywordhash.exe: mkkeywordhash.c 
	$(MAKE) -nologo $(OUTDIR)
	$(CC) -o $@ mkkeywordhash.c 

$(LEMON):
	cd ..\make-tools
	$(MAKE) -nologo -f makefile.vc lemon.exe
	cd "$(MAKEDIR)"

clean:
	-del parse.c
	-del parse.h
	-del parse.out
	-del keywordhash.h
	-del mkkeywordhash.exe 
	-del cdialog.exe*
	-del *.obj
	-del *.lib
	-del *.exp
	-rmdir /S /Q .msvc.release
	-rmdir /S /Q .msvc.aided-release
	-rmdir /S /Q .msvc.debug

distclean: clean
	cd ..\make-tools
	$(MAKE) -nologo -f makefile.vc clean
	cd "$(MAKEDIR)"
	