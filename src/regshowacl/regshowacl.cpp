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
#include <CSecurableObject/CRegSecObject.h>


//**********************************************************************
// Function prototypes
// -------------------
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);

BOOL regshowacl(const WCHAR* RegKey);
HKEY OpenKey(const WCHAR* RegKey);
BOOL GetSubKey(HKEY hKey, int KeyNum, const WCHAR* ParentName, WCHAR* SubKeyName);

const WCHAR* GetLastErrorMessage(void);
const WCHAR* GetLastErrorMessage(DWORD d);


//**********************************************************************
// Structure to help with handling ACEs
// ------------------------------------
//**********************************************************************

#define MAX_USERNAME 64

#define RHSACE_NCINHERIT      0x01 // Child objects that are not containers inherit permissions
#define RHSACE_INHERIT_PASS   0x02 // Child objects inherit and pass on permissions
#define RHSACE_INHERIT_NOPASS 0x04 // Child objects inherit but do not pass on permissions
#define RHSACE_NOINHERIT_PASS 0x08 // Object is not affected by but passes on permissions
#define RHSACE_INHERITED      0x10 // Permissions have been inherited

typedef struct _RhsACE
{
  WCHAR  domain[MAX_USERNAME+1],   // This identifies the user
         username[MAX_USERNAME+1];

  DWORD mask, // access mask
        type, // 0 = access allowed, 1 = access denied
        flags;

} RhsACE;


//**********************************************************************
// Global variables
// ----------------
//**********************************************************************

CRhsIO RhsIO;

#define SYNTAX L"regshowacl [-i -x<username>] <keyname>\r\n" \
               L"NB the key must start with HKEY_LOCAL_MACHINE etc\r\n" \
               L"e.g. HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\r\n" \
               L"(the case of the key name doesn't matter)\r\n"

BOOL IncludeInherited;

#define MAX_EXCLUDE 16
WCHAR Exclude[MAX_EXCLUDE][MAX_USERNAME+1];
int  NumExclude;


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"example"))
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
  WCHAR keyname[1024];

// Process command line flags

  IncludeInherited = FALSE;
  NumExclude = 0;
  argnum = 1;

  while (argnum < RhsIO.m_argc)
  { if (RhsIO.m_argv[argnum][0] != '-')
      break;

    CharLower(RhsIO.m_argv[argnum]+1);

    switch (RhsIO.m_argv[argnum][1])
    { case '?':
        RhsIO.printf(SYNTAX);
        return 0;

      case 'i': // Include inherited permissions
        IncludeInherited = TRUE;
        break;

      case 'x': // Exclude a username
        if (NumExclude <= MAX_EXCLUDE)
        { lstrcpy(Exclude[NumExclude], RhsIO.m_argv[argnum]+2);
          NumExclude++;
        }
        break;

      default:
        RhsIO.printf(L"Unknown flag \"%s\".\r\n%s", RhsIO.m_argv[argnum], SYNTAX);
        return(1);
    }

    argnum++;
  }

// Check there is one argument left

  if (RhsIO.m_argc - argnum < 1)
  { RhsIO.printf(SYNTAX);
    return(2);
  }

// Split the hive off the key

  for (i = 0; RhsIO.m_argv[argnum][i] != '\\' && RhsIO.m_argv[argnum][i] != '\0'; i++)
    keyname[i] = RhsIO.m_argv[argnum][i];
  keyname[i] = '\0';

  if (RhsIO.m_argv[argnum][i] == '\\')
    i++;

  if (lstrcmpi(keyname, L"HKEY_LOCAL_MACHINE") == 0)
  { lstrcpy(keyname, L"MACHINE");
  }
  else if (lstrcmpi(keyname, L"HKEY_CLASSES_ROOT") == 0)
  { lstrcpy(keyname, L"CLASSES_ROOT");
  }
  else if (lstrcmpi(keyname, L"HKEY_USERS") == 0)
  { lstrcpy(keyname, L"USERS");
  }
  else if (lstrcmpi(keyname, L"HKEY_CURRENT_USER") == 0)
  { lstrcpy(keyname, L"CURRENT_USER");
  }
  else
  { RhsIO.errprintf(L"Unrecognised hive \"%s\"\r\n%s\r\n", keyname, SYNTAX);
    return 2;
  }

  lstrcat(keyname, L"\\");
  lstrcat(keyname, RhsIO.m_argv[argnum] + i);

// Show the security for the directory and subdirectories

  if (IncludeInherited)
    RhsIO.printf(L"Listing both inherited and non-inherited permissions for \"%s\"\r\n", RhsIO.m_argv[argnum]);
  else
    RhsIO.printf(L"Listing only non-inherited permissions for \"%s\"\r\n", RhsIO.m_argv[argnum]);

  if (NumExclude > 0)
  { RhsIO.printf(L"Excluding users: %s\r\n", Exclude[0]);
    for (i = 1; i < NumExclude; i++)
      RhsIO.printf(L"                 %s\r\n", Exclude[i]);
  }

  RhsIO.printf(L"");

  regshowacl(keyname);

// All done

  return 0;
}


//**********************************************************************
// regshowacl
// ----------
//**********************************************************************

BOOL regshowacl(const WCHAR* RegKey)
{ int num_entries, num_aces, i, j;
  BOOL found_ace;
  DWORD mask, type, flags;
  HKEY hkey;
  CRegSecObject sec;
  RhsACE ace[32];
  WCHAR username[256], domain[256], rights[16];
  WCHAR subkey[1024];
  WCHAR outstr[0x1000];

  swprintf(outstr, 0x1000, L"%s\r\n", RegKey);
  found_ace = FALSE;

// Attach the CSecurableObject to this directory

  sec.SetKeyName((WCHAR*) RegKey);
  if (!sec.GetObjectSecurity())
  { RhsIO.errprintf(L"Cannot open the key %s: %s\r\n", RegKey, GetLastErrorMessage(sec.m_iSecErrorCode));
    return FALSE;
  }

// Loop over all ACEs in the list

  num_entries = sec.GetNumEntries();
  num_aces = 0;

  for (i = 0; i < num_entries; i++)
  { sec.GetRightsFromACE(username, domain, &mask, &type, &flags, i);

// Look to see if this is a new user

    for (j = 0; j < num_aces; j++)
      if (lstrcmp(username, ace[j].username) == 0 && type == ace[j].type)
        break;

// If this is a new user get all their entries

    if (j >= num_aces)
    { lstrcpyn(ace[num_aces].domain, domain, MAX_USERNAME);
      lstrcpyn(ace[num_aces].username, username, MAX_USERNAME);
      ace[num_aces].type = type;
      ace[num_aces].flags = flags;

// This gets all entries in the ACL matchine the current SID and type
      sec.GetAllRightsFor(&(ace[num_aces].mask), i);

      num_aces++;
    }
  }

// Got all the entries so print them

  for (i = 0; i < num_aces; i++)
  {

// We only want non-inherited permissions unless we've specifically
// asked for them.

    if (ace[i].flags & RHSACE_INHERITED)
      if (!IncludeInherited)
        continue;

// For tidiness omit the standard "domains" BUILTIN and NT AUTHORITY

    if (lstrlen(ace[i].domain) == 0 || lstrcmp(ace[i].domain, L"BUILTIN") == 0 || lstrcmp(ace[i].domain, L"NT AUTHORITY") == 0)
      lstrcpy(username, ace[i].username);
    else
      swprintf(username, 256, L"%s\\%s", ace[i].domain, ace[i].username);
    CharLower(username);
RhsIO.printf(L"DEBUG: %s\r\n", ace[i].username);

// Check if we're excluding this username

    for (j = 0; j < NumExclude; j++)
    { if (lstrcmp(Exclude[j], username) == 0)
        break;
    }

    if (j < NumExclude)
      continue;

// Print the access including whether it's an allowed or denied ACE

    found_ace = TRUE;

    sec.RightsToText(ace[i].mask, rights);
    if (ace[i].flags & RHSACE_INHERITED)
      lstrcat(rights, L"(I)");

    if (ace[i].type == 0)
      swprintf(outstr + lstrlen(outstr), 0x1000, L"\t%s\t%s\r\n", username, rights);
    else
      swprintf(outstr + lstrlen(outstr), 0x1000, L"\t%s\tDenied: %s\r\n", username, rights);
  }

// Only print the output if we found a non-inherited ACL

  if (found_ace)
    RhsIO.printf(outstr);

// Now work through subdirectories

  hkey = OpenKey(RegKey);

  if (hkey == INVALID_HANDLE_VALUE)
  { RhsIO.errprintf(L"Cannot open subkeys of %s\r\n", RegKey, GetLastErrorMessage());
    return 2;
  }

  i = 0;

  while (GetSubKey(hkey, i, RegKey, subkey))
  { i++;

    regshowacl(subkey);
  }

  CloseHandle(hkey);

// All done

  return TRUE;
}


//**********************************************************************
// OpenKey
// -------
// Open the named key.
// The name is in the format used by the GetNamedSecurityInfo function
// e.g. "MACHINE\keyname"
//**********************************************************************

HKEY OpenKey(const WCHAR* RegKey)
{ int i;
  LONG d;
  HKEY hivekey, h;
  WCHAR hive[256], keyname[1024];

// Split the hive off the key

  for (i = 0; RegKey[i] != '\\' && RegKey[i] != '\0'; i++)
    hive[i] = RegKey[i];
  hive[i] = '\0';

  if (RegKey[i] == '\\')
    i++;

  lstrcpy(keyname, RegKey + i);

  if (lstrcmpi(hive, L"MACHINE") == 0)
  { hivekey = HKEY_LOCAL_MACHINE;
  }
  else if (lstrcmpi(hive, L"CLASSES_ROOT") == 0)
  { hivekey = HKEY_CLASSES_ROOT;
  }
  else if (lstrcmpi(hive, L"USERS") == 0)
  { hivekey = HKEY_USERS;
  }
  else if (lstrcmpi(hive, L"CURRENT_USER") == 0)
  { hivekey = HKEY_CURRENT_USER;
  }
  else
  { RhsIO.errprintf(L"Unrecognised hive \"%s\"\r\n", hive);
    return (HKEY) INVALID_HANDLE_VALUE;
  }

// Open the key

  d = RegOpenKeyEx(hivekey, keyname, 0, KEY_READ, &h);

  if (d != ERROR_SUCCESS)
    h = (HKEY) INVALID_HANDLE_VALUE;

// Return the key

  return h;
}


//**********************************************************************
// GeySubKey
// ---------
// Get the subkey by number. Return the subkey name in the form used by
// the GetNamedSecurityInfo function e.g. "MACHINE\keyname"
//**********************************************************************

BOOL GetSubKey(HKEY hKey, int KeyNum, const WCHAR* ParentName, WCHAR* SubKeyName)
{ DWORD subkeysize, d;
  FILETIME f;
  WCHAR subkey[256];

// Get the subkey

  subkeysize = 255;
  d = RegEnumKeyEx(hKey, KeyNum, subkey, &subkeysize, NULL, NULL, NULL, &f);

// If there were no more subkeys just return FALSE

  if (d == ERROR_NO_MORE_ITEMS)
    return FALSE;

// If some other error occurred print the error message

  if (d != ERROR_SUCCESS)
  { RhsIO.errprintf(L"Error getting subkey %i of %s\r\n%s\r\n", KeyNum, ParentName, GetLastErrorMessage());
    return FALSE;
  }

// Otherwise set the full subkey name

  lstrcpy(SubKeyName, ParentName);
  lstrcat(SubKeyName, L"\\");
  lstrcat(SubKeyName, subkey);

// And return indicating success

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


const WCHAR* GetLastErrorMessage(DWORD d)
{ static WCHAR errmsg[1024];

  lstrcpy(errmsg, L"<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 1023, NULL);
  return(errmsg);
}


