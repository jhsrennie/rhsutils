########################################################################
# Makefile for getsntptime
########################################################################

# Project name

projname = getsntptime

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -D_X86_=1 -I..\Classlib -DWIN32
lflags   = -subsystem:console
# cflags   = -c -W3 -Od -Zi -D_X86_=1 -DWIN32
# lflags   = -subsystem:console -debug:full -debugtype:cv

# Objects

objs     = $(projname).obj CSNTPTimestamp.obj CRhsSocket.obj

# Libraries

libs     = kernel32.lib user32.lib gdi32.lib wsock32.lib \
           comdlg32.lib shell32.lib advapi32.lib comctl32.lib

# Inference rules

.cpp.obj:
   $(cc) $(cflags) $*.cpp -Fo$*.obj

# Targets

all: $(projname).exe

$(projname).exe: $(objs) $(projname).res
    $(link) $(lflags) -out:$(projname).exe $(objs) $(projname).res $(libs)
    copy /Y $(projname).exe ..\..\bin

CRhsSocket.obj: ..\Classlib\CRhsSocket\CRhsSocket.cpp
   $(cc) $(cflags) ..\Classlib\CRhsSocket\CRhsSocket.cpp -FoCRhsSocket.obj

$(projname).res: $(projname).rc
    rc -r -DWIN32 $(projname).rc
