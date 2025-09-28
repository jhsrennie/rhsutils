// *********************************************************************
// CRhsIO.h
// ========
// *********************************************************************

#ifndef _INC_CRHSIO
#define _INC_CRHSIO


//**********************************************************************
// Forward declarations
//**********************************************************************

class CRhsIOWnd;
class CRhsFont;
class CRhsRegistry;


//**********************************************************************
// CRhsIO
// ------
//**********************************************************************

class CRhsIO
{

  public:
    CRhsIO();
    ~CRhsIO();

    BOOL Init(HINSTANCE hInstance, WCHAR* Title);
    int Run(LPTHREAD_START_ROUTINE lpStartAddress);

    int printf(const WCHAR *Format, ...);
    int errprintf(const WCHAR *Format, ...);

    void SetLastError(const WCHAR* ErrMsg, ...);
    const WCHAR* LastError(WCHAR* ErrBuf, int BufLen);

    inline BOOL IsConsole(void) { return m_IsConsole; }
    inline BOOL GetAbort(void) { return m_Abort; }
    inline void SetAbort(BOOL Abort) { m_Abort = Abort ? TRUE : FALSE; }

  private:
    void ParseCmdLine(WCHAR *cmdstart, WCHAR **argv, int *argc, WCHAR *args, int *numchars);
    DWORD GetParentPID(WCHAR* ParentName);

  public:
    BOOL m_Initialised;

    HINSTANCE m_hInstance;
#define LEN_APPTITLE 256
    WCHAR m_AppTitle[LEN_APPTITLE];

    HANDLE m_stdin, m_stdout, m_stderr;

#define MAX_ARGS 64
    WCHAR* m_argv[MAX_ARGS];
    int    m_argc;

  private:
    CRhsIOWnd* m_hIOWnd;

    BOOL m_IsConsole;
    BOOL m_Abort; // Set to TRUE to ask worker thread to stop

#define LEN_ARGBUF 1024
    WCHAR m_argbuf[LEN_ARGBUF];

#define LEN_LASTERRBUF 256
    WCHAR m_LastError[LEN_LASTERRBUF];
};


//**********************************************************************
// CRhsIOWnd
// -----------
// Implementation of main window
//**********************************************************************

class CRhsIOWnd
{

// Methods

  public:
    CRhsIOWnd(void);
    ~CRhsIOWnd(void);

    BOOL Open(HINSTANCE hInstance, WCHAR* Title, int Left, int Top, int Width, int Depth);
    inline void SetRhsIO(CRhsIO* RhsIO) { m_RhsIO = RhsIO; }
    inline void SetThread(HANDLE hThread) { m_hMainThread = hThread; }

    BOOL AddText(WCHAR* Text);
    int  KillWorkerThread(void);
    int  SaveBufFile(void);
    BOOL SearchOutput(void);
    long OnSearchMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    int  ChooseFont(void);
    void WebSearch(void);
    void OpenURL(void);
    long OnTimer(void);

    BOOL FitOnScreen(BOOL Origin, BOOL Size);
    LRESULT OnSize(UINT fwSizeType);
    LRESULT OnMove(void);
    LRESULT OnPaint(HDC hDC);
    LRESULT OnDestroy(void);

    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
    LRESULT OnInitMenuPopup(HMENU hPopup, UINT uPos, BOOL fSystemMenu);
    void OnRightClick(int x, int y);
    LRESULT OnMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    int  TranslateAccelerator(LPMSG Msg);

    const WCHAR* LastError(WCHAR* ErrBuf, int BufLen);

// Variables

  private:
    CRhsIO* m_RhsIO;
    HINSTANCE m_hInstance;
    WCHAR m_AppTitle[LEN_APPTITLE];

    HWND   m_hWnd;
    HACCEL m_hAccel;

    HWND m_hEditWnd;
    CRhsFont* m_EditFont;

    UINT_PTR m_Timer;
    int  m_TimerCount;

    HMENU m_PopMenu;

#define LEN_SEARCHDLGTEXT 256
    HWND m_hSearchDlg;
    UINT m_SearchMsg;
    WCHAR m_SearchDlgText[LEN_SEARCHDLGTEXT];

    HANDLE m_hMainThread;
    BOOL m_ThreadComplete;

    BOOL   m_DisplayFull;
    HANDLE m_BufFile;
    WCHAR  m_BufFileName[MAX_PATH+1];

    CRhsRegistry* m_Registry;

    WCHAR m_LastError[LEN_LASTERRBUF];
};


//**********************************************************************
// End of CRhsIOWnd
//**********************************************************************

#endif // _INC_CRHSIO

