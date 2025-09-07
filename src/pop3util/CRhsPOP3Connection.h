//**********************************************************************
// RhsPOP3Connection
// =================
// Class to encapsulate a POP3 connection.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 07/05/2005
//**********************************************************************

#ifndef _INC_RHSPOP3CONNECTION
#define _INC_RHSPOP3CONNECTION

#include <CRhsSocket/CRhsSocket.h>


//**********************************************************************
// Class definition
//**********************************************************************

class CRhsPOP3Connection
{

public:

// Constructors

  CRhsPOP3Connection();
  ~CRhsPOP3Connection();

// Connect/disconnect the socket for mail commands.  The Connect function will
// automatically call GoOnline if necessary.

  BOOL Connect(const char* POP3Server, const char* Username, const char* Password);
  BOOL Disconnect(void);

// Get the number and size of the e-mails

  BOOL Stat(int* NumMails, int* TotalSize);
  BOOL List(int Mail[], int Size[], int MaxMails);
  BOOL ParseMailInfo(const char* MailInfo, int* Mail, int* Size);

// Delete a message

  BOOL Delete(int Msg);

// Abandon changes

  BOOL Reset(void);

// Send a command and get the response

  BOOL ExecCommand(const char* Command, char* Response, int MaxLen);

  BOOL GetLine(char* Response, int MaxLen);

// Initialise and shutdown the WinSock system

  BOOL GoOnline(void);
  BOOL GoOffline(void);

// Retrieve the last error message

  const char* LastError(char* Error, int Len);
  const char* GetLastErrorMessage(void);

// Class variables

private:

// WinSock data

  WSADATA m_WSAData;

// Are we online, is the mail socket connected?

  BOOL m_Online,
       m_Connected;

// The POP3 socket

  CRhsSocket m_POP3Socket;

// The last error; zero length string if there was no last error.

  char m_LastError[1024];

};


//**********************************************************************
// End of class definition
//**********************************************************************

#endif // _INC_RHSPOP3CONNECTION

