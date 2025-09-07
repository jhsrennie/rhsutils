//**********************************************************************
// dirshowacl
// ==========
//
// Show the ACLs for a chosen directory and all subdirectories
//
// You will need the CSecurableObject and CRhsFindFile classes to
// compile this. They can be downloaded from www.winsrc.freeserve.co.uk.
//
// J. Rennie
// 26th August 2001
//**********************************************************************

#include <windows.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <CSecurableObject/CFileSecObject.h>
#include <Misc/CRhsFindFile.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);
BOOL dirshowacl(const WCHAR* Directory);


//**********************************************************************
// Structure to help with handling ACEs
// ------------------------------------
//**********************************************************************

#define MAX_USERNAME 32

#define RHSACE_NCINHERIT      0x01 // Child objects that are not containers inherit permissions
#define RHSACE_INHERIT_PASS   0x02 // Child objects inherit and pass on permissions
#define RHSACE_INHERIT_NOPASS 0x04 // Child objects inherit but do not pass on permissions
#define RHSACE_NOINHERIT_PASS 0x08 // Object is not affected by but passes on permissions
#define RHSACE_INHERITED      0x10 // Permissions have been inherited

typedef struct _RhsACE
{
  WCHAR domain[MAX_USERNAME+1],   // This identifies the user
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

BOOL IncludeInherited;

#define MAX_EXCLUDE 16
WCHAR Exclude[MAX_EXCLUDE][MAX_USERNAME+1];
int  NumExclude;

#define SYNTAX L"dirshowacl [-i -x<username>] <directoryname>\r\n"


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"dirshowacl"))
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
  DWORD attr;
  CRhsFindFile ff;

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

// Check the argument is a directory

  attr = GetFileAttributes(RhsIO.m_argv[argnum]);

  if (attr == 0xffffffff)
  { RhsIO.printf(L"Cannot find the directory \"%s\"\r\n", RhsIO.m_argv[argnum]);
    return 2;
  }

  if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
  { RhsIO.printf(L"\"%s\" is not a directory\r\n", RhsIO.m_argv[argnum]);
    return 2;
  }

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

  dirshowacl(RhsIO.m_argv[argnum]);

// All done

  return 0;
}


//**********************************************************************
// dirshowacl
// ----------
//**********************************************************************

BOOL dirshowacl(const WCHAR* Directory)
{ int num_entries, num_aces, i, j;
  BOOL found_ace;
  DWORD mask, type, flags;
  CRhsFindFile ff;
  CFileSecObject sec;
  RhsACE ace[32];
  WCHAR username[256], domain[256], rights[16];
  WCHAR found[MAX_PATH+1], s[MAX_PATH+1];
  WCHAR outstr[0x1000];

  swprintf(outstr, 0x1000, L"%s\r\n", Directory);
  found_ace = FALSE;

// Attach the CSecurableObject to this directory

  sec.SetFileName((WCHAR*) Directory);
  sec.GetObjectSecurity();

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
    { lstrcpy(ace[num_aces].domain, domain);
      lstrcpy(ace[num_aces].username, username);
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
      swprintf(username, 0x1000, L"%s\\%s", ace[i].domain, ace[i].username);
    CharLower(username);

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

  lstrcpy(s, Directory);
  lstrcat(s, L"\\*");

  if (ff.First(s, found, MAX_PATH))
  { do
    {

// Ignore ordinary files

      if (!(ff.Attributes() & FILE_ATTRIBUTE_DIRECTORY))
        continue;

// Ignore . and ..

      ff.Filename(s, MAX_PATH);

      if (lstrcmp(s, L".") == 0 || lstrcmp(s, L"..") == 0)
        continue;

// Print the ACL for this directory and subdirectories

      dirshowacl(found);

    } while (ff.Next(found, MAX_PATH));
  }

  ff.Close();

// All done

  return TRUE;
}


