//**********************************************************************
// Utils.cpp
// =========
// Collection of useful functions, mostly to do with manipulating files.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 12/05/09
//**********************************************************************

#include <windows.h>
#include <Misc/CRhsFindFile.h>
#include "utils.h"


// *********************************************************************
// UtilsStripTrailingSlash
// -----------------------
// Remove any trailing backslash from a directory name
// *********************************************************************

BOOL UtilsStripTrailingSlash(WCHAR* Filename)
{ WCHAR* s;

  s = Filename;
  while (*s != '\0') s++;

  if (s - Filename > 0)
    if (*(s-1) == '\\')
      *(s-1) = '\0';

  return TRUE;
}


// *********************************************************************
// UtilsStripTrailingCRLF
// ----------------------
// Remove any trailing <CR><LF> from an ACSII string
// NB the string is ASCII not UNICODE.
// *********************************************************************

BOOL UtilsStripTrailingCRLF(char* String)
{ char* s;

  s = String;
  while (*s != '\0') s++;

// If the last character is a LF snip it off

  if (s - String > 0)
    if (*(s-1) == 10)
      *(--s) = '\0';

// If the last character is a CR snip it off

  if (s - String > 0)
    if (*(s-1) == 13)
      *(--s) = '\0';

  return TRUE;
}


// *********************************************************************
// UtilsAddCurPath
// ---------------
// If the filename is not an absolute name, i.e. doesn't start with X: or
// a backslash, prefix it with the current directory.
// *********************************************************************

BOOL UtilsAddCurPath(WCHAR* Filename, int BufLen)
{ int buflen;
  WCHAR s[LEN_FILENAME];

// Set the buffer length to whichever is the smaller of BufLen and LEN_FILENAME

  if (BufLen < LEN_FILENAME)
    buflen = BufLen;
  else
    buflen = LEN_FILENAME;

// Check if the filename is relative

  if (Filename[0] != '\\' && Filename[1] != ':')
  {

// Check the combined lengths fit in the buffer

    GetCurrentDirectory(LEN_FILENAME, s);

    if (lstrlen(s) + 1 + lstrlen(Filename) > buflen-1)
      return FALSE;

// Prefix the Filename with the current directory

    lstrcat(s, L"\\");
    lstrcat(s, Filename);
    lstrcpy(Filename, s);
  }

// Return indicating success

  return TRUE;
}


// *********************************************************************
// UtilsIsValidPath
// ----------------
// Check the directory exists
// *********************************************************************

BOOL UtilsIsValidPath(const WCHAR* Filename)
{ WCHAR s[LEN_FILENAME];

  UtilsGetPathFromFilename(Filename, s, LEN_FILENAME);

  if (lstrlen(s) > 0 && GetFileAttributes(s) == 0xFFFFFFFF)
    return(FALSE);

  return TRUE;
}


// *********************************************************************
// UtilsGetPathFromFilename
// ------------------------
// Extract the path from the supplied filename
// BufLen is the length of the array Path, i.e. including the \0
// *********************************************************************

BOOL UtilsGetPathFromFilename(const WCHAR* Filename, WCHAR* Path, int BufLen)
{ int i;

// Always set the extracted path to "" even if there is an error

  lstrcpy(Path, L"");

// Find the position of the last backslash

  for (i = 0; Filename[i] != '\0'; i++);
  while (Filename[i] != '\\' && i > 0)
    i--;

// At this point i is the number of characters to copy

  if (i > 0)
  {

// If the number of characters in the path exceeds the buffer length
// return FALSE

    if (i > BufLen-1)
      return FALSE;

// Else get the path

    lstrcpyn(Path, Filename, i+1);
  }

// Return indicating success

  return TRUE;
}


// *********************************************************************
// UtilsGetNameFromFilename
// ------------------------
// Extract the filename from the supplied fully qualified filename
// BufLen is the length of the array Name, i.e. including the \0
// *********************************************************************

BOOL UtilsGetNameFromFilename(const WCHAR* Filename, WCHAR* Name, int BufLen)
{ int i;

// Always set the extracted name to "" even if there is an error

  lstrcpy(Name, L"");

// Find the position of the character after the last backslash

  for (i = 0; Filename[i] != '\0'; i++);
  while (Filename[i] != '\\' && i > 0)
    i--;

  if (Filename[i] == '\\')
    i++;

// Check the filename isn't too long

  if (lstrlen(Filename + i) > BufLen-1)
    return FALSE;

// Else get the filename

  lstrcpy(Name, Filename + i);

// Return indicating success

  return TRUE;
}


// *********************************************************************
// UtilsFileInfoFormat
// -------------------
// *********************************************************************

BOOL UtilsFileInfoFormat(CRhsFindFile* FFInfo, BOOL Full, WCHAR* Result, int BufLen)
{ SYSTEMTIME st;
  DWORD size_low, size_high;

  WCHAR attstr[5];
  WCHAR s[LEN_FILENAME];

/*** Build file attribute string */

  lstrcpy(attstr, L"....");
  if (FFInfo->Attributes() & FILE_ATTRIBUTE_ARCHIVE)  attstr[0] = 'A';
  if (FFInfo->Attributes() & FILE_ATTRIBUTE_HIDDEN)   attstr[1] = 'H';
  if (FFInfo->Attributes() & FILE_ATTRIBUTE_READONLY) attstr[2] = 'R';
  if (FFInfo->Attributes() & FILE_ATTRIBUTE_SYSTEM)   attstr[3] = 'S';

/*** And finally print details */

  FFInfo->Filename(s, LEN_FILENAME);
  FFInfo->FileSize(&size_low, &size_high);
  FileTimeToSystemTime(&(FFInfo->LastWriteTime()), &st);

  if (Full)
  { if (FFInfo->Attributes() & FILE_ATTRIBUTE_DIRECTORY)
      wsprintf(Result,
               L"%-16s<DIR>           %2i/%02i/%02i %2i:%02i:%02i  %4s",
               s,
               st.wDay, st.wMonth, st.wYear,
               st.wHour, st.wMinute, st.wSecond,
               attstr
              );
    else
      wsprintf(Result,
               L"%-16s%8li bytes  %2i/%02i/%02i %2i:%02i:%02i  %4s",
               s, size_low,
               st.wDay, st.wMonth, st.wYear,
               st.wHour, st.wMinute, st.wSecond,
               attstr
              );
  }
  else
  { if (FFInfo->Attributes() & FILE_ATTRIBUTE_DIRECTORY)
      wsprintf(Result,
               L"%-16s    <DIR>  %2i/%02i/%02i %2i:%02i:%02i  %4s",
               s,
               st.wDay, st.wMonth, st.wYear,
               st.wHour, st.wMinute, st.wSecond,
               attstr
              );
    else
      wsprintf(Result,
               L"%-16s %8li  %2i/%02i/%02i %2i:%02i:%02i  %4s",
               s, size_low,
               st.wDay, st.wMonth, st.wYear,
               st.wHour, st.wMinute, st.wSecond,
               attstr
              );
  }

// Return indicating success

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


