// *********************************************************************
// filecomp
// ========
// Compare file(s)
//
// John Rennie
// 25/01/10
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
BOOL FileCompSub(WCHAR* Source, WCHAR* Dest);
BOOL CompareFileBinary(const WCHAR* SrcFile, const WCHAR* DestFile);
BOOL CompareFileASCII(const WCHAR* SrcFile, const WCHAR* DestFile);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;

int g_NumCompared, g_NumDiffs;

BOOL g_ASCII, g_Recurse, g_Hidden, g_Quiet, g_System;

CRhsWildCard g_WildCard;

#define SYNTAX \
L"FileComp v1.0\r\n" \
L"Syntax: [-a -d -h -q -s] <source file name> <dest file name>\r\n" \
L"Flags:\r\n" \
L"  -a assume the files are text (ASCII) files not binary\r\n" \
L"  -d recurse into subdirectories\r\n" \
L"  -h include hidden files\r\n" \
L"  -q don't list files compared, just differences\r\n" \
L"  -s include system files\r\n"


//**********************************************************************
// main
// ----
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"FileComp"))
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
  WCHAR source[LEN_FILENAME+1], dest[LEN_FILENAME+1];

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// Process flags

  g_ASCII = g_Recurse = g_Hidden = g_Quiet = g_System = FALSE;

  for (numarg = 1; numarg < RhsIO.m_argc; numarg++)
  { if (RhsIO.m_argv[numarg][0] != '-')
      break;

    switch (RhsIO.m_argv[numarg][1])
    { case 'a':
      case 'A':
        g_ASCII = TRUE;
        break;

      case 'd':
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

// Get the filenames

  lstrcpy(source, RhsIO.m_argv[numarg]);
  lstrcpy(dest, RhsIO.m_argv[numarg+1]);

// Start searching

  g_NumCompared = g_NumDiffs = 0;

  RhsIO.printf(L"Comparing: %s\r\n" \
               L"with:      %s\r\n\r\n",
               source, dest);

  FileCompSub(source, dest);

  RhsIO.printf(L"%i files compared\r\n", g_NumCompared);
  RhsIO.printf(L"%i files different\r\n", g_NumDiffs);

/// All done so return indicating success

  return 0;
}


//**********************************************************************
// FileCompSub
// -----------
//**********************************************************************

BOOL FileCompSub(WCHAR* Source, WCHAR* Dest)
{ int i;
  CRhsFindFile ff;
  WCHAR srcname[LEN_FILENAME+1], srcdir[LEN_FILENAME+1],
        destname[LEN_FILENAME+1], destdir[LEN_FILENAME+1],
        srcfile[LEN_FILENAME+1], destfile[LEN_FILENAME+1],
        s[LEN_FILENAME+1];

// Split the file names into a path and name

    UtilsGetNameFromFilename(Source, srcname, LEN_FILENAME);
    UtilsGetPathFromFilename(Source, srcdir, LEN_FILENAME);
    if (lstrlen(srcdir) == 0)
      lstrcpy(srcdir, L".");

    UtilsGetNameFromFilename(Dest, destname, LEN_FILENAME);
    UtilsGetPathFromFilename(Dest, destdir, LEN_FILENAME);
    if (lstrlen(destdir) == 0)
      lstrcpy(destdir, L".");

// If we are recursing into subdirectories do the recursion now

  if (g_Recurse)
  {

// Now find all subdirectories of the source directory

    lstrcpy(s, srcdir);
    lstrcat(s, L"\\*");

    if (ff.First(s, s, LEN_FILENAME))
    { do
      {

  // If we've found a directory recurse into it

        if (ff.Attributes() & FILE_ATTRIBUTE_DIRECTORY)
        {

// If this "directory" is a junction point ignore it

          if (ff.Attributes() & FILE_ATTRIBUTE_REPARSE_POINT)
            continue;

// Ignore . and ..

          if (lstrcmp(s, L".") == 0 || lstrcmp(s, L"..") == 0)
            continue;

// The new source expression is the directory we've just found with the
// source file name appended to it.

          lstrcpy(srcfile, s);
          lstrcat(srcfile, L"\\");
          lstrcat(srcfile, srcname);

// To get the new dest expression take the path from Dest, append the
// name (just the name not the full path) of the directory we've just
// found, then append the dest file name.

          ff.Filename(s, LEN_FILENAME);
          lstrcpy(destfile, destdir);
          lstrcat(destfile, L"\\");
          lstrcat(destfile, s);
          lstrcat(destfile, L"\\");
          lstrcat(destfile, destname);

// Recurse into the subdirectory

          if (!FileCompSub(srcfile, destfile))
            return FALSE;
        }

// Find next file

      } while (ff.Next(s, LEN_FILENAME));
    }

    ff.Close();
  }

// Now we've searched all subdirectories, so search this directory

  if (ff.First(Source, s, LEN_FILENAME))
  { do
    {

  // Ignore directories

      if (ff.Attributes() & FILE_ATTRIBUTE_DIRECTORY)
        continue;

// Check the file attributes

      if (ff.Attributes() & FILE_ATTRIBUTE_HIDDEN)
        if (!g_Hidden)
          continue;

      if (ff.Attributes() & FILE_ATTRIBUTE_SYSTEM)
        if (!g_System)
          continue;

// The source file name is the file we've just found.

      lstrcpy(srcfile, s);

// To get the new dest expression take the path from Dest. Then use
// wildcard completion to build the destination file name.

      lstrcpy(destfile, destdir);
      lstrcat(destfile, L"\\");
      i = lstrlen(destfile);

      ff.Filename(s, LEN_FILENAME);
      if (!g_WildCard.Complete(srcname, s, destname, destfile + i, LEN_FILENAME - i))
      { RhsIO.printf(L"%s\r\n", g_WildCard.LastError());
        ff.Close();
        return FALSE;
      }

// Compare the files

      if (g_ASCII)
        CompareFileASCII(srcfile, destfile);
      else
        CompareFileBinary(srcfile, destfile);

      g_NumCompared++;

// Find next file

    } while (ff.Next(s, LEN_FILENAME));
  }

  ff.Close();

// All done so return indicating success

  return TRUE;
}


// *********************************************************************
// CompareFileBinary
// -----------------
// *********************************************************************

#define LEN_COMPBUF 0x10000

BOOL CompareFileBinary(const WCHAR* SrcFile, const WCHAR* DestFile)
{ DWORD srcread, destread, bytes_to_compare, d;
  double total_read;
  HANDLE srcfile, destfile;
  char srcbuf[LEN_COMPBUF], destbuf[LEN_COMPBUF];

  total_read = 0;

// If required print the operation details

  if (!g_Quiet)
    RhsIO.printf(L"Comparing files \"%s\" and \"%s\"\r\n", SrcFile, DestFile);

// Open the source file

  srcfile = CreateFile(SrcFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

  if (srcfile == INVALID_HANDLE_VALUE)
  { RhsIO.errprintf(L"Cannot open source \"%s\": \"%s\"\r\n\r\n", SrcFile, GetLastErrorMessage());
    return TRUE;
  }

// Check that the destination file exists

  if (GetFileAttributes(DestFile) == (DWORD) -1)
  { RhsIO.printf(L"Destination file \"%s\" does not exist\r\n\r\n", DestFile);
    CloseHandle(srcfile);
    g_NumDiffs++;
    return TRUE;
  }

// Open the destination file

  destfile = CreateFile(DestFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

  if (destfile == INVALID_HANDLE_VALUE)
  { RhsIO.errprintf(L"Cannot open destination \"%s\": \"%s\"\r\n\r\n", SrcFile, GetLastErrorMessage());
    CloseHandle(srcfile);
    g_NumDiffs++;
    return TRUE;
  }

// Do the comparison

  for (;;)
  { if (!ReadFile(srcfile, srcbuf, LEN_COMPBUF, &srcread, NULL))
    { RhsIO.errprintf(L"Error reading source \"%s\": \"%s\"\r\n\r\n", SrcFile, GetLastErrorMessage());
      break;
    }

    if (!ReadFile(destfile, destbuf, LEN_COMPBUF, &destread, NULL))
    { RhsIO.errprintf(L"Error reading destination \"%s\": \"%s\"\r\n\r\n", DestFile, GetLastErrorMessage());
      break;
    }

// Check end of file

    if (srcread == 0 && destread == 0)
    { if (!g_Quiet)
        RhsIO.printf(L"Files are identical\r\n\r\n");
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
    { if (g_Quiet)
        RhsIO.printf(L"Files \"%s\" and \"%s\" are different at offset %.0f\r\n\r\n", SrcFile, DestFile, total_read);
      else
        RhsIO.printf(L"Files are different at offset %.0f\r\n\r\n", total_read);

      g_NumDiffs++;
      break;
    }

// Check if we've reached the end of either file

    if (srcread < destread)
    { RhsIO.printf(L"Destination file \"%s\" is longer\r\n\r\n", DestFile);
      g_NumDiffs++;
      break;
    }

    if (destread < srcread)
    { RhsIO.printf(L"Source file \"%s\" is longer\r\n\r\n", SrcFile);
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


// *********************************************************************
// CompareFileASCII
// ----------------
// *********************************************************************

BOOL CompareFileASCII(const WCHAR* SrcFile, const WCHAR* DestFile)
{ int lines_read, err;
  FILE* srcfile;
  FILE* destfile;
  char srcbuf[LEN_COMPBUF], destbuf[LEN_COMPBUF];

// If required print the operation details

  if (!g_Quiet)
    RhsIO.printf(L"Comparing files \"%s\" and \"%s\"\r\n", SrcFile, DestFile);

// Open the source file

  if ((err = _wfopen_s(&srcfile, SrcFile, L"r")) != 0)
  { RhsIO.errprintf(L"Cannot open source \"%s\": \"%i\"\r\n\r\n", SrcFile, err);
    return TRUE;
  }

// Check that the destination file exists

  if (GetFileAttributes(DestFile) == (DWORD) -1)
  { RhsIO.printf(L"Destination file \"%s\" does not exist\r\n\r\n", SrcFile);
    CloseHandle(srcfile);
    g_NumDiffs++;
    return TRUE;
  }

// Open the destination file

  if (_wfopen_s(&destfile, DestFile, L"r") != 0)
  { RhsIO.errprintf(L"Cannot open destination \"%s\": \"%s\"\r\n\r\n", DestFile, GetLastErrorMessage());
    fclose(srcfile);
    g_NumDiffs++;
    return TRUE;
  }

// Do the comparison

  lines_read = 0;

  for (;;)
  {

// Check if we've reached the end of either file

    if (feof(srcfile) && feof(destfile))
    { if (!g_Quiet)
        RhsIO.printf(L"Files are identical\r\n\r\n");
      break;
    }

    if (feof(srcfile) && !feof(destfile))
    { RhsIO.printf(L"Destination file \"%s\" is longer\r\n\r\n", DestFile);
      g_NumDiffs++;
      break;
    }

    if (!feof(srcfile) && feof(destfile))
    { RhsIO.printf(L"Source file \"%s\" is longer\r\n\r\n", SrcFile);
      g_NumDiffs++;
      break;
    }

// Read the next lines from the files

    if (!fgets(srcbuf, LEN_COMPBUF, srcfile) && !feof(srcfile))
    { RhsIO.errprintf(L"Error reading source \"%s\": \"%s\"\r\n\r\n", SrcFile, GetLastErrorMessage());
      break;
    }

    if (!fgets(destbuf, LEN_COMPBUF, destfile) && !feof(destfile))
    { RhsIO.errprintf(L"Error reading destination \"%s\": \"%s\"\r\n\r\n", DestFile, GetLastErrorMessage());
      break;
    }

    lines_read = lines_read + 1;

// Compare the data read

    if (lstrcmpA(srcbuf, destbuf) != 0)
    { if (g_Quiet)
        RhsIO.printf(L"Files \"%s\" and \"%s\" are different at line %d\r\n\r\n", SrcFile, DestFile, lines_read);
      else
        RhsIO.printf(L"Files are different at line %d\r\n\r\n", lines_read);

      g_NumDiffs++;
      break;
    }
  }

// Close the files

  fclose(srcfile);
  fclose(destfile);

// Return indicating success

  return TRUE;
}


