// *********************************************************************
// dirspace
// ========
// Listing of subdirectories including the total disk space they use.
//
// John Rennie
// 03/05/04
// *********************************************************************

#include <windows.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <Misc/CRhsFindFile.h>
#include <Misc/CRhsDate.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);

double GetDirDiskSpace(const WCHAR* DirName, FILETIME* CutOff, BOOL Modified, DWORD* NumFiles);

BOOL SetBackupPrivilege(BOOL NotifyErrors);
BOOL UnsetBackupPrivilege(BOOL NotifyErrors);
int strcmpw(const WCHAR* Pattern, const WCHAR* Source);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;


//**********************************************************************
// Global variables
//**********************************************************************

#define SYNTAX \
L"dirspace v2.0.1\r\n" \
L"Syntax: [-i<wildcards> -m<Cutoff date>/-c<Cutoff date>] [<source dir>] \r\n" \
L"Flags:\r\n" \
L"  -i<wildcards>   include only files matching the wildcards. Separate\r\n" \
L"                  multiple wildcards with , or ;\r\n" \
L"  -m<cutoff date> include only files modified after this date\r\n" \
L"  -c<cutoff date> include only files created after this date\r\n"

#define MAX_FILELEN 4096

#define MAX_WILDCARDS 32
int   g_NumWildcards = 0;
WCHAR* g_Wildcards[32];
WCHAR  g_WildcardBuf[256];


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"dirspace"))
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
{ int numarg, i, j;
  double subdir_space, total_space;
  DWORD num_files, num_sub_files;
  BOOL modify_date;
  SYSTEMTIME st;
  FILETIME ft;
  CRhsDate dt;
  CRhsFindFile ff;
  WCHAR rootdir[MAX_FILELEN+1], subdir[MAX_FILELEN+1], s[MAX_FILELEN+1];

  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;

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
    {

// -i specifies files to include in the count

      case 'i':
      case 'I':
        if (lstrlen(RhsIO.m_argv[numarg]+2) > 255)
        { RhsIO.errprintf(L"The mail address wildcard string is longer than the maximum allowed length of 255 characters.\r\n");
          return 2;
        }
        if (lstrlen(RhsIO.m_argv[numarg]+2) == 0)
        { RhsIO.errprintf(L"The search wildcards need to follow the -i flag.\r\n");
          return 2;
        }

        for (i = 2, j = 0; RhsIO.m_argv[numarg][i] != 0; i++)
          if (RhsIO.m_argv[numarg][i] != ' ')
            g_WildcardBuf[j++] = RhsIO.m_argv[numarg][i];
        g_WildcardBuf[j] = '\0';
        CharLower(g_WildcardBuf);

        g_Wildcards[g_NumWildcards++] = g_WildcardBuf;

        for (i = 0; g_WildcardBuf[i] != '\0'; i++)
        { if (g_WildcardBuf[i] == ',' || g_WildcardBuf[i] == ';')
          { g_WildcardBuf[i++] = '\0';
            if (g_WildcardBuf[i] != '\0')
            { if (g_NumWildcards >= MAX_WILDCARDS)
              { RhsIO.errprintf(L"The number of wildcards exceeds the maximum of %i\r\n", MAX_WILDCARDS);
                return 2;
              }

              g_Wildcards[g_NumWildcards++] = g_WildcardBuf + i;
            }
          }
        }
        break;

// -m specifies a modified date. Count only files modified on ar after this date.

      case 'm':
      case 'M':
        modify_date = TRUE;

        if (!dt.SetDate(RhsIO.m_argv[numarg]+2))
        { RhsIO.errprintf(L"The date supplied, \"%s\", is not valid.\r\n", RhsIO.m_argv[numarg]+2);
          return 2;
        }

        dt.GetDate(&st);
        SystemTimeToFileTime(&st, &ft);
        break;

// -c specifies a created date. Count only files created on ar after this date.

      case 'c':
      case 'C':
        modify_date = FALSE;

        if (!dt.SetDate(RhsIO.m_argv[numarg]+2))
        { RhsIO.errprintf(L"The date supplied, \"%s\", is not valid.\r\n", RhsIO.m_argv[numarg]+2);
          return 2;
        }

        dt.GetDate(&st);
        SystemTimeToFileTime(&st, &ft);
        break;

// Unknown flag

      default:
        RhsIO.errprintf(L"Unknown flag: %s\r\n", RhsIO.m_argv[numarg]);
        return 2;
    }
  }

// There should be at most one argument left

  if (RhsIO.m_argc - numarg > 1)
  { RhsIO.errprintf(SYNTAX);
    return 2;
  }

// If no arguments are supplied use the current directory

  if (RhsIO.m_argc - numarg == 0)
  { GetCurrentDirectory(MAX_FILELEN, rootdir);
  }

// Else use the directory supplied as the first argument

  else
  { lstrcpyn(rootdir, RhsIO.m_argv[numarg], MAX_FILELEN);
  }

// Set the backup privilege so we can count files we can't open

  SetBackupPrivilege(true);

// We've got everything, so start the directory listing

  RhsIO.printf(L"Directory of %s\r\n", rootdir);

  if (g_NumWildcards > 0)
  { RhsIO.printf(L"Files matching: ");
    for (i = 0; i < g_NumWildcards; i++)
      RhsIO.printf(L"%s ", g_Wildcards[i]);
    RhsIO.printf(L"\r\n");
  }

  if (ft.dwLowDateTime != 0)
  { RhsIO.printf(L"Cutoff date is %s\r\n", dt.FormatDate(s, 255, 4));

    if (modify_date)
      RhsIO.printf(L"Files modified on or after %s\r\n", dt.FormatDate(s, 255, 4));
    else
      RhsIO.printf(L"Files created on or after %s\r\n", dt.FormatDate(s, 255, 4));
  }

  lstrcpy(s, rootdir);
  lstrcat(s, L"\\*");

  if (!ff.First(s, subdir, MAX_FILELEN))
  { RhsIO.errprintf(L"Cannot find the directory \"%s\"\r\n", rootdir);
    return 2;
  }
  else
  { total_space = 0;
    num_files = 0;

    do
    { if (ff.Attributes() & FILE_ATTRIBUTE_DIRECTORY)
      { if (ff.Attributes() & FILE_ATTRIBUTE_REPARSE_POINT)
        { // Ignore junction points
        }
        else
        { subdir_space = GetDirDiskSpace(ff.FullFilename(s, MAX_FILELEN), &ft, modify_date, &num_sub_files);
          RhsIO.printf(L"%s\t%.0f\t%i\r\n", ff.Filename(subdir, MAX_FILELEN), subdir_space, num_sub_files);

          total_space += subdir_space;
          num_files += num_sub_files;
        }
      }

    } while (ff.Next(subdir, MAX_FILELEN));

    RhsIO.printf(L"Total\t%.0f\tMb\r\n", total_space);
  }

  ff.Close();

// Turn the backup privilege off again

  UnsetBackupPrivilege(true);

// All done

  return 0;
}


//**********************************************************************
// GetDirDiskSpace
// ---------------
//**********************************************************************

double GetDirDiskSpace(const WCHAR* DirName, FILETIME* CutOff, BOOL Modified, DWORD* NumFiles)
{ int i;
  double disk_space, file_size;
  DWORD num_files, num_sub_files;
  DWORD low, high;
  FILETIME ft;
  CRhsFindFile ff;
  WCHAR filename[MAX_FILELEN+1], s[MAX_FILELEN+1];

  disk_space = 0;
  num_files = 0;

// Find all files in the specified directory

  lstrcpy(s, DirName);
  lstrcat(s, L"\\*");

  if (ff.First(s, filename, MAX_FILELEN))
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

// Recurse into the directory/juntion point

        disk_space += GetDirDiskSpace(ff.FullFilename(filename, MAX_FILELEN), CutOff, Modified, &num_sub_files);
        num_files += num_sub_files;
      }

// Otherwise we've found a file

      else
      {

// If we are using a wildcard check to see if the filename matches any
// of the wildcards. If it doesn't skip past this file.

        if (g_NumWildcards > 0)
        { for (i = 0; i < g_NumWildcards; i++)
          { ff.Filename(filename, MAX_FILELEN);
            CharLower(filename);
            if (strcmpw(g_Wildcards[i], filename) == 0)
              break;
          }
          if (i == g_NumWildcards)
            continue;
        }

// Get the file size

        ff.FileSize(&low, &high);
        file_size = ((double) high)*4294967296.0 + (double) low;
        file_size /= 1048576.0;

// If we are not using a date cutoff add the file size immediately

        if (CutOff->dwLowDateTime == 0)
        { disk_space += file_size;
          num_files++;
        }

// If we are using a date cutoff compare the file timestamp with the
// date cutoff.

        else
        { if (Modified)
            ft = ff.LastWriteTime();
          else
            ft = ff.CreationTime();

          if (CompareFileTime(CutOff, &ft) == -1)
          { disk_space += file_size;
            num_files++;
          }
        }
      }

// Next file

    } while (ff.Next(filename, MAX_FILELEN));
  }

// Search has finished so close it

  ff.Close();

// The number of files found (including recursed directories) is
// written into the NumFiles argument and the files size is returned
// as the return value of the function.

  *NumFiles = num_files;

  return disk_space;
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
      RhsIO.errprintf(L"Cannot open the process token.  You may not have the authority to set the Backup privilege.");
    return(FALSE);
  }

  LookupPrivilegeValue(NULL, L"SeBackupPrivilege", &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0))
  { if (NotifyErrors)
      RhsIO.errprintf(L"Cannot adjust privileges.  You may not have the authority to set the Backup privilege.");
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
      RhsIO.errprintf(L"Cannot open the process token.  You may not have the authority to set the Backup privilege.");
    return(FALSE);
  }

  LookupPrivilegeValue(NULL, L"SeBackupPrivilege", &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = 0;
  AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0);

  return(TRUE);
}


//**********************************************************************
// strcmpw
// =======
// Just like strcmp except it handles wildcards * and ?
//**********************************************************************

int strcmpw(const WCHAR* Pattern, const WCHAR* Source)
{ int i;
  const WCHAR *patptr, *strptr;

  patptr = Pattern;
  strptr = Source;

  while (*patptr != '\0' || *strptr != '\0')
  {
    if (*patptr == '*')
    { while (*patptr == '*')
        patptr++;

      if (*patptr == '\0')
        return(0);

      while ((i = strcmpw(patptr, strptr)) != 0)
        if (*strptr++ == '\0')
          return(i);
    }

    else if (*patptr == '?')
    {
    }

    else
    { if (*patptr > *strptr)
        return(1);

      if (*patptr < *strptr)
        return(-1);
    }

    ++patptr;
    ++strptr;
  }

  return(0);
}
