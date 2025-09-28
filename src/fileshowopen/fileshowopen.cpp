//**********************************************************************
// ListOpenFiles
// =============
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 17th April 1999
//**********************************************************************

#include <windows.h>
#include <stdio.h>
#include <lm.h>
#include <CRhsIO/CRhsIO.h>


//**********************************************************************
// Function prototypes
// -------------------
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);
const WCHAR* GetLastErrorMessage(DWORD dwLastError);


//**********************************************************************
// Global variables
// ----------------
//**********************************************************************

CRhsIO RhsIO;

#define SYNTAX \
  L"listopenfiles [<server> [<path> [<user> [<close|closeread|closeall>]]]]\r\n"


//**********************************************************************
// main
// ----
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"fileshowopen"))
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
{ DWORD entries_read, total_entries, files_found, files_closed;
  DWORD_PTR resume_handle;
  BOOL b_path, b_server, b_user, b_close, b_forcecloseread, b_forcecloseall, b_header;
  FILE_INFO_3* fileinfo;
  WCHAR server[128], path[MAX_PATH+1], user[128], s[256];

// Process the command line arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// First argument is the server name

  b_server = FALSE;
  if (RhsIO.m_argc > 1)
  { if (lstrcmp(RhsIO.m_argv[1], L".") != 0)
    { lstrcpy(server, RhsIO.m_argv[1]);
      b_server = TRUE;
    }
  }

// Second argument is the filename

  b_path = FALSE;
  if (RhsIO.m_argc > 2)
  { if (lstrcmp(RhsIO.m_argv[2], L".") != 0)
    { lstrcpy(path, RhsIO.m_argv[2]);
      b_path = TRUE;
    }
  }

// Third argument is the username

  b_user = FALSE;
  if (RhsIO.m_argc > 3)
  { if (lstrcmp(RhsIO.m_argv[3], L".") != 0)
    { lstrcpy(user, RhsIO.m_argv[3]);
      b_user = TRUE;
    }
  }

// Fourth and last argument is the "close file" option

  b_close = b_forcecloseread = b_forcecloseall = FALSE;
  if (RhsIO.m_argc > 4)
  { if (lstrcmp(RhsIO.m_argv[4], L"close") == 0)
    { b_close = TRUE;
    }
    else if (lstrcmp(RhsIO.m_argv[4], L"closeread") == 0)
    { b_forcecloseread = TRUE;
    }
    else if (lstrcmp(RhsIO.m_argv[4], L"closeall") == 0)
    { b_forcecloseall = TRUE;
    }
    else
    { RhsIO.errprintf(L"Unrecognised 4th argument \"%s\".  Options are \"close\", \"closeread\" or \"closeall\".\r\n", RhsIO.m_argv[4]);
      return 2;
    }

// The "close" option isn't supported if this is a GUI app

    if (b_close && !RhsIO.IsConsole())
    { RhsIO.errprintf(L"The \"close\" option can only be used when running fileshowopen\r\nfrom the command prompt.\r\n");
      return 2;
    }
  }

// Print a banner

  if (b_close)
    lstrcpy(s, L"Close");
  else if (b_forcecloseread)
    lstrcpy(s, L"Close read");
  else if (b_forcecloseall)
    lstrcpy(s, L"Close all");
  else
    lstrcpy(s, L"None");

  RhsIO.printf(L"Files on \"%s\", path \"%s\", user \"%s\", close is \"%s\"\r\n",
         b_server ? server : L"Local server",
         b_path ? path : L"All files",
         b_user ? user : L"All users",
         s
        );

  b_header = FALSE; // Don't print a column header until a file is found

// Now list the open files

  resume_handle = NULL;
  files_found = files_closed = 0;

  for(;;)
  {

// Get the next batch of files

    if (NetFileEnum(b_server ? server : NULL,
                    b_path ? path : NULL,
                    b_user ? user : NULL,
                    3,
                    (BYTE**) &fileinfo,
                    0x10000,
                    &entries_read,	
                    &total_entries,	
                    &resume_handle
                   ) != NERR_Success)
    {
      RhsIO.errprintf(L"Cannot list files: %s\r\n", GetLastErrorMessage(GetLastError()));
      break;
    }

// If there are no files to get then exit now

    if (total_entries == 0)
      break;

// If for some reason we did not retrieve any files then loop round to try again

    if (entries_read == 0)
      continue;

// Not quite sure why, but if there is just one entry for \PIPE\srvsvc then
// this indicates the end of the search

    if (entries_read == 1)
    { lstrcpyn(path, fileinfo[0].fi3_pathname, MAX_PATH);
      if (lstrcmp(path, L"\\PIPE\\srvsvc") == 0)
        break;
    }

// Print the details of the files we found

    for (DWORD d = 0; d < entries_read; d++)
    { files_found++;

      lstrcpyn(path, fileinfo[d].fi3_pathname, MAX_PATH);
      lstrcpyn(user, fileinfo[d].fi3_username, 127);

      if (lstrcmp(path, L"\\PIPE\\srvsvc") == 0)
        continue;

      lstrcpy(s, L"    ");
      if (fileinfo[d].fi3_permissions & PERM_FILE_READ)
        s[0] = 'R';
      if (fileinfo[d].fi3_permissions & PERM_FILE_WRITE)
        s[1] = 'W';
      if (fileinfo[d].fi3_permissions & 8) // 8 is exec I think
        s[2] = 'E';
      if (fileinfo[d].fi3_permissions & PERM_FILE_CREATE)
        s[3] = 'C';

      if (!b_header)
      { RhsIO.printf(L"\r\nUser          Type  Locks  Filename\r\n");
        b_header = TRUE;
      }

      RhsIO.printf(L"%-12s  %-4s  %4i   %s", user, s, fileinfo[d].fi3_num_locks, path);

// Do we want to close the file?

      if (b_forcecloseall)
      { if (NetFileClose((LPTSTR) (b_server ? server : NULL), fileinfo[d].fi3_id) == NERR_Success)
        { RhsIO.printf(L": Closed");
          files_closed++;
        }
        else
        { RhsIO.errprintf(L"\r\nCannot close %s: %s\r\n", path, GetLastErrorMessage(GetLastError()));
        }
      }

      else if (b_forcecloseread)
      { if (!(fileinfo[d].fi3_permissions & PERM_FILE_WRITE))
        { if (NetFileClose((LPTSTR) (b_server ? server : NULL), fileinfo[d].fi3_id) == NERR_Success)
          { RhsIO.printf(L": Closed");
            files_closed++;
          }
          else
          { RhsIO.errprintf(L"\r\nCannot close %s: %s\r\n", path, GetLastErrorMessage(GetLastError()));
          }
        }
      }

      else if (b_close)
      { RhsIO.printf(L": Close? (Y/N): ");
        _getws_s(s, 255);
        CharUpper(s);

        if (s[0] == 'Y')
        { if (NetFileClose((LPTSTR) (b_server ? server : NULL), fileinfo[d].fi3_id) != NERR_Success)
          { RhsIO.printf(L"Closed");
            files_closed++;
          }
          else
          { RhsIO.errprintf(L"\r\nCannot close %s: %s\r\n", path, GetLastErrorMessage(GetLastError()));
          }
        }
      }

// Next line

      RhsIO.printf(L"\r\n");
    }

// Free the buffer returned by NetFileEnum

    NetApiBufferFree(fileinfo);
  }

// Print a summary

  if (files_found == 0)
  { RhsIO.printf(L"\r\nNo open files found\r\n");
  }
  else
  { RhsIO.printf(L"\r\n%i open files", files_found);

    if (files_closed > 0)
      RhsIO.printf(L", %i files closed", files_closed);

    RhsIO.printf(L"\r\n");
  }

// All done

  return 0;
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
//**********************************************************************

const WCHAR* GetLastErrorMessage(DWORD dwLastError)
{ DWORD dwFormatFlags;
  HMODULE hModule = NULL;
  static WCHAR errmsg[512];

  dwFormatFlags = FORMAT_MESSAGE_FROM_SYSTEM;

// If dwLastError is in the network range, load the message source

  if (dwLastError >= NERR_BASE && dwLastError <= MAX_NERR)
  { hModule = LoadLibraryEx(L"netmsg.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModule != NULL)
      dwFormatFlags = FORMAT_MESSAGE_FROM_HMODULE;
  }

// Format the message

  lstrcpy(errmsg, L"<unknown error>");
  FormatMessage(dwFormatFlags, hModule, dwLastError, 0, errmsg, 511, NULL);

// Free any libraries we loaded

  if (hModule != NULL)
    FreeLibrary(hModule);

// Return the error message

  return errmsg;
}


