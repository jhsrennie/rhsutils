//**********************************************************************
// CRhsRegistry
// ============
//
// A class to simplify registry access
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 4th January 1998
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include "CRhsRegistry.h"


//**********************************************************************
// CRhsRegistry::CRhsRegistry
// CRhsRegistry::~CRhsRegistry
// ---------------------------
// Constructor and destructor
// The key name can be passed to the constructor
//**********************************************************************

CRhsRegistry::CRhsRegistry(void)
{
  m_HKeyLocalMachine = FALSE;
  lstrcpy(m_Key, L"");
}


CRhsRegistry::CRhsRegistry(const WCHAR* Key, BOOL HKeyLocalMachine)
{
  m_HKeyLocalMachine = FALSE;
  lstrcpy(m_Key, L"");

  if (Key)
    if (lstrlen(Key) <= LEN_REGISTRYPATH)
      lstrcpy(m_Key, Key);

  m_HKeyLocalMachine = HKeyLocalMachine;
}


CRhsRegistry::~CRhsRegistry(void)
{
}


//**********************************************************************
// CRhsRegistry::SetKey
// --------------------
// Set the key and whether HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER is
// to be used.
//**********************************************************************

BOOL CRhsRegistry::SetKey(const WCHAR* Key, BOOL HKeyLocalMachine)
{
  if (!Key)
    return(FALSE);

  if (lstrlen(Key) > LEN_REGISTRYPATH)
    return(FALSE);

  lstrcpy(m_Key, Key);

  m_HKeyLocalMachine = HKeyLocalMachine;

  return(TRUE);
}


//**********************************************************************
// CRhsRegistry::GetKey
// --------------------
// Get the key and whether HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER is
// being used.
//**********************************************************************

BOOL CRhsRegistry::GetKey(WCHAR* Key, int Len, BOOL* HKeyLocalMachine)
{
  if ((int) lstrlen(m_Key) > Len)
    return(FALSE);

  lstrcpy(Key, m_Key);

  if (HKeyLocalMachine)
    *HKeyLocalMachine = m_HKeyLocalMachine;

  return(TRUE);
}


//**********************************************************************
// CRhsRegistry::WriteString
// -------------------------
// Write a string to the registry
//**********************************************************************

BOOL CRhsRegistry::WriteString(const WCHAR* Key, const WCHAR* String)
{ HKEY h;
  LONG l;
  DWORD d;

// Check parameters

  if (!Key || !String)
    return(FALSE);

// Write to the registry

  l = RegCreateKeyEx(m_HKeyLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, m_Key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &h, &d);
  if (l != ERROR_SUCCESS)
    return(FALSE);

  l = RegSetValueEx(h, Key, 0, REG_SZ, (CONST BYTE*) String, lstrlen(String)*2 + 1);
  RegCloseKey(h);

  if (l != ERROR_SUCCESS)
    return(FALSE);

// Return indicating success

  return(TRUE);
}


//**********************************************************************
// CRhsRegistry::WriteInt
// ----------------------
// Write an integer to the registry
//**********************************************************************

BOOL CRhsRegistry::WriteInt(const WCHAR* Key, int Value)
{ HKEY h;
  LONG l;
  DWORD value, d;

// Check parameters

  if (!Key)
    return(FALSE);

// Write to the registry

  l = RegCreateKeyEx(m_HKeyLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, m_Key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &h, &d);
  if (l != ERROR_SUCCESS)
    return(FALSE);

  value = (DWORD) Value;
  l = RegSetValueEx(h, Key, 0, REG_DWORD, (CONST BYTE*) &value, sizeof(DWORD));
  RegCloseKey(h);

  if (l != ERROR_SUCCESS)
    return(FALSE);

// Return indicating success

  return(TRUE);
}


//**********************************************************************
// CRhsRegistry::GetString
// -----------------------
// Read a string from the registry.
// The string is read into a local buffer and a pointer to the string
// is returned.
//**********************************************************************

const WCHAR* CRhsRegistry::GetString(const WCHAR* Key, const WCHAR* Default, int Len)
{ DWORD len;
  HKEY h;
  LONG l;
  DWORD regtype, d;

// An internal 4 string buffer is used to store read strings

  static int  id = -1;
  static WCHAR string[4][LEN_REGISTRYPATH+1];

// Check parameters

  id = (id + 1) % 4;

  if (!Key)
    return(NULL);

// Read from the registry

  len = Len;

  l = RegCreateKeyEx(m_HKeyLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, m_Key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &h, &d);
  if (l != ERROR_SUCCESS)
  { if (Default)
      lstrcpyn(string[id], Default, LEN_REGISTRYPATH);
    else
      lstrcpy(string[id], L"");
  }

  l = RegQueryValueEx(h, Key, NULL, &regtype, (LPBYTE) string[id], (DWORD*) &len);
  RegCloseKey(h);

// If necessary use the default

  if (l != ERROR_SUCCESS)
  { if (Default)
      lstrcpyn(string[id], Default, LEN_REGISTRYPATH);
    else
      lstrcpy(string[id], L"");
  }

// Return length of string

  return(string[id]);
}


//**********************************************************************
// CRhsRegistry::GetInt
// --------------------
// Get an integer from the registry
//**********************************************************************

int CRhsRegistry::GetInt(const WCHAR* Key, int Default)
{ HKEY h;
  LONG l;
  DWORD regtype, value, len, d;

// Check parameters

  if (!Key)
    return(Default);

// Read from the registry

  l = RegCreateKeyEx(m_HKeyLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, m_Key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &h, &d);
  if (l != ERROR_SUCCESS)
    return(Default);

  len = sizeof(DWORD);
  l = RegQueryValueEx(h, Key, NULL, &regtype, (LPBYTE) &value, (DWORD*) &len);
  RegCloseKey(h);

  if (l != ERROR_SUCCESS)
    return(Default);

  if (regtype != REG_DWORD)
    return(Default);

  return((int) value);
}


