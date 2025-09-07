//**********************************************************************
// CWinInetFTP
// ===========
// Class to simplify use of the WinInet API for FTP
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include "CWinInetFTP.h"


//**********************************************************************
//**********************************************************************

CWinInetFTP::CWinInetFTP(void)
{
  m_hOpen = m_hConnect = m_hGet = m_hPut = NULL;
  m_Opened = FALSE;
  m_Server = m_Username = m_Password = m_Proxy = NULL;
}


CWinInetFTP::~CWinInetFTP(void)
{
  Close();

  if (m_Server)   delete [] m_Server;
  if (m_Username) delete [] m_Username;
  if (m_Password) delete [] m_Password;
  if (m_Proxy)    delete [] m_Proxy;
}


//**********************************************************************
//**********************************************************************

HANDLE CWinInetFTP::Open(HANDLE Handle, WCHAR* Proxy)
{ WCHAR s[256];

// If we're being supplied with a handle just save that handle

  if (Handle && Handle != m_hOpen)
  { if (m_hOpen && m_Opened)
      Close();

    m_hOpen = Handle;
    m_Opened = FALSE;
  }

// Otherwise open the Internet session ourself

  else
  { if (Proxy)
      if (!SetProxy(Proxy))
        return(NULL);

    m_hOpen = InternetOpen(L"CWinInetFTP", m_Proxy ? CERN_PROXY_INTERNET_ACCESS : LOCAL_INTERNET_ACCESS, m_Proxy, NULL, 0);
    m_Opened = m_hOpen ? TRUE : FALSE;

    if (!m_hOpen)
    { if (m_Notify)
      { swprintf(s, 255, L"Error opening internet: %s", error());
        m_Notify(s);
      }
    }
  }

// Return the session handle

  return(m_hOpen);
}


void CWinInetFTP::Close(void)
{

// If there is an open connection then close it

  Disconnect();

// If we opened the Internet session then close it

  if (m_hOpen && m_Opened)
  { InternetCloseHandle(m_hOpen);
    m_hOpen = NULL;
    m_Opened = FALSE;
  }
}


//**********************************************************************
//**********************************************************************

HANDLE CWinInetFTP::Connect(WCHAR* Server, WCHAR* Username, WCHAR* Password, WCHAR* Proxy)
{ WCHAR username[256], password[256];

// Process the optional arguments

  if (Server)
    if (!SetServer(Server))
      return(NULL);

  if (Username)
    if (!SetUsername(Username))
      return(NULL);

  if (Password)
    if (!SetPassword(Password))
      return(NULL);

  if (Proxy)
    if (!SetProxy(Proxy))
      return(NULL);

// Check the server has been set

  if (!m_Server)
    return(NULL);

// If the session is not open then open it

  if (!m_hOpen)
    if (!Open(NULL))
      return(NULL);

// Get the username and password

  if (!m_Username)
  { lstrcpy(username, L"anonymous");
    DWORD d = 255;
    GetUserName(password, &d);
    lstrcat(password, L"@");
  }
  else
  { lstrcpy(username, m_Username);
    lstrcpy(password, m_Password ? m_Password : L"");
  }

// If using a proxy reorganise the server/username/password

  if (m_Proxy)
  { lstrcat(username, L"@");
    lstrcat(username, m_Server);
  }

// Open the connection

  m_hConnect = InternetConnect(m_hOpen,
                               m_Proxy ? m_Proxy : m_Server,
                               m_Proxy ? 1555 : INTERNET_INVALID_PORT_NUMBER,
                               username,
                               password,
                               INTERNET_SERVICE_FTP,
                               0,
                               0);

  if (!m_hConnect)
  { if (m_Notify)
    { swprintf(username, 255, L"Error opening connection to %s: %s", m_Proxy ? m_Proxy : m_Server, error());
      m_Notify(username);
    }
    return(NULL);
  }

// Return the connection handle

  return(m_hConnect);
}


void CWinInetFTP::Disconnect(void)
{
  if (m_hConnect)
  { InternetCloseHandle(m_hConnect);
    m_hConnect = NULL;
  }
}


//**********************************************************************
// Routine to retrieve a file from an FTP server
//**********************************************************************

#define LEN_BUF 0x4000

BOOL CWinInetFTP::Get(WCHAR* Source, CWININETFTP_GETPROC GetProc, void* GetProcParam, WCHAR* Server, WCHAR* Username, WCHAR* Password, WCHAR* Proxy)
{ DWORD bytes, d;
  WCHAR buffer[LEN_BUF];

// Check the necessary internet connections are open

  if (!m_hConnect)
    if (!Connect(Server, Username, Password, Proxy))
      return(FALSE);

  if (m_hGet)
  { if (m_Notify)
      m_Notify(L"An FTP file retrieval is already in progress.");
    return(FALSE);
  }

// Open the source file

  if (m_Notify)
  { swprintf(buffer, LEN_BUF, L"Opening %s", Source);
    m_Notify(buffer);
  }

  m_hGet = FtpOpenFile(m_hConnect,
                       Source,
                       GENERIC_READ,
                       FTP_TRANSFER_TYPE_BINARY,
                       0);

  if (!m_hGet)
  { if (m_Notify)
    { swprintf(buffer, LEN_BUF, L"Error opening %s: %s", Source, error());
      m_Notify(buffer);
    }
    return(FALSE);
  }

// Read the file

  bytes = 0;

  while (InternetReadFile(m_hGet, buffer, LEN_BUF, &d))
  { if (d == 0)
      break;

    if (!GetProc(buffer, d, GetProcParam))
      break;

    bytes += d;
    if (m_Notify)
    { swprintf(buffer, LEN_BUF, L"%i bytes transferred", bytes);
      m_Notify(buffer);
    }
  }

  InternetCloseHandle(m_hGet);
  m_hGet = NULL;

// All done

  return(TRUE);
}


//**********************************************************************
// Routine to write a file to an FTP server
//**********************************************************************

BOOL CWinInetFTP::Put(WCHAR* Destination, CWININETFTP_PUTPROC PutProc, void* PutProcParam, WCHAR* Server, WCHAR* Username, WCHAR* Password, WCHAR* Proxy)
{ DWORD bytes, read, written;
  BOOL success;
  WCHAR buffer[LEN_BUF];

  success = TRUE;

// Check the necessary internet connections are open

  if (!m_hConnect)
    if (!Connect(Server, Username, Password, Proxy))
      return(FALSE);

  if (m_hPut)
  { if (m_Notify)
      m_Notify(L"An FTP file retrieval is already in progress.");
    return(FALSE);
  }

// Open the source file

  if (m_Notify)
  { swprintf(buffer, LEN_BUF, L"Opening %s", Destination);
    m_Notify(buffer);
  }

  m_hPut = FtpOpenFile(m_hConnect,
                       Destination,
                       GENERIC_WRITE,
                       FTP_TRANSFER_TYPE_BINARY,
                       0);

  if (!m_hPut)
  { if (m_Notify)
    { swprintf(buffer, LEN_BUF, L"Error opening %s: %s", Destination, error());
      m_Notify(buffer);
    }
    return(FALSE);
  }

// Read the file

  bytes = 0;

  while ((read = PutProc(buffer, LEN_BUF, PutProcParam)) > 0)
  { written = 0;

// If the write fails set the return code to FALSE and break out

    if (!InternetWriteFile(m_hPut, buffer, read, &written))
    { swprintf(buffer, LEN_BUF, L"Failed: %s\n", error());
      m_Notify(buffer);
      success = FALSE;
      break;
    }

// If no all the data was written set the return code to FALSE and break out

    if (read != written)
    { swprintf(buffer, LEN_BUF, L"Failed: %s\n", error());
      m_Notify(buffer);
      success = FALSE;
      break;
    }

// Call the notify proc with the amount of data written

    bytes += written;
    if (m_Notify)
    { swprintf(buffer, LEN_BUF, L"%i bytes transferred", bytes);
      m_Notify(buffer);
    }
  }

// Put is finished so close the connection

  InternetCloseHandle(m_hPut);
  m_hPut = NULL;

// All done

  return success;
}


//**********************************************************************
//**********************************************************************

int CWinInetFTP::Dir(WCHAR* SearchPath, WCHAR* SearchFile, WIN32_FIND_DATA* Info, int MaxItems, WCHAR* Server, WCHAR* Username, WCHAR* Password, WCHAR* Proxy)
{ int numfound;
  HINTERNET hfind;

// Check the necessary internet connections are open

  if (!m_hConnect)
    if (!Connect(Server, Username, Password, Proxy))
      return(FALSE);

// Change to the search directory.

  if (!FtpSetCurrentDirectory(m_hConnect, SearchPath))
    return FALSE;

// Start the file search

  numfound = 0;

  hfind = FtpFindFirstFile(m_hConnect, SearchFile, Info++, 0, 0);

  if (hfind)
  { do
    { numfound++;
      if (numfound >= MaxItems)
        break;
    } while (InternetFindNextFile(hfind, Info++));

    InternetCloseHandle(hfind);
  }

// Return the number of files found

  return(numfound);
}


//**********************************************************************
//**********************************************************************

BOOL CWinInetFTP::SetServer(WCHAR* Server)
{
  if (m_Server)
  { delete [] m_Server;
    m_Server = NULL;
  }

  if (Server)
  { m_Server = new WCHAR[lstrlen(Server) + 1];
    if (!m_Server)
      return(FALSE);
    lstrcpy(m_Server, Server);
  }

  return(TRUE);
}


BOOL CWinInetFTP::SetUsername(WCHAR* Username)
{
  if (m_Username)
  { delete [] m_Username;
    m_Username = NULL;
  }

  if (Username)
  { m_Username = new WCHAR[lstrlen(Username) + 1];
    if (!m_Username)
      return(FALSE);
    lstrcpy(m_Username, Username);
  }

  return(TRUE);
}


BOOL CWinInetFTP::SetPassword(WCHAR* Password)
{
  if (m_Password)
  { delete [] m_Password;
    m_Password = NULL;
  }

  if (Password)
  { m_Password = new WCHAR[lstrlen(Password) + 1];
    if (!m_Password)
      return(FALSE);
    lstrcpy(m_Password, Password);
  }

  return(TRUE);
}


BOOL CWinInetFTP::SetProxy(WCHAR* Proxy)
{
  if (m_Proxy)
  { delete [] m_Proxy;
    m_Proxy = NULL;
  }

  if (Proxy)
  { m_Proxy = new WCHAR[lstrlen(Proxy) + 1];
    if (!m_Proxy)
      return(FALSE);
    lstrcpy(m_Proxy, Proxy);
  }

  return(TRUE);
}




//**********************************************************************
// Print error information from the wininet library
//**********************************************************************

const WCHAR* CWinInetFTP::error(void)
{ DWORD d, len;
  HMODULE h;
  static int errnum = 0;
  static WCHAR err[4][256];

  errnum = errnum < 3 ? errnum + 1 : 0;
  lstrcpy(err[errnum], L"Unidentified error");

  d = GetLastError();

  if (d != ERROR_INTERNET_EXTENDED_ERROR)
  { h = LoadLibrary(L"wininet.dll");
    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM, h, GetLastError(), 0, err[errnum], 255, NULL);
    FreeLibrary(h);
  }
  else
  { len = 255;
    InternetGetLastResponseInfo(&d, err[errnum], &len);
  }

  return(err[errnum]);
}


