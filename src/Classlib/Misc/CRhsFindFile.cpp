//**********************************************************************
// CRhsFindFile
// ============
// Class to find files
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include "CRhsFindFile.h"


//**********************************************************************
// CRhsFindFile
// ------------
//**********************************************************************

CRhsFindFile::CRhsFindFile()
{
  m_Handle = NULL;
  m_Path = NULL;
}

CRhsFindFile::~CRhsFindFile()
{
  Close();
}


//**********************************************************************
//**********************************************************************

BOOL CRhsFindFile::First(const WCHAR* Search, WCHAR* Found, int Len)
{ int i;

// Some obvious checks

  if (!Search || !Found || Len < 1)
    return(FALSE);

// Close the object just in case it's been used before

  Close();

// Try the search

  m_Handle = FindFirstFile(Search, &m_Data);

  if (m_Handle == INVALID_HANDLE_VALUE)
  { m_Handle = NULL;
    return(FALSE);
  }

// Ignore . and ..

  while (lstrcmp(m_Data.cFileName, L".") == 0 || lstrcmp(m_Data.cFileName, L"..") == 0)
  { if (!FindNextFile(m_Handle, &m_Data))
    { Close();
      return(FALSE);
    }
  }

// Found a file; save the path from the search expression

  i = lstrlen(Search);
  m_Path = new WCHAR[i + 1];

  if (!m_Path)
  { Close();
    return(FALSE);
  }

  lstrcpy(m_Path, Search);
  while (i > 0)
  { if (m_Path[i - 1] == '\\')
      break;
    i--;
  }
  m_Path[i] = '\0';

// Now return the file found including the path

  if (i + (int) lstrlen(m_Data.cFileName) > Len)
  { Close();
    return(FALSE);
  }

  lstrcpy(Found, m_Path);
  lstrcat(Found, m_Data.cFileName);

// Return indicating success

  return(TRUE);
}


//**********************************************************************
//**********************************************************************

BOOL CRhsFindFile::Next(WCHAR* Found, int Len)
{

// Some obvious checks

  if (!m_Handle || !Found || Len < 1)
    return(FALSE);

// Find the next file

  if (!FindNextFile(m_Handle, &m_Data))
    return(FALSE);

// Ignore . and ..

  while (lstrcmp(m_Data.cFileName, L".") == 0 || lstrcmp(m_Data.cFileName, L"..") == 0)
    if (!FindNextFile(m_Handle, &m_Data))
      return(FALSE);

// Now return the file found including the path

  if ((int) lstrlen(m_Path) + (int) lstrlen(m_Data.cFileName) > Len)
    return(FALSE);

  lstrcpy(Found, m_Path);
  lstrcat(Found, m_Data.cFileName);

// Return indicating success

  return(TRUE);
}


//**********************************************************************
//**********************************************************************

void CRhsFindFile::Close(void)
{
  if (m_Handle)
  { FindClose(m_Handle);
    m_Handle = NULL;
  }

  if (m_Path)
  { delete [] m_Path;
    m_Path = NULL;
  }
}


//**********************************************************************
//**********************************************************************

WCHAR* CRhsFindFile::FullFilename(WCHAR* Name, int Len)
{

// Some obvious checks

  if (!m_Handle || !Name || Len < 1)
    return(FALSE);

// Return the file found including the path

  if ((int) lstrlen(m_Path) + (int) lstrlen(m_Data.cFileName) > Len)
    return(NULL);

  lstrcpy(Name, m_Path);
  lstrcat(Name, m_Data.cFileName);

  return(Name);
}


WCHAR* CRhsFindFile::Filename(WCHAR* Name, int Len)
{

// Some obvious checks

  if (!m_Handle || !Name || Len < 1)
    return(FALSE);

// Return the file found without the path

  if ((int) lstrlen(m_Data.cFileName) > Len)
    return(NULL);

  lstrcpy(Name, m_Data.cFileName);

  return(Name);
}


//**********************************************************************
//**********************************************************************

BOOL CRhsFindFile::FileSize(DWORD* Low, DWORD* High)
{
  if (!m_Handle)
    return FALSE;

  *Low = m_Data.nFileSizeLow;
  *High = m_Data.nFileSizeHigh;

  return TRUE;
}


