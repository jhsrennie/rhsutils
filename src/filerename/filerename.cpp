// *********************************************************************
// filerename
// ==========
// Rename file(s)
//
// John Rennie
// 12/10/09
// *********************************************************************

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <Misc/CRhsFindFile.h>
#include <Misc/CRhsWildCard.h>
#include <Misc/Utils.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);
BOOL FileRenameSub(WCHAR* Filename);

BOOL SetBackupPrivilege(BOOL NotifyErrors);
BOOL UnsetBackupPrivilege(BOOL NotifyErrors);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;

WCHAR g_Search[LEN_FILENAME+1], g_Replace[LEN_FILENAME+1];
int g_NumFiles;

BOOL g_Recurse, g_Hidden, g_Quiet, g_Report, g_System;

CRhsWildCard g_WildCard;

#define SYNTAX \
L"filerename v1.0\r\n" \
L"Syntax: [-d -h -q -r -s] <old file name> <new file name>\r\n" \
L"Flags:\r\n" \
L"  -d recurse into subdirectories\r\n" \
L"  -h include hidden files\r\n" \
L"  -q don't list files renamed\r\n" \
L"  -r report mode\r\n" \
L"  -s include system files\r\n"


//**********************************************************************
// main
// ----
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"filerename"))
  { wprintf(L"Error initialising: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  retcode = RhsIO.Run(rhsmain);

  if (retcode != 0)
  { wprintf(L"Error: %s\n", RhsIO.LastError(lasterror, 256));
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
  WCHAR startdir[LEN_FILENAME+1], s[LEN_FILENAME+1];

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// Process flags

  g_Recurse = g_Hidden = g_Quiet = g_Report = g_System = FALSE;

  for (numarg = 1; numarg < RhsIO.m_argc; numarg++)
  { if (RhsIO.m_argv[numarg][0] != '-')
      break;

    switch (RhsIO.m_argv[numarg][1])
    { case 'd':
      case 'D':
        g_Recurse = TRUE;
        break;

      case 'h':
      case 'H':
        g_Hidden = TRUE;
        break;

      case 'q':
      case 'Q':
        g_Quiet = TRUE;
        break;

      case 'r':
      case 'R':
        g_Report = TRUE;
        break;

      case 's':
      case 'S':
        g_System = TRUE;
        break;

      case '?':
        RhsIO.printf(SYNTAX);
        return 0;

      default:
        RhsIO.errprintf(L"Unknown flag: %s\r\n", RhsIO.m_argv[numarg]);
        return 2;
    }
  }

  if (RhsIO.m_argc - numarg != 2)
  { RhsIO.errprintf(SYNTAX);
    return 2;
  }

// Process the arguments
// Make sure the first argument has a full path. Split off the path
// and save the file name a a global.

  lstrcpy(s, RhsIO.m_argv[numarg]);
  UtilsStripTrailingSlash(s);
  UtilsAddCurPath(s, LEN_FILENAME);
  UtilsGetPathFromFilename(s, startdir, LEN_FILENAME);
  UtilsGetNameFromFilename(s, g_Search, LEN_FILENAME);

  if (!UtilsIsValidPath(startdir))
    return 2;

// The replace spec must not contain a path

  lstrcpy(g_Replace, RhsIO.m_argv[numarg+1]);

  if (wcschr(g_Replace, '\\'))
  { RhsIO.errprintf(L"The new name must not contain a path.\r\n");
    return 2;
  }

// Start searching

  g_NumFiles = 0;

  if (!g_Quiet)
    RhsIO.printf(L"Renaming: %s\r\n" \
                 L"to:       %s\r\n" \
                 L"in dir:   %s\r\n\r\n",
                 g_Search, g_Replace, startdir);

  FileRenameSub(startdir);

  if (!g_Quiet)
    RhsIO.printf(L"%i files renamed\r\n", g_NumFiles);

/// All done so return indicating success

  return 0;
}


//**********************************************************************
// FileRenameSub
// -----------
//**********************************************************************

BOOL FileRenameSub(WCHAR* StartDir)
{ BOOL heading;
  CRhsFindFile ff;
  WCHAR filename[LEN_FILENAME+1], newfilename[LEN_FILENAME+1], s[LEN_FILENAME];

// If we are recursing into subdirectories do the recursion now

  if (g_Recurse)
  { lstrcpy(s, StartDir);
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
          if (!FileRenameSub(filename))
            return FALSE;
        }

// Find next file

      } while (ff.Next(filename, LEN_FILENAME));
    }

    ff.Close();
  }

// Now we've searched all subdirectories, so search this directory

  heading = FALSE;

  lstrcpy(s, StartDir);
  lstrcat(s, L"\\");
  lstrcat(s, g_Search);

  if (ff.First(s, filename, LEN_FILENAME))
  { do
    {

// Check the file attributes

      if (ff.Attributes() & FILE_ATTRIBUTE_HIDDEN)
        if (!g_Hidden)
          continue;

      if (ff.Attributes() & FILE_ATTRIBUTE_SYSTEM)
        if (!g_System)
          continue;

// Work out the new name

      ff.Filename(filename, LEN_FILENAME);
      if (!g_WildCard.Complete(g_Search, filename, g_Replace, s, LEN_FILENAME))
      { RhsIO.printf(L"%s: %s\r\n", filename, g_WildCard.LastError());
        ff.Close();
        return FALSE;
      }

// If we haven't printed the heading yet do it now

      if (!g_Quiet)
      { if (!heading)
        { RhsIO.printf(L"Directory: %s\r\n", StartDir);
          heading = TRUE;
        }

        RhsIO.printf(L"%s -> %s\r\n", filename, s);
      }

// Rename the file

      if (!g_Report)
      { lstrcpy(newfilename, StartDir);
        lstrcat(newfilename, L"\\");
        lstrcat(newfilename, s);

        ff.FullFilename(filename, LEN_FILENAME);

        if (!MoveFile(filename, newfilename))
        { RhsIO.printf(L"%s\r\n", GetLastErrorMessage());
          ff.Close();
          return FALSE;
        }
      }

      g_NumFiles++;

// Find next file

    } while (ff.Next(s, LEN_FILENAME));
  }

  ff.Close();

// If we've printed anything append a blank line

  if (heading)
    RhsIO.printf(L"\r\n");

// All done so return indicating success

  return TRUE;
}


