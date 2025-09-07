// *********************************************************************
// findtext
// ========
// v1.2 04/08/2018
//
// v1.2 adds option for the number of hits required
//
// v1.1 allows multiple alternative search expressions seperated by
// colons.
// *********************************************************************

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <CRhsIO/CRhsIO.h>
#include <Misc/CRhsFindFile.h>
#include <Misc/CRhsDate.h>
#include <Misc/Utils.h>
#include "CSearchExpr.h"
#include "CTextBuffer.h"


//**********************************************************************
// FTINFO
// ------
// Structure for holding details of the text search
//**********************************************************************

#define MAX_SEARCHEXPR 8

typedef struct
{
  WORD attrib;

  BOOL subdir,
       ignorecase;

  int  needhits;

  FILETIME searchdate;
  BOOL     usesearchdate;

  int numfiles;

  int num_searchexpr;
  int num_hits[MAX_SEARCHEXPR];
  CSearchExpr searchexpr[MAX_SEARCHEXPR];

  CTextBuffer textbuf;

} FTINFO;

typedef FTINFO FAR * LPFTINFO;

#define MAX_HITS   64


//**********************************************************************
// Prototypes
// ----------
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);
BOOL FindText(WCHAR* Filename, FTINFO* FTInfo);
BOOL FindTextSub(WCHAR* FileName, FTINFO* FTInfo);


//**********************************************************************
// Global variables
// ----------------
//**********************************************************************

CRhsIO RhsIO;

#define SYNTAX \
L"findtext v1.2.2\r\n" \
L"Syntax: [-d -h -i -m<cutoff date> -n<num hits> -s] <search string> <file name(s)>\r\n" \
L"Flags:\r\n" \
L"  -d recurse into subdirectories\r\n" \
L"  -h include hidden files\r\n" \
L"  -i case insensitive search\r\n" \
L"  -m<cutoff date> include only files modified after this date\r\n" \
L"  -n<num hits> minimum number of hits required\r\n" \
L"  -s include system files\r\n"

// Maximum length of input line
// Lines longer than 1000 characters don't display well so we put the
// limite at 1000 characters.

#define MAX_INPUTLINELEN 1000


//**********************************************************************
// Main
// ----
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"filefindtext"))
  { wprintf(L"Error initialising: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  retcode = RhsIO.Run(rhsmain);

  return retcode;
}


//**********************************************************************
// Here we go
// ----------
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused)
{ int numarg, i, j;
  WCHAR target[LEN_FILENAME];
  DWORD attrib;
  FTINFO ftinfo;
  CRhsDate dt;
  SYSTEMTIME st;
  WCHAR searchexpr[LEN_SEARCHTEXT];
  char a_searchexpr[LEN_SEARCHTEXT];

// Check the arguments

  if (RhsIO.m_argc == 2)
  { if (lstrcmp(RhsIO.m_argv[1], L"-?") == 0 || lstrcmp(RhsIO.m_argv[1], L"/?") == 0)
    { RhsIO.printf(SYNTAX);
      return 0;
    }
  }

// Process arguments

  ftinfo.attrib        = 0;
  ftinfo.subdir        = FALSE;
  ftinfo.ignorecase    = FALSE;
  ftinfo.needhits      = 1;
  ftinfo.usesearchdate = 0;

  for (numarg = 1; numarg < RhsIO.m_argc; numarg++)
  { if (RhsIO.m_argv[numarg][0] != '-')
      break;

    switch (RhsIO.m_argv[numarg][1])
    { case 'd':
      case 'D':
        ftinfo.subdir = TRUE;
        break;

      case 'h':
      case 'H':
        ftinfo.attrib |= FILE_ATTRIBUTE_HIDDEN;
        break;

      case 'i':
      case 'I':
        ftinfo.ignorecase = TRUE;
        break;

      case 'm':
      case 'M':
        ftinfo.usesearchdate = TRUE;

        if (!dt.SetDate(RhsIO.m_argv[numarg]+2))
        { RhsIO.errprintf(L"The date supplied, \"%s\", is not valid.\r\n", RhsIO.m_argv[numarg]+2);
          return 2;
        }

        dt.GetDate(&st);
        SystemTimeToFileTime(&st, &ftinfo.searchdate);
        break;

      case 'n':
      case 'N':
        if ((ftinfo.needhits = _wtoi(RhsIO.m_argv[numarg]+2)) < 1)
        { RhsIO.errprintf(L"The number of hits required, \"%s\", must be an integer greater than zero.\r\n", RhsIO.m_argv[numarg]+2);
          return 2;
        }
        break;

      case 's':
      case 'S':
        ftinfo.attrib |= FILE_ATTRIBUTE_SYSTEM;
        break;

      default:
        RhsIO.errprintf(L"Unknown flag: %s\r\n", RhsIO.m_argv[numarg]);
        return 2;
    }
  }

// There must be at exactly two arguments left

  if (RhsIO.m_argc - numarg != 2)
  { RhsIO.errprintf(SYNTAX);
    return 2;
  }

// Process the search input. This is a string of the form:
//   "xxx:yyy:zzz"
// that is a number of substrings separated by colons.
// For each of the substrings a CSearchExpr is generated.

  // The search expression needs to be ASCII
  lstrcpyn(searchexpr, RhsIO.m_argv[numarg], LEN_SEARCHTEXT);
  WideCharToMultiByte(CP_ACP, 0, searchexpr, -1, a_searchexpr, LEN_SEARCHTEXT, NULL, 0);

  ftinfo.num_searchexpr = 0;

  // Keep going until we hit the end of the input
  for (i = 0; a_searchexpr[i] != '\0'; )
  {
    // Find the terminating colon (or end of input)
    for (j = i; a_searchexpr[j] != ':' && a_searchexpr[j] != '\0'; j++);

    if (a_searchexpr[j] == ':')
      a_searchexpr[j++] = '\0';

    // If we've found a non-zero length substring initialise the CSearchExpr
    if (a_searchexpr[i] != '\0')
    {
      if (ftinfo.num_searchexpr >= MAX_SEARCHEXPR)
      {
        RhsIO.errprintf(L"Maximum number of search expressions %i was reached\r\n", MAX_SEARCHEXPR);
        break;
      }

      if (ftinfo.searchexpr[ftinfo.num_searchexpr].ParseExpr(a_searchexpr+i))
        ftinfo.num_searchexpr++;
    }

    i = j;
  }

// We must have at least one search expression

  if (ftinfo.num_searchexpr == 0)
  {
    RhsIO.errprintf(L"No search expression was supplied\r\n", RhsIO.m_argv[numarg]);
    return 2;
  }

// Process the file name

  lstrcpyn(target, RhsIO.m_argv[numarg+1], LEN_FILENAME);
  UtilsStripTrailingSlash(target);
  UtilsAddCurPath(target, LEN_FILENAME);

  attrib = GetFileAttributes(target);

  if (attrib != 0xFFFFFFFF)
    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
      lstrcat(target, L"\\*");

// Check the path is valid

  if (!UtilsIsValidPath(target))
  { RhsIO.errprintf(L"The path %s is not valid.\r\n", target);
    return 2;
  }

// Print the search expressions

  RhsIO.printf(L"Searching file(s) %s for:\r\n", target);

  for (i = 0; i < ftinfo.num_searchexpr; i++)
  {
    if (i > 0)
     RhsIO.printf(L"and:\r\n");

    for (j = 0; j < ftinfo.searchexpr[i].num_expr; j++)
    { MultiByteToWideChar(CP_ACP, 0, ftinfo.searchexpr[i].text[j], -1, searchexpr, LEN_SEARCHTEXT);
      RhsIO.printf(L"  %s\r\n", searchexpr);
    }
  }

  RhsIO.printf(L"\r\n");

// And find the text

  ftinfo.numfiles = 0;

  FindText(target, &ftinfo);

  RhsIO.printf(L"%i files searched\r\n", ftinfo.numfiles);

// All done

  return 0;
}


//**********************************************************************
// FindText
// --------
//**********************************************************************

BOOL FindText(WCHAR* Filename, FTINFO* FTInfo)
{ CRhsFindFile ff;
  FILETIME ft;
  WCHAR s[LEN_FILENAME];

// First find all subdirectories and recurse into them

  if (FTInfo->subdir)
  { UtilsGetPathFromFilename(Filename, s, LEN_FILENAME);
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

// Append the target file name to the directory we've found and recurse
// into it.

          ff.FullFilename(s, LEN_FILENAME);
          lstrcat(s, L"\\");
          UtilsGetNameFromFilename(Filename, s + lstrlen(s), LEN_FILENAME - lstrlen(s));

          if (!FindText(s, FTInfo))
            return FALSE;
        }

// Find next file

      } while (ff.Next(s, LEN_FILENAME));
    }

    ff.Close();
  }

// Now we've searched all subdirectories, so search this directory

  if (ff.First(Filename, Filename, LEN_FILENAME))
  { do
    {

// Check for an abort signal

      if (RhsIO.GetAbort())
        break;

// Ignore all directories

      if (ff.Attributes() & FILE_ATTRIBUTE_DIRECTORY)
        continue;

// Search hidden and/or system files only if -h and/or -s were specified

      if (ff.Attributes() & FILE_ATTRIBUTE_HIDDEN)
        if (!(FTInfo->attrib  & FILE_ATTRIBUTE_HIDDEN))
          continue;

      if (ff.Attributes() & FILE_ATTRIBUTE_SYSTEM)
        if (!(FTInfo->attrib  & FILE_ATTRIBUTE_SYSTEM))
          continue;

// If we are using a search date check the file timestamp

      if (FTInfo->usesearchdate)
      { ft = ff.LastWriteTime();

        if (CompareFileTime(&(FTInfo->searchdate), &ft) >= 0)
          continue;
      }

/// Search this file

      ff.FullFilename(s, LEN_FILENAME);

      FindTextSub(s, FTInfo);

      FTInfo->numfiles++;

// Find next file

    } while (ff.Next(s, LEN_FILENAME));
  }

  ff.Close();

// All done so return indicating success

  return TRUE;
}


//**********************************************************************
// FindTextSub
// -----------
// ASCII text search.
// Search the file line by line.
//**********************************************************************

BOOL FindTextSub(WCHAR* FileName, FTINFO* FTInfo)
{ int line, num_hits, i;
  BOOL header_printed, hit_all;
  FILE *wf;
  char* nonspace;
  const char* textbufline;
  char ft[MAX_INPUTLINELEN];
  WCHAR s[MAX_INPUTLINELEN];

// Open the file

  if (_wfopen_s(&wf, FileName, L"rb") != 0)
  { RhsIO.errprintf(L"Cannot open the file %s: %s\r\n", FileName, RhsIO.LastError(s, 256));
    return FALSE;
  }

// Search the file

  line = 0;
  num_hits = 0;
  header_printed = FALSE;

  for (i = 0; i < FTInfo->num_searchexpr; i++)
    FTInfo->num_hits[i] = 0;

  while (fgets(ft, MAX_INPUTLINELEN, wf))
  {
    line++;
    UtilsStripTrailingCRLF(ft);

// Check all the search expressions in turn

    for (i = 0; i < FTInfo->num_searchexpr; i++)
    {
      if (FTInfo->searchexpr[i].FindText(ft, FTInfo->ignorecase))
      {

// If we have a hit increment the hit counter just for this expression

        FTInfo->num_hits[i]++;

// Print the matching line to the text buffer. If we match all the search
// expressions we'll print the buffer to stdout.

        if (!header_printed)
        {
          FTInfo->textbuf.Initialise();

          char filename[MAX_PATH+1];
          WideCharToMultiByte(CP_ACP, 0, FileName, -1, filename, MAX_PATH, NULL, NULL);
          FTInfo->textbuf.printf("%s\r\n", filename);
          header_printed = TRUE;
        }

        for (nonspace = ft; *nonspace == ' ' || *nonspace == '\t'; nonspace++);
        FTInfo->textbuf.printf("Line %i: %s\r\n", line, nonspace);
      }
    }
  }

  fclose(wf);

// See if matched all the search expressions

  num_hits = 0;
  hit_all = TRUE;

  for (i = 0; i < FTInfo->num_searchexpr; i++)
  {
    num_hits += FTInfo->num_hits[i];

    if (FTInfo->num_hits[i] == 0)
    {
      hit_all = FALSE;
      break;
    }
  }

  if (hit_all && num_hits >= FTInfo->needhits)
  {
    FTInfo->textbuf.Readback();

    while (textbufline = FTInfo->textbuf.gets())
    {
      // Make a local copy because the line could be longer than MAX_INPUTLINELEN
      lstrcpynA(ft, textbufline, MAX_INPUTLINELEN);
      UtilsStripTrailingCRLF(ft);

      MultiByteToWideChar(CP_ACP, 0, ft, -1, s, MAX_INPUTLINELEN);
      RhsIO.printf(L"%s\r\n", s);
    }
    RhsIO.printf(L"\r\n");
  }

// Return indicating success

  return TRUE;
}


