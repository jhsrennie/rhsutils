########################################################################
# Makefile for regshowacl
########################################################################

# Project name

projname = regshowacl

# Tools

cc       = cl
link     = link

# Flags

cflags   = -c -W3 -Ox -I..\Classlib -DUNICODE -D_UNICODE
lflags   = -subsystem:console
#cflags   = -c -W3 -Od -Zi -fsanitize=address -I..\Classlib -DUNICODE -D_UNICODE
#lflags   = -subsystem:console -debug:full

# Objects

objs     = $(projname).obj CRegSecObject.obj CSecurableObject.obj \
           CRhsIO.obj CRhsFont.obj CRhsRegistry.obj

# Libraries

libs     = kernel32.lib user32.lib gdi32.lib comdlg32.lib \
           shell32.lib advapi32.lib comctl32.lib shlwapi.lib

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

CRegSecObject.obj: ..\Classlib\CSecurableObject\CRegSecObject.cpp
   $(cc) $(cflags) ..\Classlib\CSecurableObject\CRegSecObject.cpp -FoCRegSecObject.obj

CSecurableObject.obj: ..\Classlib\CSecurableObject\CSecurableObject.cpp
   $(cc) $(cflags) ..\Classlib\CSecurableObject\CSecurableObject.cpp -FoCSecurableObject.obj

CRhsIO.obj: ..\Classlib\CRhsIO\CRhsIO.cpp
   $(cc) $(cflags) ..\Classlib\CRhsIO\CRhsIO.cpp -FoCRhsIO.obj

CRhsFont.obj: ..\Classlib\Misc\CRhsFont.cpp
   $(cc) $(cflags) ..\Classlib\Misc\CRhsFont.cpp -FoCRhsFont.obj

CRhsRegistry.obj: ..\Classlib\Misc\CRhsRegistry.cpp
   $(cc) $(cflags) ..\Classlib\Misc\CRhsRegistry.cpp -FoCRhsRegistry.obj
