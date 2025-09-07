//**********************************************************************
// CFileSecObject
// ==============
// Manage security on files
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <aclapi.h>
#include "CSecurableObject.h"
#include "CFileSecObject.h"


//**********************************************************************
// CFileSecObject
// ==============
//**********************************************************************

CFileSecObject::CFileSecObject() : CSecureableObject(TRUE)
{
  m_FileName = NULL;
}


CFileSecObject::CFileSecObject(WCHAR* FileName) : CSecureableObject(TRUE)
{
  if (FileName)
  { m_FileName = (WCHAR*) malloc(lstrlen(FileName)*2 + 1);
    if (m_FileName)
      lstrcpy(m_FileName, FileName);
  }
}


CFileSecObject::~CFileSecObject()
{
  if (m_FileName)
    free(m_FileName);
}


//**********************************************************************
// CFileSecObject::SetObjectSecurity
// =================================
// Set the security info in our internal security descriptor into our
// filename.
//**********************************************************************

BOOL CFileSecObject::SetObjectSecurity(int bInheritACL)
{ DWORD d;

// Check the file name has been set

  if (!m_FileName)
  { m_iSecErrorCode = 0; // Have to think what error to return here
    return(FALSE);
  }

// Set the file security

  SetPrivilegeInAccessToken(TRUE);

  if (bInheritACL > 0)
    d = SetNamedSecurityInfo(m_FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, m_pDACL, NULL);
  else if (bInheritACL < 0)
    d = SetNamedSecurityInfo(m_FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, m_pDACL, NULL);
  else
    d = SetNamedSecurityInfo(m_FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, m_pDACL, NULL);

  SetPrivilegeInAccessToken(FALSE);

  FreeDataStructures();

// If we failed to set the security save the error and return false

  if (d != ERROR_SUCCESS)
  { m_iSecErrorCode = d;
    return(FALSE);
  }

// Return indicating success

  return(TRUE);
};


//**********************************************************************
// CFileSecObject::GetObjectSecurity
// ===================================
// Read the ACLs from the supplied handle and build a security
// descriptor with the ACLs so we can subsequently manipulate them.
//**********************************************************************

BOOL CFileSecObject::GetObjectSecurity(void)
{ PSECURITY_DESCRIPTOR pSD;
  PACL pDACL;
  DWORD d;

// The file name must have been set for this function to work

  if (!m_FileName)
  { m_iSecErrorCode = 0; // Have to think what error to return here
    return(FALSE);
  }

// Get the ACL and SD

  SetPrivilegeInAccessToken(TRUE);

  d = GetNamedSecurityInfo(m_FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDACL, NULL, &pSD);

  SetPrivilegeInAccessToken(FALSE);

  if (d != ERROR_SUCCESS)
  { m_iSecErrorCode = d;
    return(FALSE);
  }

// Build a security descriptor with the ACLs we've just retrieved

  if (!BuildSD(pSD))
  { LocalFree(pSD);
    return(FALSE);
  }

// Return indicating success

  LocalFree(pSD);

  return TRUE;
}


//**********************************************************************
// CFileSecObject::RightsToText
// ----------------------------
// This converts a file access mask to a text representation
// NB the WCHAR* must be at least 10 bytes long.
//**********************************************************************

const WCHAR* CFileSecObject::RightsToText(DWORD Mask, WCHAR* Text)
{ DWORD specific_mask, generic_mask;

  lstrcpy(Text, L"");

// Specific rights first

  specific_mask = Mask & 0x00ffffff;

  if ((specific_mask & FILE_ALL_ACCESS) == FILE_ALL_ACCESS)
  { lstrcpy(Text, L"Full");
  }
  else
  { if ((specific_mask & FILE_GENERIC_READ) == FILE_GENERIC_READ)
      lstrcat(Text, L"R");
    if ((specific_mask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE)
      lstrcat(Text, L"W");
    if ((specific_mask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE)
      lstrcat(Text, L"E");
    if ((specific_mask & DELETE) == DELETE)
      lstrcat(Text, L"D");

    if (lstrlen(Text) == 0)
      lstrcpy(Text, L"-");
  }

// Now generic rights

  generic_mask = Mask & 0xff000000;

  if (generic_mask != 0)
    lstrcat(Text, L",");

  if (generic_mask & GENERIC_ALL)
  { lstrcat(Text, L"Full");
  }
  else
  { if (generic_mask & GENERIC_READ)
      lstrcat(Text, L"R");
    if (generic_mask & GENERIC_WRITE)
      lstrcat(Text, L"W");
    if (generic_mask & GENERIC_EXECUTE)
      lstrcat(Text, L"E");
  }

// Return the string we've built

  return Text;
}


//**********************************************************************
// CFileSecObject::SetFileName
// ===========================
// Set the file name if it wasn't done in the constructor
//**********************************************************************

BOOL CFileSecObject::SetFileName(WCHAR* FileName)
{

  if (m_FileName)
  { free(m_FileName);
    m_FileName = NULL;
  }

  if (FileName)
  { m_FileName = (WCHAR*) malloc(lstrlen(FileName)*2 + 1);
    if (!m_FileName)
    {
      return(FALSE);
    }

    lstrcpy(m_FileName, FileName);
  }

  return(TRUE);
}


//**********************************************************************
// CFileSecObject::GetFileName
// ===========================
// Get the file name
//**********************************************************************

const WCHAR* CFileSecObject::GetFileName(WCHAR* FileName, int Len)
{
  if (!FileName || !m_FileName)
    return(NULL);

  if ((int) lstrlen(m_FileName) > Len)
    return(NULL);

  lstrcpy(FileName, m_FileName);

  return(FileName);
}


