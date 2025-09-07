//**********************************************************************
// RhsPOP3Connection
// =================
// Class to encapsulate a POP3 connection.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 07/05/2005
//**********************************************************************

#define STRICT
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include "CRhsPOP3Connection.h"
#include "utils.h"


//**********************************************************************
// Constructor
// -----------
//**********************************************************************

CRhsPOP3Connection::CRhsPOP3Connection()
{
  m_Online = FALSE;
  m_Connected = FALSE;
  strcpy(m_LastError, "");
}


CRhsPOP3Connection::~CRhsPOP3Connection()
{
  if (m_Connected)
    Disconnect();

  if (m_Online)
    GoOffline();
}


//**********************************************************************
// Connect
// -------
// Connect to the POP3 server.
// This will initialise WinSock if it isn't already initialised.
//**********************************************************************

BOOL CRhsPOP3Connection::Connect(const char* POP3Server, const char* Username, const char* Password)
{ char s[256], t[256];

// If WinSock isn't initialised do it now

  if (!m_Online)
  { if (!GoOnline())
    { GetLastErrorMessage();
      m_Online = FALSE;
      return FALSE;
    }
  }

// Connect and read back the response

  if (!m_POP3Socket.Connect(POP3Server, 110))
  { GetLastErrorMessage();
    return FALSE;
  }

  if (!m_POP3Socket.gets(s, 255))
  { strcpy(m_LastError, "The POP3 server did not respond");
    m_POP3Socket.CloseSocket();
    return FALSE;
  }

  if (strncmp(s, "+OK", 3) != 0)
  { strcpy(m_LastError, "Unexpected response: ");
    strcat(m_LastError, s);
    m_POP3Socket.CloseSocket();
    return FALSE;
  }

  m_Connected = TRUE;

// Send the username

  sprintf(s, "USER %s", Username);

  if (!ExecCommand(s, t, 255))
  { strcpy(m_LastError, t);
    return FALSE;
  }

// Send the password

  sprintf(s, "PASS %s", Password);

  if (!ExecCommand(s, t, 255))
  { strcpy(m_LastError, t);
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// Disconnect
// ----------
// Send the QUIT command and disconnect from the server.
// This is called automatically by the destructor
//**********************************************************************

BOOL CRhsPOP3Connection::Disconnect(void)
{ char s[256];

// Check we are connected

  if (!m_Connected)
    return TRUE;

// Send the quit command

  ExecCommand("QUIT", s, 255);

// Close the socket

  m_POP3Socket.CloseSocket();

  m_Connected = FALSE;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// Stat
// ----
// Send the STAT command and return the number of messages and their
// total size.
//**********************************************************************

BOOL CRhsPOP3Connection::Stat(int* NumMails, int* TotalSize)
{ char s[256];

  *NumMails = 0;
  *TotalSize = 0;

// Send the STAT command

  if (!ExecCommand("STAT", s, 255))
    return FALSE;

// Process the results

  if (!ParseMailInfo(s, NumMails, TotalSize))
  { sprintf(m_LastError, "STAT command returned: %s", s);
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// List
// ----
// Send the LIST command and write the message info to the supplied
// arrays.
//**********************************************************************

int CRhsPOP3Connection::List(int Mail[], int Size[], int MaxMails)
{ int num_mails, total_size, i;
  char s[256];

// First do a STAT to get the number of e-mails

  if (!ExecCommand("STAT", s, 255))
    return 0;

  if (!ParseMailInfo(s, &num_mails, &total_size))
  { sprintf(m_LastError, "STAT command returned: %s", s);
    return 0;
  }

// Read back the list of e-mails

  if (!ExecCommand("LIST", s, 255))
    return 0;

  i = 0;

  while (m_POP3Socket.gets(s, 255))
  { if (strcmp(s, ".") == 0)
      break;

// If we've reached the buffer limit continue reading mail info but
// don't save it.

    if (i < MaxMails)
      if (ParseMailInfo(s, Mail + i, Size + i))
        i++;
  }

// All done

  return i;
}


//**********************************************************************
// ParseMailInfo
// -------------
// Mainly a helper function for the Stat and List functions. This takes
// a string of the form:
//   <message number> <message size>
// and parses the two numbers out of it.
//**********************************************************************

BOOL CRhsPOP3Connection::ParseMailInfo(const char* MailInfo, int* Mail, int* Size)
{ int i;

// Find the start of the mail number

  i = 0;

  while (MailInfo[i] == ' ')
    i++;

  if (!strtoint(MailInfo + i, Mail))
    return FALSE;

// Find the start of the mail size

  while (MailInfo[i] != ' ' && MailInfo[i] != '\0')
    i++;

  while (MailInfo[i] == ' ')
    i++;

  if (!strtoint(MailInfo + i, Size))
    return FALSE;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// Delete
// ------
// Delete the specified message
//**********************************************************************

BOOL CRhsPOP3Connection::Delete(int Msg)
{ char s[256], t[256];

// Send the command

  sprintf(s, "DELE %i", Msg);

  if (!ExecCommand(s, t, 255))
  { sprintf(m_LastError, "DELE returned: %s", t);
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// Reset
// -----
// Send the RSET command to abandon all changes (i.e. deletions)
//**********************************************************************

BOOL CRhsPOP3Connection::Reset(void)
{ char s[256];

// Send the command

  if (!ExecCommand("RSET", s, 255))
  { sprintf(m_LastError, "RSET returned: %s", s);
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// ExecCommand
// -----------
// Send a command to the POP3 server and read back the response.
// If the response starts +OK then return TRUE else return FALSE.
// In either case trim +OK or -ERR.
//**********************************************************************

BOOL CRhsPOP3Connection::ExecCommand(const char* Command, char* Response, int MaxLen)
{ int i, j;
  BOOL return_code;
  char s[255];

  strcpy(Response, "");

// Obvious check, we have to be connected

  if (!m_Connected)
  { strcpy(m_LastError, "POP3 server is not connected.");
    return FALSE;
  }

// Send the command

  if (!m_POP3Socket.printf("%s\r\n", Command))
  { GetLastErrorMessage();
    return FALSE;
  }

// Check the response

  if (!m_POP3Socket.gets(s, 255))
  { strcpy(m_LastError, "The POP3 server did not respond");
    return FALSE;
  }

  if (strncmp(s, "+OK", 3) == 0)
  { i = 3;
    return_code = TRUE;
  }

  else if (strncmp(s, "-ERR", 4) == 0)
  { i = 4;
    return_code = FALSE;
  }

  else
  { strncpy(Response, s, MaxLen);
    Response[MaxLen] = '\0';
    return FALSE;
  }

// Clean up the response

  while (s[i] == ' ')
    i++;

  for (j = 0; j < MaxLen && s[i] != '\0'; i++, j++)
    Response[j] = s[i];
  Response[j] = '\0';

// Return indicating if the command was successful

  return return_code;
}


//**********************************************************************
// GetLine
// -------
// Read one line from the POP3 server
//**********************************************************************

BOOL CRhsPOP3Connection::GetLine(char* Response, int MaxLen)
{

// Read data from the server

  if (!m_POP3Socket.gets(Response, MaxLen))
  { strcpy(m_LastError, "The POP3 server did not respond");
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// GoOnline
// --------
// Initialise the WinSock system
//**********************************************************************

BOOL CRhsPOP3Connection::GoOnline(void)
{
  if (m_Online)
    return TRUE;

  WSAStartup(0x0101, &m_WSAData);

  m_Online = TRUE;

  return TRUE;
}


//**********************************************************************
// GoOffline
// ---------
// Close down the WinSock system
//**********************************************************************

BOOL CRhsPOP3Connection::GoOffline(void)
{
  if (!m_Online)
    return TRUE;

  WSACleanup();

  m_Online = FALSE;

  return TRUE;
}


//**********************************************************************
// LastError
// ---------
// Retrieve the last error message.
// The error message string is set by other methods if they encounter
// an error.
//**********************************************************************

const char* CRhsPOP3Connection::LastError(char* Error, int Len)
{
  if (!Error)
    return(NULL);

  strncpy(Error, m_LastError, Len);
  Error[Len] = '\0';
  return(Error);
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
// Helper function to convert the last Win32 error code to a string.
//**********************************************************************

const char* CRhsPOP3Connection::GetLastErrorMessage(void)
{ DWORD d;

  d = GetLastError();

  strcpy(m_LastError, "<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, m_LastError, 1023, NULL);

  return m_LastError;
}


