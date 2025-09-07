########################################################################
# Makefile for RHSCalc application
########################################################################

# Project name

projname = rhscalc

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -I..\Classlib -DUNICODE -D_UNICODE
lflags   = -subsystem:windows -entry:WinMainCRTStartup
# cflags   = -c -Od -Zi -G3 -I..\Classlib -DWIN32
# lflags   = -subsystem:windows -entry:WinMainCRTStartup -debug:full -debugtype:cv

# Objects

objs     = RHSCalc.obj \
           CRhsRegistry.obj

# Libraries

libs     = kernel32.lib user32.lib gdi32.lib \
           comdlg32.lib shell32.lib advapi32.lib

# Inference rules

.cpp.obj:
   $(cc) $(cflags) $*.cpp -Fo$*.obj

.c.obj:
   $(cc) $(cflags) $*.c -Fo$*.obj

# Targets

all: $(projname).exe

$(projname).exe: $(objs) $(projname).res
    $(link) $(lflags) -out:$(projname).exe $(objs) $(projname).res $(libs)
    copy /Y $(projname).exe ..\..\bin

$(projname).res: $(projname).rc RHSCalcDlg.r
    rc -r -DWIN32 $(projname).rc

CRhsRegistry.obj: ..\Classlib\Misc\CRhsRegistry.cpp
   $(cc) $(cflags)  ..\Classlib\Misc\CRhsRegistry.cpp -FoCRhsRegistry.obj
