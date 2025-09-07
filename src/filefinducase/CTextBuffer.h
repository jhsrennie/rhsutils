// *********************************************************************
// CTextBuffer.h
// =============
//
// John Rennie
// 05/06/16
// *********************************************************************

#ifndef _INC_CTEXTBUFFER
#define _INC_CTEXTBUFFER


//**********************************************************************
// CTextBuffer
// -----------
// Class to buffer text
//**********************************************************************

#define LEN_CTEXTBUFFERBUF 0x4000

class CTextBuffer
{
  public:
    CTextBuffer();
    ~CTextBuffer();

    BOOL Initialise(void);
    BOOL Readback(void);

    int printf(const char* Format, ...);
    const char* gets(void);

  private:
    FILE* m_File;
    BOOL  m_IOState; // 0 = not open, 1 = write, 2 = read
    WCHAR m_TempFileName[MAX_PATH+1];

    char  m_Buf[LEN_CTEXTBUFFERBUF];

};


//**********************************************************************
// End of CTextBuffer
// ------------------
//**********************************************************************

#endif // _INC_CTEXTBUFFER

