// *********************************************************************
// example
// ========
// Listing of subdirectories including the total disk space they use.
//
// John Rennie
// 03/05/04
// *********************************************************************

#include <windows.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;


//**********************************************************************
// Global variables
//**********************************************************************

#define SYNTAX \
L"example v1.0\r\n" \
L"Syntax: [-a -b -c] \r\n" \
L"Flags:\r\n" \
L"  -a etc  example of how to proxcess flags.\r\n"


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"example"))
  { wprintf(L"Error initialising: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  retcode = RhsIO.Run(rhsmain);

  return retcode;
}


//**********************************************************************
// Here we go
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused)
{ int numarg;

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// Process arguments

  for (numarg = 1; numarg < RhsIO.m_argc; numarg++)
  { if (RhsIO.m_argv[numarg][0] != '-')
      break;

    switch (RhsIO.m_argv[numarg][1])
    { case 'a':
        RhsIO.printf(L"Option: -a\r\n");
        break;

      case 'b':
        RhsIO.printf(L"Option: -b\r\n");
        break;

      case 'c':
        RhsIO.printf(L"Option: -c\r\n");
        break;

      default:
        RhsIO.errprintf(L"Unknown flag: %s\r\n", RhsIO.m_argv[numarg]);
        return 2;
    }
  }

// Check the number of arguments left.
// In this example the number should be zero.

  if (RhsIO.m_argc - numarg != 0)
  { RhsIO.errprintf(SYNTAX);
    return 2;
  }

// Ready to go

  RhsIO.printf(L"Hello world\r\n");

// All done

  return 0;
}


