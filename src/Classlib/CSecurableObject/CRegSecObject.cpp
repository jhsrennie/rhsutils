//**********************************************************************
// CRegSecObject
// ==============
// Manage security on registry keys
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <aclapi.h>
#include <string.h>
#include "CSecurableObject.h"
#include "CRegSecObject.h"


//**********************************************************************
// CRegSecObject
// ==============
//**********************************************************************

CRegSecObject::CRegSecObject() : CSecureableObject(TRUE)
{
  m_KeyName = NULL;
}


CRegSecObject::CRegSecObject(WCHAR* KeyName) : CSecureableObject(TRUE)
{
  m_KeyName = NULL;

  if (KeyName)
  { m_KeyName = (WCHAR*) malloc(lstrlen(KeyName)*2 + 1);
    if (m_KeyName)
      lstrcpy(m_KeyName, KeyName);
  }
}


CRegSecObject::~CRegSecObject()
{
  if (m_KeyName)
    free(m_KeyName);
}


//**********************************************************************
// CRegSecObject::SetObjectSecurity
// =================================
// Set the security info in our internal security descriptor into our
// registry key.
//**********************************************************************

BOOL CRegSecObject::SetObjectSecurity(int bInheritACL)
{ DWORD d;

// Check the key name has been set

  if (!m_KeyName)
  { m_iSecErrorCode = 0; // Have to think what error to return here
    return(FALSE);
  }

// Set the key security

  SetPrivilegeInAccessToken(TRUE);

  if (bInheritACL > 0)
    d = SetNamedSecurityInfo(m_KeyName, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, m_pDACL, NULL);
  else if (bInheritACL < 0)
    d = SetNamedSecurityInfo(m_KeyName, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, m_pDACL, NULL);
  else
    d = SetNamedSecurityInfo(m_KeyName, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, m_pDACL, NULL);

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
// CRegSecObject::GetObjectSecurity
// ===================================
// Read the ACLs from the supplied handle and build a security
// descriptor with the ACLs so we can subsequently manipulate them.
//**********************************************************************

BOOL CRegSecObject::GetObjectSecurity(void)
{ PSECURITY_DESCRIPTOR pSD;
  PACL pDACL;
  DWORD d;

// The registry key name must have been set for this function to work

  if (!m_KeyName)
  { m_iSecErrorCode = 0; // Have to think what error to return here
    return(FALSE);
  }

// Get the ACL and SD

  SetPrivilegeInAccessToken(TRUE);

  d = GetNamedSecurityInfo(m_KeyName, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, &pDACL, NULL, &pSD);

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
};


//**********************************************************************
// CRegSecObject::RightsToText
// ----------------------------
// This converts an access mask to a text representation
// NB the WCHAR* must be at least 10 bytes long.
//
// winnt.h defines the following specific access masks:
//
// #define KEY_QUERY_VALUE         (0x0001)
// #define KEY_SET_VALUE           (0x0002)
// #define KEY_CREATE_SUB_KEY      (0x0004)
// #define KEY_ENUMERATE_SUB_KEYS  (0x0008)
// #define KEY_NOTIFY              (0x0010)
// #define KEY_CREATE_LINK         (0x0020)
//
// #define KEY_READ                ((STANDARD_RIGHTS_READ       |\
//                                  KEY_QUERY_VALUE            |\
//                                  KEY_ENUMERATE_SUB_KEYS     |\
//                                  KEY_NOTIFY)                 \
//                                  &                           \
//                                 (~SYNCHRONIZE))
//
//
//#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
//                                  KEY_SET_VALUE              |\
//                                  KEY_CREATE_SUB_KEY)         \
//                                  &                           \
//                                 (~SYNCHRONIZE))
//
//#define KEY_EXECUTE             ((KEY_READ)                   \
//                                  &                           \
//                                 (~SYNCHRONIZE))
//
//#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
//                                  KEY_QUERY_VALUE            |\
//                                  KEY_SET_VALUE              |\
//                                  KEY_CREATE_SUB_KEY         |\
//                                  KEY_ENUMERATE_SUB_KEYS     |\
//                                  KEY_NOTIFY                 |\
//                                  KEY_CREATE_LINK)            \
//                                  &                           \
//                                 (~SYNCHRONIZE))
//**********************************************************************

const WCHAR* CRegSecObject::RightsToText(DWORD Mask, WCHAR* Text)
{ DWORD specific_mask, generic_mask;

  lstrcpy(Text, L"");

// Specific rights first

  specific_mask = Mask & 0x00ffffff;

  if ((specific_mask & KEY_ALL_ACCESS) == KEY_ALL_ACCESS)
  { lstrcpy(Text, L"Full");
  }
  else
  { if ((specific_mask & KEY_READ) == KEY_READ)
      lstrcat(Text, L"R");
    if ((specific_mask & KEY_WRITE) == KEY_WRITE)
      lstrcat(Text, L"W");
    if ((specific_mask & KEY_EXECUTE) == KEY_EXECUTE)
      lstrcat(Text, L"E");
    if ((specific_mask & DELETE) == DELETE)
      lstrcat(Text, L"D");

    if (lstrlen(Text) == 0)
      wsprintf(Text, L"%08X", specific_mask);
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
// CRegSecObject::SetKeyName
// ===========================
// Set the file name if it wasn't done in the constructor
//**********************************************************************

BOOL CRegSecObject::SetKeyName(WCHAR* KeyName)
{
  if (m_KeyName)
  { free(m_KeyName);
    m_KeyName = NULL;
  }

  if (KeyName)
  { m_KeyName = (WCHAR*) malloc(lstrlen(KeyName)*2 + 1);
    if (!m_KeyName)
      return(FALSE);
    lstrcpy(m_KeyName, KeyName);
  }

  return(TRUE);
}


//**********************************************************************
// CRegSecObject::GetKeyName
// ===========================
// Get the file name
//**********************************************************************

const WCHAR* CRegSecObject::GetKeyName(WCHAR* KeyName, int KeyLen)
{
  if (!KeyName || !m_KeyName)
    return(NULL);

// Get the key name

  if ((int) lstrlen(m_KeyName) > KeyLen)
    return(NULL);

  lstrcpy(KeyName, m_KeyName);

// Return the key name

  return KeyName;
}


