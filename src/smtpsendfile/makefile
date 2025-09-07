########################################################################
# Makefile for smtpsendfile
########################################################################

# Project name

projname = smtpsendfile

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -I..\Classlib
lflags   = -subsystem:console
# cflags   = -c -Od -Zi -I..\Classlib
# lflags   = -debug:full -debugtype:cv

# Objects

objs     = $(projname).obj CRhsSocket.obj

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

CRhsSocket.obj: ..\Classlib\CRhsSocket\CRhsSocket.cpp
   $(cc) $(cflags) ..\Classlib\CRhsSocket\CRhsSocket.cpp -FoCRhsSocket.obj
