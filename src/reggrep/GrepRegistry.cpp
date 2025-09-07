//**********************************************************************
// GrepRegistry
// ============
// Search and replace strings in the registry
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "regexp.h"


//**********************************************************************
// Structure to hold search info
//**********************************************************************

typedef struct tagGREPINFO
{ regexp* Search;
  char* Replace;

  BOOL Name,
       Value, ReplaceValue,
       IgnoreCase,
       Confirm;

} GREPINFO;


//**********************************************************************
// Prototypes
//**********************************************************************

int GrepRegistry(HKEY hKey, GREPINFO* GrepInfo, char* RegPath);
BOOL GrepExecRegistry(HKEY hKey, char* Name, char* Value, DWORD Type, GREPINFO* GrepInfo, char* RegPath);

const char* GetLastErrorMessage(void);


//**********************************************************************
// Data
//**********************************************************************

#define LEN_DATA 0x1000

#define SYNTAX \
"Syntax: GrepRegistry [-lm -u -cr -cu -n -v[r] -i -q] <search> [<replace>]\n" \
"  -lm search HKEY_LOCAL_MACHINE\n" \
"  -u  search HKEY_USERS\n" \
"  -cr search HKEY_CLASSES_ROOT (included in HKEY_LOCAL_MACHINE)\n" \
"  -cu search HKEY_CURRENT_USER (included in HKEY_USERS)\n" \
"  -n  search the key names\n" \
"  -v  search the key values (default)\n" \
"  -vr replace strings in the key values\n" \
"  -i  search is not case sensitive\n" \
"  -q  do not prompt when replacing\n"


//**********************************************************************
// main
// ====
//**********************************************************************

int main(int argc, char* argv[])
{ int numarg, hits;
  BOOL localmachine, users, classesroot, currentuser;
  GREPINFO grepinfo;
  char s[256];

// Help?

  if (argc == 2)
  { if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "/?") == 0)
    { printf(SYNTAX);
      return(0);
    }
  }

// Check for flag(s)

  localmachine = users = classesroot = currentuser = FALSE;

  grepinfo.Name = grepinfo.Value = grepinfo.ReplaceValue = grepinfo.IgnoreCase = FALSE;
  grepinfo.Confirm = TRUE;

  for (numarg = 1; numarg < argc; numarg++)
  { if (argv[numarg][0] != '-')
      break;

    if (strcmp(argv[numarg] + 1, "lm") == 0)
    { localmachine = TRUE;
    }
    else if (strcmp(argv[numarg] + 1, "u") == 0)
    { users = TRUE;
    }
    else if (strcmp(argv[numarg] + 1, "cr") == 0)
    { classesroot = TRUE;
    }
    else if (strcmp(argv[numarg] + 1, "cu") == 0)
    { currentuser = TRUE;
    }
    else if (argv[numarg][1] == 'n')
    { grepinfo.Name = TRUE;
    }
    else if (argv[numarg][1] == 'v')
    { grepinfo.Value = TRUE;
      if (argv[numarg][2] == 'r')
        grepinfo.ReplaceValue = TRUE;
    }
    else if (strcmp(argv[numarg] + 1, "i") == 0)
    { grepinfo.IgnoreCase = TRUE;
    }
    else if (strcmp(argv[numarg] + 1, "q") == 0)
    { grepinfo.Confirm = FALSE;
    }
    else
    { printf("Unknown flag: %s\n", argv[numarg]);
      return(2);
    }
  }

  if (!(localmachine || users || classesroot || currentuser))
    currentuser = TRUE;

  if (!grepinfo.Name && !grepinfo.Value)
    grepinfo.Value = TRUE;

// Check required arguments are present

  if (argc - numarg < 1)
  { printf("A search string must be supplied\n");
    printf(SYNTAX);
    return(2);
  }

  if (grepinfo.ReplaceValue)
  { if (argc - numarg < 2)
    { printf("If -vr is specified a replacement string must be supplied\n");
      printf(SYNTAX);
      return(2);
    }
  }

// Process the regular expression

  strcpy(s, argv[numarg]);
  if (grepinfo.IgnoreCase)
    AnsiLower(s);

  grepinfo.Search = regcomp(s);

  if (!grepinfo.Search)
  { printf("Error compiling the regular expression %s\n", argv[1]);
    return(2);
  }

// Get the replacement string

  if (grepinfo.ReplaceValue)
    grepinfo.Replace = argv[numarg+1];
  else
    grepinfo.Replace = NULL;

// Describe what we're going to do

  if (grepinfo.Name && !grepinfo.Value)
    strcpy(s, "in the key name only");
  else if (!grepinfo.Name && grepinfo.Value)
    strcpy(s, "in the key value only");
  else
    strcpy(s, "in the key name and value");

  if (grepinfo.ReplaceValue)
    printf("Searching for \"%s\" %s and replacing with \"%s\" in the key value\n", argv[numarg], s, argv[numarg+1]);
  else
    printf("Searching for \"%s\" %s\n", argv[numarg], s);

  printf("\n");

// Grep the registries

  if (localmachine)
  { printf("HKEY_LOCAL_MACHINE\n");
    hits = GrepRegistry(HKEY_LOCAL_MACHINE, &grepinfo, "");
    printf("HKEY_LOCAL_MACHINE: %i matches\n", hits);
  }

  if (users)
  { printf("HKEY_USERS\n");
    hits = GrepRegistry(HKEY_USERS, &grepinfo, "");
    printf("HKEY_USERS: %i matches\n", hits);
  }

  if (!localmachine && classesroot)
  { printf("HKEY_CLASSES_ROOT\n");
    hits = GrepRegistry(HKEY_CLASSES_ROOT, &grepinfo, "");
    printf("HKEY_CLASSES_ROOT: %i matches\n", hits);
  }

  if (!users && currentuser)
  { printf("HKEY_CURRENT_USER\n");
    hits = GrepRegistry(HKEY_CURRENT_USER, &grepinfo, "");
    printf("HKEY_CURRENT_USER: %i matches\n", hits);
  }

// All done

  return(0);
}


//**********************************************************************
// GrepRegistry
// ============
// Recursively search a registry key
//   hKey     - the current key
//   GrepInfo - the search parameters
//   RegPath  - the current key path
//**********************************************************************

int GrepRegistry(HKEY hKey, GREPINFO* GrepInfo, char* RegPath)
{ int numvalues = 0;
  DWORD index, type, namelen, datalen, classlen;
  HKEY hsubkey;
  FILETIME ft;
  char name[256], classname[256], regpath[1024];
  BYTE data[LEN_DATA+1];

// First enumerate subkeys.  This is to make key renames more efficient.

  for (index = 0; ; index++)
  { namelen = classlen = 255;

    if (RegEnumKeyEx(hKey, index, name, &namelen, NULL, classname, &classlen, &ft) != ERROR_SUCCESS)
      break;

    if (RegOpenKeyEx(hKey, name, 0, KEY_ALL_ACCESS, &hsubkey) != ERROR_SUCCESS)
      if (RegOpenKeyEx(hKey, name, 0, KEY_READ, &hsubkey) != ERROR_SUCCESS)
        continue;

    strcpy(regpath, RegPath);
    strcat(regpath, "\\");
    strcat(regpath, name);
    numvalues += GrepRegistry(hsubkey, GrepInfo, regpath);
    RegCloseKey(hsubkey);
  }

// Now enumerate values

  for (index = 0; ; index++)
  { namelen = 255;
    datalen = LEN_DATA;

    if (RegEnumValue(hKey, index, name, &namelen, NULL, &type, data, &datalen) != ERROR_SUCCESS)
      break;

    switch (type)
    { case REG_BINARY:
      case REG_DWORD:
      case REG_DWORD_BIG_ENDIAN:
      case REG_LINK:
      case REG_NONE:
      case REG_RESOURCE_LIST:
        break;

      case REG_MULTI_SZ:
        // Have to think what to do here
        break;

      case REG_EXPAND_SZ:
      case REG_SZ:
        if (GrepExecRegistry(hKey, name, (char*) data, type, GrepInfo, RegPath))
          numvalues++;
        break;
    }
  }

// All done

  return(numvalues);
}


//**********************************************************************
// GrepExecRegistry
// ================
// Search an individual key/value
// The function will replace text in the value though not in the name.
//**********************************************************************

BOOL GrepExecRegistry(HKEY hKey, char* Name, char* Value, DWORD Type, GREPINFO* GrepInfo, char* RegPath)
{ int len;
  DWORD d;
  BOOL matchname = FALSE, matchvalue = FALSE, doreplace;
  char* p;
  char ivalue[LEN_DATA+1], s[LEN_DATA+1], t[8];

// Check the value to see if we have a match.  For case insensitive searches
// lowercase the search string.

  if (GrepInfo->Value)
  { if (GrepInfo->IgnoreCase)
    { strcpy(ivalue, Value);
      AnsiLower(ivalue);
	  p = ivalue;
    }
    else
    { p = Value;
    }

    matchvalue = regexec(GrepInfo->Search, p);

    if (GrepInfo->IgnoreCase)
      strcpy(ivalue, Value);

// If we have a match print it

    if (matchvalue)
    { printf("%s\\%s: %s\n", RegPath, Name, Value);

// Now handle any replacements

      if (GrepInfo->ReplaceValue)
      {

// Build the replacement string

        len = GrepInfo->Search->startp[0] - p;
        strncpy(s, Value, len);
        s[len] = '\0';

        regsub(GrepInfo->Search, GrepInfo->Replace, s + len);

        strcat(s, Value + (GrepInfo->Search->endp[0] - p));

// Check if confirmation is required

        doreplace = TRUE;

        if (GrepInfo->Confirm)
        { doreplace = FALSE;

          printf("Change to \"%s\" (y/n/a) ?: ", s);
          gets(t);
          AnsiLower(t);

          if (t[0] == 'y' || t[0] == 'a')
          { doreplace = TRUE;

            if (t[0] == 'a')
              GrepInfo->Confirm = FALSE;
          }
        }

// Do the replacement

        if (doreplace)
        { if ((d = RegSetValueEx(hKey, Name, 0, Type, (BYTE*) s, strlen(s) + 1)) != ERROR_SUCCESS)
          { strcpy(s, "<unknown error>");
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, s, LEN_DATA, NULL);
            printf("Failed with error: %s\n", s);
          }
          else
          { printf("Changed to: %s\n", s);
          }
        }
      }
    }
  }

// Check the name to see if we have a match; do this only if there was not
// a match in the value.  Names are not replaced.  For case insensitive searches
// lowercase the value into the string s.

  if (GrepInfo->Name && !matchvalue)
  { if (GrepInfo->IgnoreCase)
    { strcpy(ivalue, Name);
      AnsiLower(ivalue);
      p = ivalue;
    }
    else
    { p = Name;
    }

    matchname = regexec(GrepInfo->Search, p);

    if (matchname)
      if (!matchvalue)
        printf("%s\\%s\n", RegPath, Name);
   }

// Return indicating whether there was a match

  if (!matchname && !matchvalue)
    return(FALSE);
  else
    return(TRUE);
}


//**********************************************************************
// regerror
// ========
// Error reporting function for regexp code
//**********************************************************************

void regerror(const char* s)
{
  printf("Regular expression error: %s\n", s);
}


//**********************************************************************
// GetLastErrorMessage
// ===================
// Get a Win32 error string for the most recent error
//**********************************************************************

const char* GetLastErrorMessage(void)
{ DWORD d;
  static char errmsg[512];

  d = GetLastError();

  strcpy(errmsg, "<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 511, NULL);
  return(errmsg);
}


