// *********************************************************************
// definedosdevice
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
L"definedosdevice v1.0\r\n" \
L"Syntax: <device name> <device path>\r\n"


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"definedosdevice"))
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
{ WCHAR targetname[1024];

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// There must be exactly two arguments

  if (RhsIO.m_argc != 3)
  { RhsIO.SetLastError(SYNTAX);
    return 2;
  }

// Define the device

  if (DefineDosDevice(DDD_RAW_TARGET_PATH, RhsIO.m_argv[1], RhsIO.m_argv[2]) == 0)
  { RhsIO.SetLastError(GetLastErrorMessage());
    return 2;
  }

// Query the device to check it

  if (QueryDosDevice(RhsIO.m_argv[1], targetname, 1024) == 0)
  { RhsIO.SetLastError(GetLastErrorMessage());
    return 2;
  }

  RhsIO.printf(L"%s\t%s\n", RhsIO.m_argv[1], targetname);

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


