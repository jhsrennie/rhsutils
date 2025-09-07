########################################################################
# Makefile for filefinducase
########################################################################

# Project name

projname = filefinducase

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -I..\RHSUtils\src\Classlib -DUNICODE -D_UNICODE
lflags   = -subsystem:console
#cflags   = -c -W3 -Od -Zi -fsanitize=address -IRHSUtils\src\Classlib -DUNICODE -D_UNICODE
#lflags   = -subsystem:console -debug:full

# Objects

objs     = $(projname).obj CTextBuffer.obj \
           CRhsFindFile.obj CRhsDate.obj Utils.obj \
           CRhsIO.obj CRhsFont.obj CRhsRegistry.obj

# Libraries

libs     = kernel32.lib user32.lib gdi32.lib \
           comdlg32.lib shell32.lib advapi32.lib comctl32.lib

# Inference rules

.cpp.obj:
   $(cc) $(cflags) $*.cpp -Fo$*.obj

# Targets

all: $(projname).exe

$(projname).exe: $(objs) $(projname).res
    $(link) $(lflags) -out:$(projname).exe $(objs) $(projname).res $(libs)
    copy /Y $(projname).exe ..\..\bin

$(projname).res: $(projname).rc ..\RHSUtils\src\Classlib\CRhsIO\CRhsIOWnd.rc ..\RHSUtils\src\Classlib\CRhsIO\CRhsIOWnd.h
    rc -r -I..\RHSUtils\src\Classlib -DWIN32 $(projname).rc

CRhsFindFile.obj: ..\RHSUtils\src\Classlib\Misc\CRhsFindFile.cpp
   $(cc) $(cflags) ..\RHSUtils\src\Classlib\Misc\CRhsFindFile.cpp -FoCRhsFindFile.obj

CRhsDate.obj: ..\RHSUtils\src\Classlib\Misc\CRhsDate.cpp
   $(cc) $(cflags) ..\RHSUtils\src\Classlib\Misc\CRhsDate.cpp -FoCRhsDate.obj

Utils.obj: ..\RHSUtils\src\Classlib\Misc\Utils.cpp
   $(cc) $(cflags) ..\RHSUtils\src\Classlib\Misc\Utils.cpp -FoUtils.obj

CRhsIO.obj: ..\RHSUtils\src\Classlib\CRhsIO\CRhsIO.cpp
   $(cc) $(cflags) ..\RHSUtils\src\Classlib\CRhsIO\CRhsIO.cpp -FoCRhsIO.obj

CRhsFont.obj: ..\RHSUtils\src\Classlib\Misc\CRhsFont.cpp
   $(cc) $(cflags) ..\RHSUtils\src\Classlib\Misc\CRhsFont.cpp -FoCRhsFont.obj

CRhsRegistry.obj: ..\RHSUtils\src\Classlib\Misc\CRhsRegistry.cpp
   $(cc) $(cflags) ..\RHSUtils\src\Classlib\Misc\CRhsRegistry.cpp -FoCRhsRegistry.obj

