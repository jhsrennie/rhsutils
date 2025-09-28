########################################################################
# Makefile for HttpServer
########################################################################

# Project name

projname = httpserver

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -I..\Classlib
lflags   = -subsystem:console
#cflags   = -c -W3 -Od -Zi -fsanitize=address -I..\Classlib
#lflags   = -subsystem:console -debug:full

# Objects

objs     = $(projname).obj CRhsSocket.obj

# Libraries

libs     = kernel32.lib user32.lib gdi32.lib \
           comdlg32.lib shell32.lib advapi32.lib comctl32.lib \
           wsock32.lib

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

