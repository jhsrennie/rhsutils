// *********************************************************************
// querydosdevice
// ==============
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
L"querydosdevice v1.0\r\n" \
L"Syntax: [<device name>]\r\n"


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"querydosdevice"))
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

// Start at 8KB for the device names buffer. If this is too small it's
// dynamically expanded by up to 256 times.
#define LEN_DEVNAMEBUF 0x2000

DWORD WINAPI rhsmain(LPVOID unused)
{ DWORD targetsize, devnamesize, i;
  LPCTSTR devicename;
  WCHAR* devnamebuf;
  WCHAR* devnameptr;
  WCHAR targetname[1024];

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// There must be at one or two arguments

  if (RhsIO.m_argc < 1 || RhsIO.m_argc > 2)
  { RhsIO.SetLastError(SYNTAX);
    return 2;
  }

  if (RhsIO.m_argc == 2)
    devicename = RhsIO.m_argv[1];
  else
    devicename = NULL;

// If we have a device name look it up now

  if (devicename)
  { targetsize = QueryDosDevice(devicename, targetname, 1024);

    if (targetsize == 0)
    { RhsIO.SetLastError(GetLastErrorMessage());
      return 2;
    }

    RhsIO.printf(L"%s\n", targetname);

    return 0;
  }

// If no device name was specified get a list of all device names.

  devnamesize = LEN_DEVNAMEBUF;

  for (i = 0; i < 8; i++)
  { devnamebuf = new WCHAR[devnamesize];

    targetsize = QueryDosDevice(NULL, devnamebuf, devnamesize-2);
    if (targetsize != 0)
      break;

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    { RhsIO.SetLastError(GetLastErrorMessage());
      return 2;
    }

    delete devnamebuf;
    devnamesize = devnamesize*2;
  }

  if (targetsize == 0)
  { RhsIO.SetLastError(GetLastErrorMessage());
    return 2;
  }

// Loop through the targets and list all the names

  for (devnameptr = devnamebuf; *devnameptr; devnameptr += lstrlen(devnameptr) + 1)
  { targetsize = QueryDosDevice(devnameptr, targetname, 1024);

    if (targetsize == 0)
    { RhsIO.printf(L"%s\tError: %s\r\n", devnameptr, GetLastErrorMessage());
    }
    else
    { RhsIO.printf(L"%s\t%s\n", devnameptr, targetname);
    }
  }

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


