//**********************************************************************
// RhsPOP3Message
// ==============
// Class to encapsulate a POP3 message.
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
#include "CRhsPOP3Message.h"
#include "utils.h"


//**********************************************************************
// Constructor
// -----------
//**********************************************************************

CRhsPOP3Message::CRhsPOP3Message()
{ char s[MAX_PATH];

// Set the POP3Connection object to NULL

  m_Connection = NULL;

// Get the temporary file

  m_TempFile = NULL;
  
  GetTempPath(MAX_PATH, s);
  GetTempFileName(s, "POP3", 0, m_TempFileName);

// The last error; zero length string if there was no last error.

  char m_LastError[1024];
  strcpy(m_LastError, "");
}


CRhsPOP3Message::~CRhsPOP3Message()
{

// Delete the temporary file

  if (m_TempFile)
    fclose(m_TempFile);
  
  DeleteFile(m_TempFileName);
}


//**********************************************************************
// SetConnection
// -------------
// Set the connection object
//**********************************************************************

void CRhsPOP3Message::SetConnection(CRhsPOP3Connection* Connection)
{
  m_Connection = Connection;
}


//**********************************************************************
// ReadHeader
// ----------
// Read the message header using the TOP command
//**********************************************************************

BOOL CRhsPOP3Message::ReadMessage(int Msg, BOOL HeaderOnly)
{ char s[256], t[256];

// We must have a connection open

  if (!m_Connection)
  { strcpy(m_LastError, "The POP connection object has not been set");
    return FALSE;
  }

// Open the temporary storage

  if (m_TempFile)
  { fclose(m_TempFile);
    m_TempFile = NULL;
  }
  
  m_TempFile = fopen(m_TempFileName, "w");
  if (!m_TempFile)
  { GetLastErrorMessage();
    return FALSE;
  }

// Get the message

  if (HeaderOnly)
    sprintf(s, "TOP %i 0", Msg);
  else
    sprintf(s, "RETR %i", Msg);

  if (!m_Connection->ExecCommand(s, t, 255))
  { sprintf(m_LastError, "POP3 server returned error: %s\n", m_Connection->LastError(s, 255));
    return FALSE;
  }

// Read back the message
// Note that we leave the file open to stop anything deleting it

  while (m_Connection->GetLine(s, 255))
  { if (strcmp(s, ".") == 0)
      break;

    fprintf(m_TempFile, "%s\n", s);
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// Filter
// ------
// Search the message for the supplied filter.
// Return TRUE if the filter is found and FALSE otherwise;
//**********************************************************************

BOOL CRhsPOP3Message::Filter(const char* FilterString)
{ char s[256];

// Open the temporary storage

  if (m_TempFile)
  { fclose(m_TempFile);
    m_TempFile = NULL;
  }
  
  m_TempFile = fopen(m_TempFileName, "r");
  if (!m_TempFile)
  { GetLastErrorMessage();
    return FALSE;
  }

// Stream the storage to the supplied handle

  while (fgets(s, 255, m_TempFile))
  { 

// If we find the filkter expression return immediately
  
    if (strstr(s, FilterString))
      return TRUE;
  }

// Return indicating the filter was not found

  return FALSE;
}


//**********************************************************************
// SaveToFile
// ----------
// Write the temporary storage to the supplied file name or handle
//**********************************************************************

BOOL CRhsPOP3Message::SaveToFile(const char* Filename)
{ BOOL success;
  FILE* f;

// Create the file

  f = fopen(Filename, "w");
  if (!m_TempFile)
  { GetLastErrorMessage();
    return FALSE;
  }

  success = SaveToFile(f);

  fclose(f);

// Return indicating success

  return success;
}


BOOL CRhsPOP3Message::SaveToFile(FILE* File)
{ char s[256];

// Open the temporary storage

  if (m_TempFile)
  { fclose(m_TempFile);
    m_TempFile = NULL;
  }
  
  m_TempFile = fopen(m_TempFileName, "r");
  if (!m_TempFile)
  { GetLastErrorMessage();
    return FALSE;
  }

// Stream the storage to the supplied handle

  while (fgets(s, 255, m_TempFile))
    fputs(s, File);

// Return indicating success

  return TRUE;
}


//**********************************************************************
// LastError
// ---------
// Retrieve the last error message.
// The error message string is set by other methods if they encounter
// an error.
//**********************************************************************

const char* CRhsPOP3Message::LastError(char* Error, int Len)
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

const char* CRhsPOP3Message::GetLastErrorMessage(void)
{ DWORD d;

  d = GetLastError();

  strcpy(m_LastError, "<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, m_LastError, 1023, NULL);

  return m_LastError;
}


