########################################################################
# Makefile for filecomp
########################################################################

# Project name

projname = filecomp

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -D_X86_=1 -I..\Classlib -DWIN32 -DUNICODE -D_UNICODE
lflags   = -subsystem:console
# -entry:WinMainCRTStartup
# cflags   = -c -W3 -Od -Zi -D_X86_=1 -DWIN32 -DUNICODE -D_UNICODE
# lflags   = -debug:full -debugtype:cv

# Objects

objs     = $(projname).obj CRhsFindFile.obj CRhsWildCard.obj Utils.obj \
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

$(projname).res: $(projname).rc ..\Classlib\CRhsIO\CRhsIOWnd.rc ..\Classlib\CRhsIO\CRhsIOWnd.h
    rc -r -I..\Classlib -DWIN32 $(projname).rc

CRhsFindFile.obj: ..\Classlib\Misc\CRhsFindFile.cpp
   $(cc) $(cflags) ..\Classlib\Misc\CRhsFindFile.cpp -FoCRhsFindFile.obj

CRhsWildCard.obj: ..\Classlib\Misc\CRhsWildCard.cpp
   $(cc) $(cflags) ..\Classlib\Misc\CRhsWildCard.cpp -FoCRhsWildCard.obj

Utils.obj: ..\Classlib\Misc\Utils.cpp
   $(cc) $(cflags) ..\Classlib\Misc\Utils.cpp -FoUtils.obj

CRhsIO.obj: ..\Classlib\CRhsIO\CRhsIO.cpp
   $(cc) $(cflags) ..\Classlib\CRhsIO\CRhsIO.cpp -FoCRhsIO.obj

CRhsFont.obj: ..\Classlib\Misc\CRhsFont.cpp
   $(cc) $(cflags) ..\Classlib\Misc\CRhsFont.cpp -FoCRhsFont.obj

CRhsRegistry.obj: ..\Classlib\Misc\CRhsRegistry.cpp
   $(cc) $(cflags) ..\Classlib\Misc\CRhsRegistry.cpp -FoCRhsRegistry.obj

