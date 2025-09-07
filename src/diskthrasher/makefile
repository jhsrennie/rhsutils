########################################################################
# Makefile for diskthrasher
########################################################################

# Project name

projname = diskthrasher

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -D_X86_=1 -DWIN32
lflags   = -subsystem:console
# -entry:WinMainCRTStartup
# cflags   = -c -W3 -Od -Zi -D_X86_=1 -DWIN32
# lflags   = -debug:full -debugtype:cv

# Objects

objs     = $(projname).obj

# Libraries

libs     = kernel32.lib user32.lib gdi32.lib ole32.lib \
           comdlg32.lib shell32.lib advapi32.lib comctl32.lib

# Inference rules

.cpp.obj:
   $(cc) $(cflags) $*.cpp -Fo$*.obj

# Targets

all: $(projname).exe

$(projname).exe: $(objs) $(projname).res
    $(link) $(lflags) -out:$(projname).exe $(objs) $(projname).res $(libs)
    copy /Y $(projname).exe ..\..\bin

$(projname).res: $(projname).rc ..\Classlib\CRhsIO\CRhsIOWnd.rc ..\Classlib\CRhsIO\CRhsIOWnd.h
    rc -r -I..\Classlib -DWIN32 $(projname).rc
