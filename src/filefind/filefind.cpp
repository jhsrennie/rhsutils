// *********************************************************************
// findfile
// ========
// Find a file or directory
//
// John Rennie
// 03/05/04
// *********************************************************************

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <Misc/CRhsFindFile.h>
#include <Misc/CRhsDate.h>
#include <Misc/Utils.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);
BOOL FindFileSub(WCHAR* Filename);

BOOL SetBackupPrivilege(BOOL NotifyErrors);
BOOL UnsetBackupPrivilege(BOOL NotifyErrors);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;

FILETIME g_SearchDate;
BOOL     g_UseSearchDate;

int g_NumFiles;

#define SYNTAX \
L"filefind v1.0\r\n" \
L"Syntax: [-m<cutoff date>] <file name>\r\n" \
L"  -m<cutoff date> include only files modified after this date\r\n"


//**********************************************************************
// main
// ----
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"filefind"))
  { wprintf(L"Error initialising: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  retcode = RhsIO.Run(rhsmain);

  if (retcode != 0)
  { RhsIO.errprintf(L"Error: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  return retcode;
}


//**********************************************************************
// Here we go
// ----------
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused)
{ int numarg;
  CRhsDate dt;
  SYSTEMTIME st;

  WCHAR target[LEN_FILENAME];

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// Process flags

  g_UseSearchDate = FALSE;

  for (numarg = 1; numarg < RhsIO.m_argc; numarg++)
  { if (RhsIO.m_argv[numarg][0] != '-')
      break;

    switch (RhsIO.m_argv[numarg][1])
    { case 'm':
      case 'M':
        g_UseSearchDate = TRUE;

        if (!dt.SetDate(RhsIO.m_argv[numarg]+2))
        { RhsIO.SetLastError(L"The date supplied, \"%s\", is not valid.\r\n", RhsIO.m_argv[numarg]+2);
          return 2;
        }

        dt.GetDate(&st);
        SystemTimeToFileTime(&st, &g_SearchDate);
        break;

      case '?':
        RhsIO.printf(SYNTAX);
        return 0;

      default:
        RhsIO.SetLastError(L"Unknown flag: %s\r\n", RhsIO.m_argv[numarg]);
        return 2;
    }
  }

  if (RhsIO.m_argc - numarg != 1)
  { RhsIO.SetLastError(SYNTAX);
    return 2;
  }

// Process the arguments

  lstrcpy(target, RhsIO.m_argv[numarg]);
  UtilsStripTrailingSlash(target);
  UtilsAddCurPath(target, LEN_FILENAME);

// Check the path is valid

  if (!UtilsIsValidPath(target))
  { RhsIO.SetLastError(L"Invalid directory: %s\r\n", target);
    return 2;
  }

// Start searching

  g_NumFiles = 0;

  RhsIO.printf(L"Searching for: %s\r\n", target);

  SetBackupPrivilege(FALSE);
  FindFileSub(target);
  UnsetBackupPrivilege(FALSE);

  RhsIO.printf(L"%i files found\r\n", g_NumFiles);

/// All done so return indicating success

  return 0;
}


//**********************************************************************
// FindFileSub
// -----------
//**********************************************************************

BOOL FindFileSub(WCHAR* Filename)
{ BOOL heading = FALSE;
  CRhsFindFile ff;
  FILETIME ft;
  WCHAR filename[LEN_FILENAME+1], s[LEN_FILENAME];

// First find all subdirectories and recurse into them

  UtilsGetPathFromFilename(Filename, s, LEN_FILENAME);
  lstrcat(s, L"\\*");

  if (ff.First(s, filename, LEN_FILENAME))
  { do
    {

// If we've found a directory recurse into it

      if (ff.Attributes() & FILE_ATTRIBUTE_DIRECTORY)
      {

// If this "directory" is a junction point ignore it

        if (ff.Attributes() & FILE_ATTRIBUTE_REPARSE_POINT)
          continue;

// Ignore . and ..

        if (lstrcmp(filename, L".") == 0 || lstrcmp(filename, L"..") == 0)
          continue;

// Append the target file name to the directory we've found and recurse
// into it.

        ff.FullFilename(filename, LEN_FILENAME);
        lstrcat(filename, L"\\");
        UtilsGetNameFromFilename(Filename, filename + lstrlen(filename), LEN_FILENAME - lstrlen(filename));

        if (!FindFileSub(filename))
          return FALSE;
      }

// Find next file

    } while (ff.Next(filename, LEN_FILENAME));
  }

  ff.Close();

// Now we've searched all subdirectories, so search this directory

  if (ff.First(Filename, s, LEN_FILENAME))
  { do
    {

// If we are using a search date check the file timestamp

      if (g_UseSearchDate)
      { ft = ff.LastWriteTime();

        if (CompareFileTime(&g_SearchDate, &ft) >= 0)
          continue;
      }

// If we haven't printed the heading yet do it now

      if (!heading)
      { UtilsGetPathFromFilename(Filename, s, LEN_FILENAME);
        RhsIO.printf(L"Directory: %s\r\n", s);
        heading = TRUE;
      }

/// Print the file details

      UtilsFileInfoFormat(&ff, FALSE, s, LEN_FILENAME);
      RhsIO.printf(L"%s\r\n", s);

      g_NumFiles++;

// Find next file

    } while (ff.Next(s, LEN_FILENAME));
  }

  ff.Close();

// If we've printed anything append a blank line

  if (heading)
  { UtilsGetPathFromFilename(Filename, s, LEN_FILENAME);
    RhsIO.printf(L"\r\n");
  }

// All done so return indicating success

  return TRUE;
}


//*********************************************************************
// SetBackupPrivilege
// ------------------
//*********************************************************************

BOOL SetBackupPrivilege(BOOL NotifyErrors)
{ HANDLE token;
  TOKEN_PRIVILEGES tkp;

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
  { if (NotifyErrors)
      RhsIO.SetLastError(L"Cannot open the process token.  You may not have the authority to set the Backup privilege.");
    return(FALSE);
  }

  LookupPrivilegeValue(NULL, L"SeBackupPrivilege", &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0))
  { if (NotifyErrors)
      RhsIO.SetLastError(L"Cannot adjust privileges.  You may not have the authority to set the Backup privilege.");
    return(FALSE);
  }

  return(TRUE);
}


//*********************************************************************
// UnsetBackupPrivilege
// --------------------
//*********************************************************************

BOOL UnsetBackupPrivilege(BOOL NotifyErrors)
{ HANDLE token;
  TOKEN_PRIVILEGES tkp;

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
  { if (NotifyErrors)
      RhsIO.SetLastError(L"Cannot open the process token.  You may not have the authority to set the Backup privilege.");
    return(FALSE);
  }

  LookupPrivilegeValue(NULL, L"SeBackupPrivilege", &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = 0;
  AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0);

  return(TRUE);
}


