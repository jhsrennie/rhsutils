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
#include <Misc/CRhsFindFile.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);

BOOL DirComp(const WCHAR* SrcDir, const WCHAR* DestDir);
BOOL DirCompFile(const WCHAR* SrcFile, const WCHAR* DestFile);

const WCHAR* GetLastErrorMessage(void);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;

int g_NumCompared = 0;
int g_NumDiffs = 0;

BOOL g_SubDir = FALSE;
int g_Verbose = 1;


#define SYNTAX \
L"dircomp v1.0\n" \
L"------------\n" \
L"dircomp [-d -v0/1/2] <source dir> <destination dir>\n" \
L"\n" \
L"Compare two directories\n" \
L" -d  recurse into subdirectories\r\n" \
L" -v0 don't print file or directory names\n" \
L" -v1 print directory names (default)\n" \
L" -v2 print file names\n"

#define MAX_FILENAMELEN 2048

#define IsDirectory(x) \
  ((x) & FILE_ATTRIBUTE_DIRECTORY)


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"dircomp"))
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
{ int argnum;
  DWORD attrib;
  WCHAR srcdir[MAX_FILENAMELEN], destdir[MAX_FILENAMELEN];

// Check the arguments

  argnum = 1;

  while (argnum < RhsIO.m_argc)
  { if (RhsIO.m_argv[argnum][0] != '-')
      break;

    CharUpperBuff(RhsIO.m_argv[argnum]+1, 1);

    switch (RhsIO.m_argv[argnum][1])
    { case '?':
        RhsIO.printf(SYNTAX);
        return 0;

      case 'D':
        g_SubDir = TRUE;
        break;

      case 'V':
        if (RhsIO.m_argv[argnum][2] == '0')
        { g_Verbose = 0;
        }
        else if (RhsIO.m_argv[argnum][2] == '1')
        { g_Verbose = 1;
        }
        else if (RhsIO.m_argv[argnum][2] == '2')
        { g_Verbose = 2;
        }
        else
        { RhsIO.errprintf(L"dircomp: Unknown flag: %s\n-v needs to be followed by 0, 1 or 2\n", RhsIO.m_argv[argnum]);
          return 2;
        }
        break;

      default:
        RhsIO.errprintf(L"dircomp: Unknown flag \"%s\".\n%s", RhsIO.m_argv[argnum], SYNTAX);
        return 1;
    }

    argnum++;
  }

// Check there are two arguments left

  if (RhsIO.m_argc - argnum < 2)
  { RhsIO.errprintf(L"dircomp: Not enough arguments were given.\n%s", SYNTAX);
    return(2);
  }

// Check the source directory

  lstrcpy(srcdir, RhsIO.m_argv[argnum]);

  attrib = GetFileAttributes(srcdir);

  if (attrib == 0xFFFFFFFF)
  { RhsIO.errprintf(L"dircomp: Cannot find the source directory \"%s\".\n", srcdir);
    return 2;
  }

  if (!IsDirectory(attrib))
  { RhsIO.errprintf(L"dircomp: source \"%s\" is not a directory.\n", srcdir);
    return 2;
  }

// Check the destination directory

  lstrcpy(destdir, RhsIO.m_argv[argnum+1]);

  attrib = GetFileAttributes(destdir);

  if (attrib == 0xFFFFFFFF)
  { RhsIO.errprintf(L"dircomp: Cannot find the destination directory \"%s\".\n", destdir);
    return 2;
  }

  if (!IsDirectory(attrib))
  { RhsIO.errprintf(L"dircomp: destination \"%s\" is not a directory.\n", destdir);
    return 2;
  }

// Do the comparison

  RhsIO.printf(L"Comparing \"%s\" with \"%s\"\n", srcdir, destdir);

  if (!DirComp(srcdir, destdir))
    return 2;

  RhsIO.printf(L"No. files compared: %i\n", g_NumCompared);
  RhsIO.printf(L"No. differences:    %i\n", g_NumDiffs);

// Return indicating success

  return 0;
}



//**********************************************************************
// DirComp
// -------
//**********************************************************************

BOOL DirComp(const WCHAR* SrcDir, const WCHAR* DestDir)
{ int destfilelen;
  DWORD attrib;
  CRhsFindFile ff;
  WCHAR srcfile[MAX_FILENAMELEN], destfile[MAX_FILENAMELEN];

// Find all files in the specified directory

  lstrcpy(srcfile, SrcDir);
  lstrcat(srcfile, L"\\*");

  lstrcpy(destfile, DestDir);
  lstrcat(destfile, L"\\");
  destfilelen = lstrlen(destfile);

  if (ff.First(srcfile, srcfile, MAX_FILENAMELEN-1))
  { do
    {

// Build the matching destination file name

      ff.Filename(destfile + destfilelen, MAX_FILENAMELEN-(destfilelen+1));

// If we've found a directory recurse into it

      if (ff.Attributes() & FILE_ATTRIBUTE_DIRECTORY)
      { if (g_SubDir)
        { attrib = GetFileAttributes(destfile);

          if (attrib == 0xFFFFFFFF)
          { RhsIO.printf(L"Destination directory \"%s\" does not exist\n", destfile);
          }
          else if (!IsDirectory(attrib))
          { RhsIO.printf(L"Destination \"%s\" is not a directory.\n", destfile);
          }
          else
          { if (g_Verbose >= 1)
              RhsIO.printf(L"Comparing directories \"%s\" and \"%s\"\n", srcfile, destfile);

            if (!DirComp(srcfile, destfile))
              break;
          }
        }
      }

// Otherwise we've found a file

      else
      { if (!DirCompFile(srcfile, destfile))
          break;
      }

// Next file

    } while (ff.Next(srcfile, MAX_PATH));
  }

// Search has finished so close it

  ff.Close();

// Return indicating success

  return TRUE;
}


// *********************************************************************
// DirCompFile
// -----------
// *********************************************************************

#define LEN_COMPBUF 0x10000

BOOL DirCompFile(const WCHAR* SrcFile, const WCHAR* DestFile)
{ DWORD srcread, destread, bytes_to_compare, d;
  double total_read;
  HANDLE srcfile, destfile;
  char srcbuf[LEN_COMPBUF], destbuf[LEN_COMPBUF];

  total_read = 0;

// If required print the operation details

  if (g_Verbose >= 2)
    RhsIO.printf(L"Comparing files \"%s\" and \"%s\"\n", SrcFile, DestFile);

// Open the source file

  srcfile = CreateFile(SrcFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

  if (srcfile == INVALID_HANDLE_VALUE)
  { RhsIO.errprintf(L"Cannot open source \"%s\": \"%s\"\n", SrcFile, GetLastErrorMessage());
    return TRUE;
  }

  g_NumCompared++;

// Check that the destination file exists

  if (GetFileAttributes(DestFile) == (DWORD) -1)
  { RhsIO.printf(L"Destination file \"%s\" does not exist\n", DestFile);
    CloseHandle(srcfile);
    g_NumDiffs++;
    return TRUE;
  }

// Open the destination file

  destfile = CreateFile(DestFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

  if (destfile == INVALID_HANDLE_VALUE)
  { RhsIO.errprintf(L"Cannot open destination \"%s\": \"%s\"\n", DestFile, GetLastErrorMessage());
    CloseHandle(srcfile);
    g_NumDiffs++;
    return TRUE;
  }

// Do the comparison

  for (;;)
  { if (!ReadFile(srcfile, srcbuf, LEN_COMPBUF, &srcread, NULL))
    { RhsIO.errprintf(L"Error reading source \"%s\": \"%s\"\n", SrcFile, GetLastErrorMessage());
      break;
    }

    if (!ReadFile(destfile, destbuf, LEN_COMPBUF, &destread, NULL))
    { RhsIO.errprintf(L"Error reading destination \"%s\": \"%s\"\n", DestFile, GetLastErrorMessage());
      break;
    }

// Check end of file

    if (srcread == 0 && destread == 0)
    { if (g_Verbose >= 2)
        RhsIO.printf(L"Files are identical\n");
      break;
    }

// Compare the data read

    bytes_to_compare = srcread < destread ? srcread : destread;

    for (d = 0; d < bytes_to_compare; d++)
    { if (srcbuf[d] != destbuf[d])
        break;
      total_read++;
    }

    if (d < bytes_to_compare)
    { RhsIO.printf(L"Files \"%s\" and \"%s\" are different at offset %.0f\n", SrcFile, DestFile, total_read);
      g_NumDiffs++;
      break;
    }

// Check if we've reached the end of either file

    if (srcread < destread)
    { RhsIO.printf(L"Destination file \"%s\" is longer\n", DestFile);
      g_NumDiffs++;
      break;
    }

    if (destread < srcread)
    { RhsIO.printf(L"Source file \"%s\" is longer\n", SrcFile);
      g_NumDiffs++;
      break;
    }
  }

// Close the files

  CloseHandle(srcfile);
  CloseHandle(destfile);

// Return indicating success

  return TRUE;
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


