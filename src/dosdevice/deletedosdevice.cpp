// *********************************************************************
// deletedosdevice
// ===============
//
// John Rennie
// 17/05/2013
// *********************************************************************

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);
const WCHAR* GetLastErrorMessage(void);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;

#define SYNTAX \
L"deletedosdevice v1.0\r\n" \
L"Syntax: <device name>\r\n"


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"deletedosdevice"))
  { wprintf(L"Error initialising: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  retcode = RhsIO.Run(rhsmain);

  if (retcode != 0)
    RhsIO.printf(L"Error: %s\r\n", RhsIO.LastError(lasterror, 256));

  return retcode;
}


//**********************************************************************
// Here we go
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused)
{

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// There must be exactly one argument

  if (RhsIO.m_argc != 2)
  { RhsIO.SetLastError(SYNTAX);
    return 2;
  }

// Delete the device

  if (DefineDosDevice(DDD_REMOVE_DEFINITION, RhsIO.m_argv[1], NULL) == 0)
  { RhsIO.SetLastError(GetLastErrorMessage());
    return 2;
  }

  RhsIO.printf(L"Deleted %s\n", RhsIO.m_argv[1]);

// All done

  return 0;
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
//**********************************************************************

const WCHAR* GetLastErrorMessage(void)
{ DWORD d;
  static WCHAR errmsg[1024];

  d = GetLastError();

  lstrcpy(errmsg, L"<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 1023, NULL);
  return(errmsg);
}


