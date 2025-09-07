// *********************************************************************
// ftpdir
// ======
//
// John Rennie
// 18/06/09
// *********************************************************************

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <Internet/CWinInetFTP.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);

BOOL FtpDir(WCHAR* SearchPath, WCHAR* SearchFile, BOOL Recurse, CWinInetFTP* FTP);
BOOL FtpDirFormat(WIN32_FIND_DATA* Info, WCHAR* Result, int Maxlen);
BOOL notify(WCHAR* s);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;


//**********************************************************************
// Global variables
//**********************************************************************

#define SYNTAX \
L"ftpdir [-d -p<proxy>] <server> [<search expr> [<username> <password>]]\r\n" \
L"e.g.\r\n" \
L"ftpdir -d ftp.microsoft.com /*.zip\r\n" \
L"will list all zip files on the Microsoft ftp site.\r\n"

#define MAX_FILENAME 512


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"ftpdir"))
  { wprintf(L"Error initialising: %s\r\n", RhsIO.LastError(lasterror, 256));
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
  BOOL recurse;
  WCHAR* proxy;
  WCHAR* server;
  WCHAR* username;
  WCHAR* password;
  CWinInetFTP ftp;
  WCHAR search_path[MAX_FILENAME+1], search_file[MAX_FILENAME+1];

// Check enough arguments were supplied

  if (RhsIO.m_argc < 2)
  { RhsIO.printf(SYNTAX);
    return(1);
  }

// Process flags

  proxy = NULL;
  recurse = FALSE;

  for (numarg = 1; numarg < RhsIO.m_argc && RhsIO.m_argv[numarg][0] == '-'; numarg++)
  { switch (RhsIO.m_argv[numarg][1])
    { case 'd':
      case 'D':
        recurse = TRUE;
        break;

      case 'p':
      case 'P':
        proxy = RhsIO.m_argv[numarg] + 2;
        break;

      case '?':
        RhsIO.printf(SYNTAX);
        return(2);

      default:
        RhsIO.printf(L"Unknown flag \"%s\"\r\n", RhsIO.m_argv[numarg]);
        return(2);
    }
  }

// Get server name

  if (RhsIO.m_argc - numarg < 1)
  { RhsIO.printf(SYNTAX);
    return(2);
  }

  server = RhsIO.m_argv[numarg];

// If no name was specified set the path to "/" and the name to "*"

  if (RhsIO.m_argc - numarg < 2)
  { lstrcpy(search_path, L"/");
    lstrcpy(search_file, L"*");
  }

// Else split the search name into the path and filename

  else
  { i = 0;
    if (RhsIO.m_argv[numarg+1][0] != '/')
      search_path[i++] = '/';

    for (j = 0 ; RhsIO.m_argv[numarg+1][j] != '\0'; i++,j++)
      search_path[i] = RhsIO.m_argv[numarg+1][j];
    search_path[i] = '\0';
    while (search_path[i] != '/')
      i--;
    i++;

    lstrcpy(search_file, search_path + i);
    if (lstrlen(search_file) == 0)
      lstrcpy(search_file, L"*");

    search_path[i] = '\0';
  }

// Get the username and password

  username = password = NULL;

  if (RhsIO.m_argc - numarg > 2)
    username = RhsIO.m_argv[numarg+2];

  if (RhsIO.m_argc - numarg > 3)
    password = RhsIO.m_argv[numarg+3];

// Connect to the server

  ftp.SetNotify(notify);
  if (!ftp.Connect(server, username, password, proxy))
    return(2);

// FtpDir the source to destination directory

  FtpDir(search_path, search_file, recurse, &ftp);

// Disconnect from the server

  ftp.Close();

// All done

  return(0);
}


//**********************************************************************
// Function to search for all files matching the first argument.
// If Recurse is true then any sub-directories found will be searched
// into.
//**********************************************************************

BOOL FtpDir(WCHAR* SearchPath, WCHAR* SearchFile, BOOL Recurse, CWinInetFTP* FTP)
{ int numfound, i;
  WIN32_FIND_DATA* info;
  WCHAR subdir[MAX_FILENAME+1], s[MAX_FILENAME+1];

// Use a buffer to hold up to 1024 entries

  info = (WIN32_FIND_DATA*) GlobalAlloc(GMEM_FIXED, 1024*sizeof(WIN32_FIND_DATA));

// Print contents of this directory

  RhsIO.printf(L"Directory of %s%s\r\n", (WCHAR*) SearchPath, SearchFile);

  numfound = FTP->Dir(SearchPath, SearchFile, info, 1024);

  for (i = 0; i < numfound; i++)
  { FtpDirFormat(info + i, s, MAX_FILENAME);
    RhsIO.printf(L"%s\r\n", s);
  }

  if (numfound == 0)
    RhsIO.printf(L"No files found\r\n\r\n");
  else
    RhsIO.printf(L"%i files found\r\n\r\n", numfound);

// If we are not recursing then stop here

  if (!Recurse)
  { GlobalFree(info);
    return(TRUE);
  }

// Now repeat the search, this time searching for all files and then save
// all the directory names.  We have to repeat the search because a search
// expression like /*.zip would not find subdirectories unless their name
// matched "*.zip".

  numfound = FTP->Dir(SearchPath, L"*", info, 1024);

  for (i = 0; i < numfound; i++)
  { if (lstrcmp(info[i].cFileName, L".") == 0 || lstrcmp(info[i].cFileName, L"..") == 0)
      continue;

    if (!(info[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      continue;

    lstrcpy(subdir, SearchPath);
    lstrcat(subdir, info[i].cFileName);
    lstrcat(subdir, L"/");

    if (!FtpDir(subdir, SearchFile, TRUE, FTP))
      break;
  }

// Free the file info buffer

  GlobalFree((HGLOBAL) info);

// Return indicating success

  return(TRUE);
}


//**********************************************************************
// Function to format file info into a string
//**********************************************************************

BOOL FtpDirFormat(WIN32_FIND_DATA* Info, WCHAR* Result, int Maxlen)
{ WCHAR attstr[5];
  SYSTEMTIME st;

/*** Build file attribute string */

  lstrcpy(attstr, L"....");
  if (Info->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)  attstr[0] = 'A';
  if (Info->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)   attstr[1] = 'H';
  if (Info->dwFileAttributes & FILE_ATTRIBUTE_READONLY) attstr[2] = 'R';
  if (Info->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)   attstr[3] = 'S';

/*** And finally print details */

  FileTimeToSystemTime(&(Info->ftLastWriteTime), &st);

  if (Info->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    swprintf(Result, Maxlen,
             L"%-30s    <DIR>  %2i/%02i/%02i  %2i:%02i:%02i  %4s",
             Info->cFileName,
             st.wDay, st.wMonth, st.wYear,
             st.wHour, st.wMinute, st.wSecond,
             attstr
            );
  else
    swprintf(Result, Maxlen,
             L"%-30s %8li  %2i/%02i/%02i  %2i:%02i:%02i  %4s",
             Info->cFileName, Info->nFileSizeLow,
             st.wDay, st.wMonth, st.wYear,
             st.wHour, st.wMinute, st.wSecond,
             attstr
            );

/*** Return indicating success */

  return(TRUE);
}


//**********************************************************************
//**********************************************************************

BOOL notify(WCHAR* s)
{
  RhsIO.printf(L"%s\r\n", s);
  return(TRUE);
}


