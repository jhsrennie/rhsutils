//**********************************************************************
// CWinInetFTP
// ===========
// Class to simplify use of the WinInet API for FTP
//**********************************************************************

#ifndef _INC_CWININETFTP
#define _INC_CWININETFTP


//**********************************************************************
// Prototypes
// ----------
//**********************************************************************

typedef BOOL (*CWININETFTP_GETPROC)(WCHAR* Buffer, DWORD Len, void* Param);
typedef int  (*CWININETFTP_PUTPROC)(WCHAR* Buffer, DWORD Len, void* Param);
typedef BOOL (*CWININETFTP_NOTIFYPROC)(WCHAR* Buffer);


//**********************************************************************
// CWinInetFTP
// -----------
//**********************************************************************

class CWinInetFTP
{

  public:
    CWinInetFTP(void);
    ~CWinInetFTP(void);

    HANDLE Open(HANDLE Handle = NULL, WCHAR* Proxy = NULL);
    void   Close(void);

    HANDLE Connect(WCHAR* Server = NULL, WCHAR* Username = NULL, WCHAR* Password = NULL, WCHAR* Proxy = NULL);
    void   Disconnect(void);

    BOOL Get(WCHAR* Source, CWININETFTP_GETPROC GetProc, void* GetProcParam = NULL, WCHAR* Server = NULL, WCHAR* Username = NULL, WCHAR* Password = NULL, WCHAR* Proxy = NULL);
    BOOL Put(WCHAR* Source, CWININETFTP_PUTPROC PutProc, void* PutProcParam = NULL, WCHAR* Server = NULL, WCHAR* Username = NULL, WCHAR* Password = NULL, WCHAR* Proxy = NULL);

    int Dir(WCHAR* SearchPath, WCHAR* SearchFile, WIN32_FIND_DATA* Info, int MaxItems, WCHAR* Server = NULL, WCHAR* Username = NULL, WCHAR* Password = NULL, WCHAR* Proxy = NULL);

    inline void SetNotify(CWININETFTP_NOTIFYPROC Notify) { m_Notify = Notify; }

    BOOL SetServer(WCHAR* Server);
    BOOL SetUsername(WCHAR* Username);
    BOOL SetPassword(WCHAR* Password);
    BOOL SetProxy(WCHAR* Proxy);

    inline const WCHAR* GetServer(void)   { return(m_Server); }
    inline const WCHAR* GetUsername(void) { return(m_Username); }
    inline const WCHAR* GetPassword(void) { return(m_Password); }
    inline const WCHAR* GetProxy(void)    { return(m_Proxy); }

    const WCHAR* error(void);

  private:
    HANDLE m_hOpen,
           m_hConnect,
           m_hGet,
           m_hPut;

    BOOL m_Opened;

    WCHAR* m_Server;
    WCHAR* m_Username;
    WCHAR* m_Password;
    WCHAR* m_Proxy;

    CWININETFTP_NOTIFYPROC m_Notify;

};

//**********************************************************************
// End of CWinInetFTP
// ------------------
//**********************************************************************

#endif // _INC_CWININETFTP

