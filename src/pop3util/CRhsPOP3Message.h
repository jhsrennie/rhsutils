//**********************************************************************
// RhsPOP3Message
// ==============
// Class to encapsulate a POP3 message
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 07/05/2005
//**********************************************************************

#ifndef _INC_RHSPOP3MESSAGE
#define _INC_RHSPOP3MESSAGE

//**********************************************************************
// Class definition
//**********************************************************************

class CRhsPOP3Message
{

public:

// Constructors

  CRhsPOP3Message();
  ~CRhsPOP3Message();

// Methods

  void SetConnection(CRhsPOP3Connection* Connection);

  BOOL ReadMessage(int Msg, BOOL HeaderOnly);

  BOOL Filter(const char* FilterString);

  BOOL SaveToFile(const char* Filename);
  BOOL SaveToFile(FILE* File);

  const char* LastError(char* Error, int Len);
  const char* GetLastErrorMessage(void);

// Class variables

private:

// The POP3 connection

  CRhsPOP3Connection* m_Connection;

// The temporary storage file name and handle

  FILE* m_TempFile;
  char m_TempFileName[MAX_PATH+1];

// The last error; zero length string if there was no last error.

  char m_LastError[1024];

};


//**********************************************************************
// End of class definition
//**********************************************************************

#endif // _INC_RHSPOP3MESSAGE

