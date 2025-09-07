########################################################################
# Makefile for pop3util
########################################################################

# Project name

projname = pop3util

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -D_X86_=1 -I..\Classlib -DWIN32 -D_CRT_SECURE_NO_WARNINGS
lflags   = -subsystem:console
# -entry:WinMainCRTStartup
# cflags   = -c -W3 -Od -Zi -D_X86_=1 -DWIN32
# lflags   = -debug:full -debugtype:cv

# Objects

objs     = $(projname).obj CRhsPOP3Connection.obj CRhsPOP3Message.obj \
           utils.obj CRhsSocket.obj

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

$(projname).res: $(projname).rc
    rc -r -I..\Classlib -DWIN32 $(projname).rc

CRhsPOP3Connection.obj: CRhsPOP3Connection.cpp
   $(cc) $(cflags) CRhsPOP3Connection.cpp -FoCRhsPOP3Connection.obj

CRhsPOP3Message.obj: CRhsPOP3Message.cpp
   $(cc) $(cflags) CRhsPOP3Message.cpp -FoCRhsPOP3Message.obj

utils.obj: utils.cpp
   $(cc) $(cflags) utils.cpp -Foutils.obj

CRhsSocket.obj: ..\Classlib\CRhsSocket\CRhsSocket.cpp
   $(cc) $(cflags) ..\Classlib\CRhsSocket\CRhsSocket.cpp -FoCRhsSocket.obj
