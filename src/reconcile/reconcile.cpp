//**********************************************************************
// Reconcile
// =========
//
// Reconcile is a program to keep the contents of two drives and/or
// directories the same.  The syntax is:
//
// reconcil [-x -u -d -t -q -r -e -f] <source> <dest>
//
// -x: Any files on the source but not on the destination are copied
//     from the source to the destination.
//
// -u: Any files present on the destination dated earlier than the
//     same file on the source are copied from the source to the
//     destination.
//
// -d: Any files on the destination but not on the source are deleted.
//
// -t: Time stamps of destation files are changed to match the times of
//     the source files.
//
// -a: Destination files are moved to an archive before being overwritten
//     or deleted.
//
// -q: Suppress listing of operations
//
// -r: List operations but do not actually do anything.  This option
//     is useful for comparing drives/directories.
//
// -e: Normally any errors are fatal.  The -e option allows processing
//     to continue even if errors occur.
//
// -f: Use NTFS file date times, i.e. compare times to 1 second
//     resolution.  The default is the 2 second resolution of FAT.
//
// The program produces output consisting of one line per file action.
// The lines start with X, U, D or E for -x, -u, -d actions or errors
// so a pattern matching utility can be used to sift through the
// output.
//**********************************************************************

#include <windows.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <Misc/CRhsFindFile.h>
#include <Misc/CRhsDate.h>


//**********************************************************************
// reconcileINFO
// -------------
// Structure to hold information about reconcile operation
//**********************************************************************

#define MAX_FILENAMELEN 2048

typedef struct
{ WCHAR Source[MAX_FILENAMELEN+1],
       Destination[MAX_FILENAMELEN+1],
       ArchivePath[MAX_FILENAMELEN+1];

  BOOL Update,
       Create,
       Delete,
       TimeStamp,
       Archive,
       Quiet,
       Report,
       FailOnError,
       UseFATTimeStamp;

  int Updated,
      Created,
      Deleted,
      TimeStamped;

} RECONCILEINFO;


//**********************************************************************
// Function prototypes
// -------------------
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);

BOOL ReconcileUpdateFiles(WCHAR*, WCHAR*, WCHAR*, RECONCILEINFO*);
BOOL ReconcileDeleteFiles(WCHAR*, WCHAR*, WCHAR*, RECONCILEINFO*);
BOOL ReconcileTimeStampFiles(WCHAR*, WCHAR*, RECONCILEINFO*);

BOOL ReconcileArchiveFile(WCHAR*, WCHAR*);

DWORD FileExists(const WCHAR* FileName);

int  ReconcileCompareTime(SYSTEMTIME* One, SYSTEMTIME* Two, BOOL DosTime);

BOOL GetFileTimeByName(const WCHAR* FileName, SYSTEMTIME* Created, SYSTEMTIME* Accessed, SYSTEMTIME* Written);
BOOL SetFileTimeByName(const WCHAR* FileName, SYSTEMTIME* Created, SYSTEMTIME* Accessed, SYSTEMTIME* Written);

int  GetPathFromFileName(const WCHAR* FileName, WCHAR* Path, DWORD Length);
BOOL AddCurrentPath(WCHAR* FileName);
BOOL RemoveTrailingSlash(WCHAR* FileName);

const WCHAR* GetLastErrorMessage(void);

#define IsDirectory(x) \
  ((x) & FILE_ATTRIBUTE_DIRECTORY)


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;

#define SYNTAX \
  L"reconcile [-x -u -d -t -a<archive> -q -r -e -f] <source> <dest>\r\n"


#define LEN_HELP 37
static const WCHAR* HELP[LEN_HELP] =
{
  L"Reconcile v1.0.2\r\n",
  L"----------------\r\n",
  L"Reconcile is a program to keep the contents of two drives and/or\r\n",
  L"directories the same.  The syntax is:\r\n",
  L"\r\n",
  L"reconcil [-x -u -d -t -q -r -e -f] <source> <dest>\r\n",
  L"\r\n",
  L"-x: Any files on the source but not on the destination are copied\r\n",
  L"    from the source to the destination.\r\n",
  L"\r\n",
  L"-u: Any files present on the destination dated earlier than the\r\n",
  L"    same file on the source are copied from the source to the\r\n",
  L"    destination.\r\n",
  L"\r\n",
  L"-d: Any files on the destination but not on the source are deleted.\r\n",
  L"\r\n",
  L"-t: Time stamps of destation files are changed to match the times of\r\n",
  L"    the source files.\r\n",
  L"\r\n",
  L"-a: Destination files are moved to an archive before being overwritten\r\n",
  L"    or deleted.\r\n",
  L"\r\n",
  L"-q: Suppress listing of operations\r\n",
  L"\r\n",
  L"-r: List operations but do not actually do anything.  This option\r\n",
  L"    is useful for comparing drives/directories.\r\n",
  L"\r\n",
  L"-e: Normally any errors are fatal.  The -e option allows processing\r\n",
  L"    to continue even if errors occur.\r\n",
  L"\r\n",
  L"-f: Use NTFS file date times, i.e. compare times to 1 second\r\n",
  L"    resolution.  The default is the 2 second resolution of FAT.\r\n",
  L"\r\n",
  L"The program produces output consisting of one line per file action.\r\n",
  L"The lines start with X, U, D or E for -x, -u, -d actions or errors\r\n",
  L"so a pattern matching utility can be used to sift through the\r\n",
  L"output.\r\n"
};


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"reconcile"))
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
{ int argnum, i;
  DWORD attrib;
  RECONCILEINFO ri;

// Set flags for the comparison

  lstrcpy(ri.ArchivePath, L"");

  ri.Update          = FALSE;
  ri.Create          = FALSE;
  ri.Delete          = FALSE;
  ri.TimeStamp       = FALSE;
  ri.Archive         = FALSE;
  ri.Quiet           = FALSE;
  ri.Report          = FALSE;
  ri.FailOnError     = TRUE;
  ri.UseFATTimeStamp = TRUE;

  argnum = 1;

  while (argnum < RhsIO.m_argc)
  { if (RhsIO.m_argv[argnum][0] != '-')
      break;

    CharUpperBuff(RhsIO.m_argv[argnum]+1, 1);

    switch (RhsIO.m_argv[argnum][1])
    { case '?':
        for (i = 0; i < LEN_HELP; i++)
          RhsIO.printf(HELP[i]);
        return(0);

      case 'U':
        ri.Update = TRUE;
        break;

      case 'X':
        ri.Create = TRUE;
        break;

      case 'D':
        ri.Delete = TRUE;
        break;

      case 'T':
        ri.TimeStamp = TRUE;
        break;

      case 'A':
        lstrcpyn(ri.ArchivePath, RhsIO.m_argv[argnum] + 2, MAX_FILENAMELEN);
        ri.ArchivePath[MAX_FILENAMELEN] = '\0';
        ri.Archive = TRUE;
        break;

      case 'Q':
        ri.Quiet = TRUE;
        break;

      case 'R':
        ri.Report = TRUE;
        break;

      case 'E':
        ri.FailOnError = FALSE;
        break;

      case 'F':
        ri.UseFATTimeStamp = FALSE;
        break;

      default:
        RhsIO.printf(L"reconcile: Unknown flag \"%s\".\r\n%s", RhsIO.m_argv[argnum], SYNTAX);
        return 1;
    }

    argnum++;
  }

// Check there are two arguments left

  if (RhsIO.m_argc - argnum < 2)
  { RhsIO.printf(L"reconcile: Not enough arguments were given.\r\n%s", SYNTAX);
    return(2);
  }

// Check quiet and report flags don't clash

  if (ri.Quiet && ri.Report)
  { RhsIO.printf(L"reconcile: The -q and -r flags are mutually exclusive.");
    return 1;
  }

// Make sure we have something to do

  if (!ri.Update && !ri.Create && !ri.Delete && !ri.TimeStamp)
  { RhsIO.printf(L"reconcile: No action was requested.");
    return 1;
  }

// Check the first argument i.e. source directory

  lstrcpy(ri.Source, RhsIO.m_argv[argnum++]);

  RemoveTrailingSlash(ri.Source);
  AddCurrentPath(ri.Source);

  attrib = GetFileAttributes(ri.Source);

  if (attrib == 0xFFFFFFFF)
  { RhsIO.printf(L"reconcile: Cannot find the source directory \"%s\".\r\n", ri.Source);
    return 1;
  }

  if (!IsDirectory(attrib))
  { RhsIO.printf(L"reconcile: source \"%s\" is not a directory.\r\n", ri.Source);
    return 1;
  }

// Check the second argument i.e. destination directory

  lstrcpy(ri.Destination, RhsIO.m_argv[argnum++]);
  RemoveTrailingSlash(ri.Destination);
  AddCurrentPath(ri.Destination);

  attrib = GetFileAttributes(ri.Destination);

  // GetFileAttributes fails for USB drive letters prefixed with "\\?\"
  // so try with the prefix removed.
  if (attrib == 0xFFFFFFFF)
    if (lstrlen(ri.Destination) == 6)
      if (ri.Destination[0] == '\\' && ri.Destination[1] == '\\' && ri.Destination[2] == '?' && ri.Destination[3] == '\\' && ri.Destination[5] == ':')
        attrib = GetFileAttributes(ri.Destination+4);

  if (attrib == 0xFFFFFFFF)
  { RhsIO.printf(L"reconcile: Cannot find the destination directory \"%s\".\r\n", ri.Destination);
    return 1;
  }

  if (!IsDirectory(attrib))
  { RhsIO.printf(L"reconcile: destination \"%s\" is not a directory.\r\n", ri.Destination);
    return 1;
  }

// If an archive directory was specified check it now

  if (ri.Archive)
  { RemoveTrailingSlash(ri.ArchivePath);
    AddCurrentPath(ri.ArchivePath);

    attrib = GetFileAttributes(ri.ArchivePath);

    if (attrib == 0xFFFFFFFF)
    { RhsIO.printf(L"reconcile: Cannot find the archive directory \"%s\".\r\n", ri.Destination);
      return 1;
    }

    if (!IsDirectory(attrib))
    { RhsIO.printf(L"reconcile: archive \"%s\" is not a directory.\r\n", ri.Destination);
      return 1;
    }
  }

// Reconcile

  if (!ri.Quiet)
  { RhsIO.printf(L"Reconcile from\r\n%s\r\nto\r\n%s\r\n", ri.Source, ri.Destination);
    if (ri.Archive)
      RhsIO.printf(L"archive to\r\n%s\r\n", ri.ArchivePath);
    RhsIO.printf(L"\r\n");
  }

  ri.Updated = ri.Created = ri.Deleted = ri.TimeStamped = 0;

  if (ri.Update || ri.Create)
    if (!ReconcileUpdateFiles(ri.Source, ri.Destination, ri.ArchivePath, &ri))
      return 1;

  if (ri.Delete)
    if (!ReconcileDeleteFiles(ri.Source, ri.Destination, ri.ArchivePath, &ri))
      return 1;

  if (ri.TimeStamp)
    if (!ReconcileTimeStampFiles(ri.Source, ri.Destination, &ri))
      return 1;

  RhsIO.printf(L"\r\n%i files updated\r\n%i files created\r\n%i files deleted\r\n%i files timestamped\r\n", ri.Updated, ri.Created, ri.Deleted, ri.TimeStamped);

// All done

  return(0);
}


//**********************************************************************
// ReconcileUpdateFiles
// --------------------
// Update/Create from File1 to File2, optionally archiving to File3
//**********************************************************************

BOOL ReconcileUpdateFiles(WCHAR* File1, WCHAR* File2, WCHAR* File3, RECONCILEINFO* RInfo)
{ DWORD d;
  WCHAR file1[MAX_FILENAMELEN+1], file2[MAX_FILENAMELEN+1], file3[MAX_FILENAMELEN+1];
  SYSTEMTIME fdate1, fdate2;
  HANDLE h;
  WIN32_FIND_DATA wfd;

// Start the search

  lstrcpy(file1, File1);
  lstrcat(file1, L"\\*");
  h = FindFirstFile(file1, &wfd);

  if (h != INVALID_HANDLE_VALUE)
  { do
    {

// Check for an abort signal

      if (RhsIO.GetAbort())
        break;

// Check file is one we want

      if (lstrcmp(wfd.cFileName, L".") == 0 || lstrcmp(wfd.cFileName, L"..") == 0)
        continue;

// Build source and destination name

      lstrcpy(file1, File1);
      lstrcat(file1, L"\\");
      lstrcat(file1, wfd.cFileName);

      lstrcpy(file2, File2);
      lstrcat(file2, L"\\");
      lstrcat(file2, wfd.cFileName);

      lstrcpy(file3, File3);
      lstrcat(file3, L"\\");
      lstrcat(file3, wfd.cFileName);

// ---------------------------------------------------------------------
// This section deals with directories

      if (IsDirectory(wfd.dwFileAttributes))
      { d = FileExists(file2);

// If we are updating or creating and the target directory exists, recurse into it

        if ((RInfo->Update || RInfo->Create) && d)
        { if (!ReconcileUpdateFiles(file1, file2, file3, RInfo))
          { FindClose(h);
            return(FALSE);
          }
        }

// If we are creating and the target directory does not exist, create it

        if (RInfo->Create && !d)
        { if (!RInfo->Quiet)
            RhsIO.printf(L"X Creating destination directory %s\r\n", file2);

          if (!RInfo->Report)
          { if (!CreateDirectory(file2, NULL))
            { if (!RInfo->Quiet)
                RhsIO.errprintf(L"E Cannot create destination directory %s: %s\r\n", file2, GetLastErrorMessage());

              if (RInfo->FailOnError)
              { FindClose(h);
                return(FALSE);
              }
              else
              { continue;
              }
            }
          }

          if (!ReconcileUpdateFiles(file1, file2, file3, RInfo))
          { FindClose(h);
            return(FALSE);
          }
        }
      }

// ---------------------------------------------------------------------
// This section deals with files

      else
      { d = FileExists(file2);

// If we are updating check the file timestamps

        if (RInfo->Update && d)
        { GetFileTimeByName(file1, NULL, NULL, &fdate1);
          GetFileTimeByName(file2, NULL, NULL, &fdate2);

          if (ReconcileCompareTime(&fdate1, &fdate2, RInfo->UseFATTimeStamp) <= 0)
            continue;

          if (!RInfo->Quiet)
            RhsIO.printf(L"U %s %s\r\n", file1, file2);

          if (!RInfo->Report)
          {

// If required archive the old file

            if (RInfo->Archive)
            { if (!RInfo->Quiet)
                RhsIO.printf(L"A %s %s\r\n", file2, file3);

              if (!ReconcileArchiveFile(file2, file3))
              { if (!RInfo->Quiet)
                  RhsIO.errprintf(L"E Cannot archive %s to %s: %s\r\n", file1, file3, GetLastErrorMessage());

                if (RInfo->FailOnError)
                { FindClose(h);
                  return(FALSE);
                }
                else
                { continue;
                }
              }
            }

// Copy the new file

            SetFileAttributes(file2, 0);
            if (!CopyFile(file1, file2, FALSE))
            { if (!RInfo->Quiet)
                RhsIO.errprintf(L"E Cannot copy %s to %s: %s\r\n", file1, file2, GetLastErrorMessage());

              if (RInfo->FailOnError)
              { FindClose(h);
                return(FALSE);
              }
              else
              { continue;
              }
            }
          }
          RInfo->Updated++;
        }

// If we are creating copy the file

        if (RInfo->Create && !d)
        { if (!RInfo->Quiet)
            RhsIO.printf(L"X %s %s\r\n", file1, file2);

          if (!RInfo->Report)
          { if (!CopyFile(file1, file2, FALSE))
            { if (!RInfo->Quiet)
                RhsIO.errprintf(L"E Cannot copy %s to %s: %s\r\n", file1, file2, GetLastErrorMessage());

              if (RInfo->FailOnError)
              { FindClose(h);
                return(FALSE);
              }
              else
              { continue;
              }
            }
          }

          RInfo->Created++;
        }
      }

// Get next file

    } while (FindNextFile(h, &wfd));

    FindClose(h);
  }

// All done so return

  return(TRUE);
}


//**********************************************************************
// ReconcileDeleteFiles
// --------------------
// Routine to delete all files in a source directory which are not in
// a destination directory.
//**********************************************************************

BOOL ReconcileDeleteFiles(WCHAR* File1, WCHAR* File2, WCHAR* File3, RECONCILEINFO* RInfo)
{ DWORD d;
  WCHAR file1[MAX_FILENAMELEN+1], file2[MAX_FILENAMELEN+1], file3[MAX_FILENAMELEN+1];
  HANDLE h;
  WIN32_FIND_DATA wfd;

// Start the search

  lstrcpy(file2, File2);
  lstrcat(file2, L"\\*");
  h = FindFirstFile(file2, &wfd);

  if (h != INVALID_HANDLE_VALUE)
  { do
    {

// Check for an abort signal

      if (RhsIO.GetAbort())
        break;

// Check file is one we want

      if (lstrcmp(wfd.cFileName, L".") == 0 || lstrcmp(wfd.cFileName, L"..") == 0)
        continue;

// Build source and destination name

      lstrcpy(file1, File1);
      lstrcat(file1, L"\\");
      lstrcat(file1, wfd.cFileName);

      lstrcpy(file2, File2);
      lstrcat(file2, L"\\");
      lstrcat(file2, wfd.cFileName);

      lstrcpy(file3, File3);
      lstrcat(file3, L"\\");
      lstrcat(file3, wfd.cFileName);

// If the source file exists and it's not a directory then continue

      d = FileExists(file1);
      if (d > 1)
      { RhsIO.errprintf(L"E Error checking %s: %s\r\n", file1, GetLastErrorMessage());
        return FALSE;
      }

      if (d == 1)
        if (!IsDirectory(wfd.dwFileAttributes))
          continue;

// Check if name found is a directory

      if (IsDirectory(wfd.dwFileAttributes))
      { if (!ReconcileDeleteFiles(file1, file2, file3, RInfo))
          return(FALSE);

// If the source dir doesn't exist then remove the destination dir

        d = FileExists(file1);
        if (d > 1)
        { RhsIO.errprintf(L"E Error checking %s: %s\r\n", file1, GetLastErrorMessage());
          return FALSE;
        }

        if (d == 0)
        { if (!RInfo->Report)
          { if (!RInfo->Quiet)
              RhsIO.printf(L"D Deleting directory %s\r\n", file2);

            SetFileAttributes(file2, 0);

            if (!RemoveDirectory(file2))
            { if (!RInfo->Quiet)
                RhsIO.errprintf(L"E Cannot remove directory %s: %s\r\n", file2, GetLastErrorMessage());

              if (RInfo->FailOnError)
              { FindClose(h);
                return(FALSE);
              }
              else
              { continue;
              }
            }
          }
        }
      }

// Name found is a file

      else
      { if (!RInfo->Quiet)
          RhsIO.printf(L"D %s\r\n", file2);

        if (!RInfo->Report)
        {

// If required archive the old file

          if (RInfo->Archive)
          { if (!RInfo->Quiet)
              RhsIO.printf(L"A %s %s\r\n", file2, file3);

            if (!ReconcileArchiveFile(file2, file3))
            { if (!RInfo->Quiet)
                RhsIO.errprintf(L"E Cannot archive %s to %s: %s\r\n", file2, file3, GetLastErrorMessage());

              if (RInfo->FailOnError)
              { FindClose(h);
                return(FALSE);
              }
              else
              { continue;
              }
            }
          }

// Delete the file

          SetFileAttributes(file2, 0);

          if (!DeleteFile(file2))
          { if (!RInfo->Quiet)
              RhsIO.errprintf(L"E Cannot delete file %s: %s\r\n", file2, GetLastErrorMessage());

            if (RInfo->FailOnError)
            { FindClose(h);
              return(FALSE);
            }
            else
            { continue;
            }
          }
        }

        RInfo->Deleted++;
      }

// Get next file

    } while (FindNextFile(h, &wfd));

    FindClose(h);
  }

// All done so return

  return(TRUE);
}


//**********************************************************************
//**********************************************************************

BOOL ReconcileTimeStampFiles(WCHAR* File1, WCHAR* File2, RECONCILEINFO* RInfo)
{ DWORD d;
  WCHAR file1[MAX_FILENAMELEN+1], file2[MAX_FILENAMELEN+1];
  SYSTEMTIME fdate1, fdate2;
  HANDLE h;
  WIN32_FIND_DATA wfd;

// Start the search

  lstrcpy(file1, File1);
  lstrcat(file1, L"\\*");
  h = FindFirstFile(file1, &wfd);

  if (h != INVALID_HANDLE_VALUE)
  { do
    {

// Check for an abort signal

      if (RhsIO.GetAbort())
        break;

// Check file is one we want

      if (lstrcmp(wfd.cFileName, L".") == 0 || lstrcmp(wfd.cFileName, L"..") == 0)
        continue;

// Build source and destination name

      lstrcpy(file1, File1);
      lstrcat(file1, L"\\");
      lstrcat(file1, wfd.cFileName);

      lstrcpy(file2, File2);
      lstrcat(file2, L"\\");
      lstrcat(file2, wfd.cFileName);

// If destination file doesn't exist then continue

      d = FileExists(file2);

      if (d != 1)
        continue;

// Check if name found is a directory

      if (IsDirectory(wfd.dwFileAttributes))
      { if (!ReconcileTimeStampFiles(file1, file2, RInfo))
        { FindClose(h);
          return(FALSE);
        }
      }

// Name found is a file

      else
      { GetFileTimeByName(file1, NULL, NULL, &fdate1);
        GetFileTimeByName(file2, NULL, NULL, &fdate2);

        if (ReconcileCompareTime(&fdate1, &fdate2, RInfo->UseFATTimeStamp) == 0)
          continue;

        if (!RInfo->Quiet)
          RhsIO.printf(L"T %s %s %i/%i/%i %02i:%02i:%02i\r\n", file1, file2, fdate1.wDay, fdate1.wMonth, fdate1.wYear%100, fdate1.wHour, fdate1.wMinute, fdate1.wSecond);

        if (!RInfo->Report)
        { if (!SetFileTimeByName(file2, NULL, NULL, &fdate1))
          { if (!RInfo->Quiet)
              RhsIO.errprintf(L"E Cannot timestamp %s to match %s: %s\r\n", file2, file1, GetLastErrorMessage());

            if (RInfo->FailOnError)
            { FindClose(h);
              return(FALSE);
            }
            else
            { continue;
            }
          }
        }

        RInfo->TimeStamped++;
      }

// Get next file

    } while (FindNextFile(h, &wfd));

    FindClose(h);
  }

// All done so return

  return(TRUE);
}


//**********************************************************************
// ReconcileArchiveFile
// --------------------
// Copy from File1 to File2.
// The destination path may not exist and may need to be created.
// We do this by stepping through the destination file name a directory
// at a time, and creating that directory if it doesn't already exist.
//**********************************************************************

BOOL ReconcileArchiveFile(WCHAR* File1, WCHAR* File2)
{ int i, j;
  WCHAR newdir[MAX_FILENAMELEN+1];

  i = 0;

// If this is a UNC name step past the server\share

  if (File2[0] == '\\' && File2[1] == '\\')
  { for (i = 2; File2[i] != '\\' && File2[i] != '\0'; i++);

    if (File2[i] != '\0')
      for (i++; File2[i] != '\\' && File2[i] != '\0'; i++);

    if (File2[i] != '\0')
      i++;
  }

// Else if it's a drive letter step past the drive letter

  else if (File2[1] == ':')
  { i = 2;
    if (File2[i] == '\\')
      i++;
  }

// Else if we start with '\' step past the '\'

  else if (File2[0] == '\\')
  { i = 1;
  }

// Copy the bit we've skipped past into our directory buffer

  for (j = 0; j < i; j++)
    newdir[j] = File2[j];

// Now go through the path

  while (File2[i] != '\0')
  { while (File2[i] != '\\' && File2[i] != '\0')
    { newdir[i] = File2[i];
      i++;
    }

    if (File2[i] == '\\')
    { newdir[i] = '\0';

      if (!FileExists(newdir))
        if (!CreateDirectory(newdir, NULL))
          RhsIO.printf(L"ReconcileArchiveFile error creating \"%s\": %s\r\n", newdir, GetLastErrorMessage());

      newdir[i++] = '\\';
    }
  }

// Copy the file

  if (!CopyFile(File1, File2, FALSE))
    return(FALSE);

// Return indicating success

  return TRUE;
}


//**********************************************************************
// ReconcileCompareTime
// --------------------
//**********************************************************************

int ReconcileCompareTime(SYSTEMTIME* One, SYSTEMTIME* Two, BOOL DosTime)
{

// Compare fields in SYSTEMTIME

  if (One->wYear > Two->wYear)
    return 1;
  else if (One->wYear < Two->wYear)
    return(-1);

  if (One->wMonth > Two->wMonth)
    return 1;
  else if (One->wMonth < Two->wMonth)
    return(-1);

  if (One->wDay > Two->wDay)
    return 1;
  else if (One->wDay < Two->wDay)
    return(-1);

  if (One->wHour > Two->wHour)
    return 1;
  else if (One->wHour < Two->wHour)
    return(-1);

  if (One->wMinute > Two->wMinute)
    return 1;
  else if (One->wMinute < Two->wMinute)
    return(-1);

// If using FAT times round seconds down

  if (DosTime)
  { if (One->wSecond - Two->wSecond > 2)
      return 1;
    else if (One->wSecond - Two->wSecond < -2)
      return(-1);
  }
  else
  { if (One->wSecond > Two->wSecond)
      return 1;
    else if (One->wSecond < Two->wSecond)
      return(-1);
  }

// Times are equal

  return(0);
}


//**********************************************************************
// FileExists
// ----------
// Returns 0 if the file is not found or path not found
// Returns 1 if the file is found
// Returns the Windows error code on any other error
//  e.g. 53 (ERROR_BAD_NETPATH) means a network error
//**********************************************************************

DWORD FileExists(const WCHAR* FileName)
{ DWORD d;

// if GetFileAttributes succeeds return 1 to indicate file found

  if (GetFileAttributes(FileName) != 0xFFFFFFFF)
    return 1;

// if GetFileAttributes failed we need the Windows error

  d = GetLastError();

// If the error was file/path not found return 0 to indicate file not found

  if (d == ERROR_FILE_NOT_FOUND || d == ERROR_PATH_NOT_FOUND)
    return 0;

// Else return the error

  return d;
}


//**********************************************************************
// GetFileTimeByName
// -----------------
// Get the file time.
// The file has to be opened to get the time.
//**********************************************************************

BOOL GetFileTimeByName(const WCHAR* FileName, SYSTEMTIME* Created, SYSTEMTIME* Accessed, SYSTEMTIME* Written)
{ BOOL success;
  HANDLE h;
  FILETIME created, accessed, written;

  h = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (!h)
    return(FALSE);

  success = GetFileTime(h, &created, &accessed, &written);

  if (success)
  { if (Created)
      FileTimeToSystemTime(&created, Created);

    if (Accessed)
      FileTimeToSystemTime(&accessed, Accessed);

    if (Written)
      FileTimeToSystemTime(&written, Written);
  }

  CloseHandle(h);

// Return error code

  return(success);
}


//**********************************************************************
// SetFileTimeByName
// -----------------
// Set the file time.
// The file has to be opened to set the time.
//**********************************************************************

BOOL SetFileTimeByName(const WCHAR* FileName, SYSTEMTIME* Created, SYSTEMTIME* Accessed, SYSTEMTIME* Written)
{ BOOL success;
  HANDLE h;
  FILETIME created, accessed, written;
  LPFILETIME lpcreated = NULL, lpaccessed = NULL, lpwritten = NULL;

  h = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (!h)
    return(FALSE);

  if (Created)
  { SystemTimeToFileTime(Created, &created);
    lpcreated = &created;
  }

  if (Accessed)
  { SystemTimeToFileTime(Accessed, &accessed);
    lpaccessed = &accessed;
  }

  if (Written)
  { SystemTimeToFileTime(Written, &written);
    lpwritten = &written;
  }

  success = SetFileTime(h, lpcreated, lpaccessed, lpwritten);

  CloseHandle(h);

// Return error code

  return(success);
}


//**********************************************************************
// GetPathFromFileName
// -------------------
// Extract the path from a file name.
// The path does not have a trailing '\'.
//**********************************************************************

int GetPathFromFileName(const WCHAR* FileName, WCHAR* Path, DWORD Length)
{ DWORD i, j;

  i = 0;
  while (FileName[i] != '\0')
    i++;

  while (FileName[i] != '\\' && i > 0)
    i--;

  if (i >= Length)
  { lstrcpy(Path, L"");
    return(0);
  }

  for (j = 0; j < i; j++)
    Path[j] = FileName[j];
  Path[j] = '\0';

  return((int) j);
}


//**********************************************************************
// AddCurrentPath
// --------------
// Prefix the current directory to a file name
//**********************************************************************

BOOL AddCurrentPath(WCHAR* FileName)
{ WCHAR s[MAX_FILENAMELEN+1];

  if (FileName[0] != '\\' && FileName[1] != ':')
  { GetCurrentDirectory(MAX_FILENAMELEN, s);
    lstrcat(s, L"\\");
    lstrcat(s, FileName);
    lstrcpy(FileName, s);
  }

  return(TRUE);
}


//**********************************************************************
// RemoveTrailingSlash
// -------------------
// Remove a trailing '\' from a directory name
//**********************************************************************

BOOL RemoveTrailingSlash(WCHAR* FileName)
{ WCHAR* s;

  s = FileName;
  while (*s != '\0') s++;

  if (s - FileName > 0)
    if (*(s-1) == '\\')
      *(s-1) = '\0';

  return(TRUE);
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


