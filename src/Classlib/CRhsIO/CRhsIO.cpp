//**********************************************************************
// CRhsIO
// ======
// Class to encapsulate IO for an application.
// The class uses console IO if launched from a command line and
// graphical IO if there is no console.
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <windowsx.h>
#include <richedit.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <shlwapi.h>
#include <Misc/CRhsRegistry.h>
#include <Misc/CRhsFont.h>
#include "CRhsIO.h"
#include "CRhsIOWnd.h"


//**********************************************************************
// Non class variables
// -------------------
//**********************************************************************

#define TXT_BUFFULL \
L"\r\n\r\nThe display buffer is full.\r\n" \
L"Output is being buffered and can be saved to a file."

#define LEN_TXT_BUFFULL 85

// The edit control is retricted to 1MB to avoid it guzzling all the memory

#define LEN_DISPLAYBUF 1048576


//**********************************************************************
// Non class prototypes
// --------------------
//**********************************************************************

int strcmpleft(const WCHAR* s1, const WCHAR* s2);


//**********************************************************************
// CRhsIO
// ~CRhsIO
// -------
// Constructor and destructor
//**********************************************************************

CRhsIO::CRhsIO()
{ int i;

// Initialise variables

  m_Initialised = FALSE;

// General app settings

  m_hInstance = NULL;
  lstrcpy(m_AppTitle, L"");

// Command line variables

 for (i = 0; i < MAX_ARGS; i++)
    m_argv[i] = NULL;
  m_argc = 0;

  lstrcpy(m_argbuf, L"");

// Console variables

  m_IsConsole = TRUE;
  m_stdin = m_stdout = m_stderr = NULL;
  m_Abort = FALSE;

// Bits and bobs

  m_hIOWnd = NULL;
  lstrcpy(m_LastError, L"");
}

CRhsIO::~CRhsIO()
{
  if (m_hIOWnd)
    delete m_hIOWnd;
  m_hIOWnd = NULL;
}


//**********************************************************************
// Init
// ----
// Set up the environment for the app to do it's IO.
// For a console app there is little to do except retrieve the standard
// IO handles.
// For a graphical app create the window for the app to display it's
// output.
//**********************************************************************

BOOL CRhsIO::Init(HINSTANCE hInstance, WCHAR* Title)
{ int arglen, i;
  BOOL modeset;
  WCHAR argbuf[LEN_ARGBUF];
  WCHAR parent_name[MAX_PATH+1];

  lstrcpy(m_LastError, L"");

// If we've already been initialised don't allow re-initialisation

  if (m_Initialised)
  { lstrcpy(m_LastError, L"RhsIO has already been initialised.");
    return FALSE;
  }

// Save the instance and title

  m_hInstance = hInstance;

  lstrcpy(m_AppTitle, Title);
  if (lstrlen(m_AppTitle) == 0)
    lstrcpy(m_AppTitle, L"RhsIO");

// Initialise the app argument buffers

 for (i = 0; i < MAX_ARGS; i++)
    m_argv[i] = NULL;
  m_argc = 0;
  lstrcpy(m_argbuf, L"");

  lstrcpyn(argbuf, GetCommandLine(), LEN_ARGBUF);
  ParseCmdLine(argbuf, m_argv, &m_argc, m_argbuf, &arglen);

// Check if this is a command line or GUI app

  modeset = FALSE;
  m_IsConsole = TRUE;

// If the first flag is --gui this forces the app to be GUI. If the
// first argument is --console it forces the app to be console.
// Remove the flag so the app doesn't see it.

  if (m_argc > 1)
  { if (lstrcmp(m_argv[1], L"--gui") == 0)
    { modeset = TRUE;
      m_IsConsole = FALSE;

      for (i = 2; i < m_argc; i++)
        m_argv[i-1] = m_argv[i];
      m_argc--;
    }
    else if (lstrcmp(m_argv[1], L"--console") == 0)
    { modeset = TRUE;
      m_IsConsole = TRUE;

      for (i = 2; i < m_argc; i++)
        m_argv[i-1] = m_argv[i];
      m_argc--;
    }
  }

// If the parent app is not cmd.exe this is a GUI app

  if (!modeset)
  { if (GetParentPID(parent_name))
    { CharLower(parent_name);
      if (lstrcmp(parent_name, L"cmd.exe") != 0)
      { modeset = TRUE;
        m_IsConsole = FALSE;
      }
    }
  }

// If it's a console app save the stdio handles

  if (m_IsConsole)
  { m_stdin  = GetStdHandle(STD_INPUT_HANDLE);
    m_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    m_stderr = GetStdHandle(STD_ERROR_HANDLE);
  }

// If this is a graphical app create a display window

  if (!m_IsConsole)
    m_hIOWnd = new CRhsIOWnd;

// Return indicating success

  m_Initialised = TRUE;

  return TRUE;
}


//**********************************************************************
// Run
// ---
// Run the client's main function.
// For console mode just call the function.
// For graphical mode run the function in a separate thread then spin
// the message loop.
//**********************************************************************

int CRhsIO::Run(LPTHREAD_START_ROUTINE lpStartAddress)
{ BOOL b;
  DWORD retcode, threadid;
  HANDLE hthread;
  MSG msg;

  lstrcpy(m_LastError, L"");

// Check we've been initialised

  if (!m_Initialised)
  { lstrcpy(m_LastError, L"RhsIO has not been initialised.");
    return 2;
  }

// If this is a command line app just run the main function

  if (m_IsConsole)
  { retcode = lpStartAddress(NULL);

    return (int) retcode;
  }

// Else create the IO window and free the console

  m_hIOWnd->SetRhsIO(this);

  b = m_hIOWnd->Open(m_hInstance, m_AppTitle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT);

  if (!b)
  { m_hIOWnd->LastError(m_LastError, LEN_LASTERRBUF);
    return 2;
  }

  FreeConsole();

// Run the main function in a separate thread

  hthread = CreateThread(NULL, 0, lpStartAddress, this, 0, &threadid);

  m_hIOWnd->SetThread(hthread);

// Enter message loop

  while (GetMessage(&msg, NULL, 0, 0))
  { if (!m_hIOWnd->TranslateAccelerator(&msg))
    { TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return((int) msg.wParam);
}


//**********************************************************************
// printf
// ------
// Write to standard output
// For graphical apps both printf and errprintf display the text in the
// same window.
//**********************************************************************

#define MAX_PRINTFBUFLEN 0x4000

int CRhsIO::printf(const WCHAR *Format, ...)
{ BOOL b;
  DWORD numtowrite, numwritten;
  WCHAR w[MAX_PRINTFBUFLEN];
  char s[MAX_PRINTFBUFLEN];
  va_list ap;

// Build the output string

  va_start(ap, Format);
  numtowrite = vswprintf(w, MAX_PRINTFBUFLEN-1, Format, ap);
  va_end(ap);

// We always output ANSI not UNICODE, and convert \n to \r\n

  WideCharToMultiByte(CP_ACP, 0, w, -1, s, MAX_PRINTFBUFLEN-1, NULL, 0);

// For console mode apps write to the standard output stream

  if (m_IsConsole)
  { b = WriteFile(m_stdout, s, numtowrite, &numwritten, NULL);

    if (!b)
    { FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, m_LastError, LEN_LASTERRBUF, NULL);
      return -1;
    }

    return (int) numwritten;
  }

// For GUI apps append the text to the display window

  m_hIOWnd->AddText(w);

  return (int) numtowrite;
}


//**********************************************************************
// errprintf
// ---------
// Write to standard error.
// For graphical apps both printf and errprintf display the text in the
// same window.
//**********************************************************************

int CRhsIO::errprintf(const WCHAR *Format, ...)
{ BOOL b;
  DWORD numtowrite, numwritten;
  WCHAR w[MAX_PRINTFBUFLEN];
  char s[MAX_PRINTFBUFLEN];
  va_list ap;

// Build the output string

  va_start(ap, Format);
  numtowrite = vswprintf(w, MAX_PRINTFBUFLEN-1, Format, ap);
  va_end(ap);

// We always output ANSI not UNICODE

  WideCharToMultiByte(CP_ACP, 0, w, -1, s, MAX_PRINTFBUFLEN-1, NULL, 0);

// For console mode apps write to the standard output stream

  if (m_IsConsole)
  { b = WriteFile(m_stderr, s, numtowrite, &numwritten, NULL);

    if (!b)
    { FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, m_LastError, LEN_LASTERRBUF, NULL);
      return -1;
    }

    return (int) numwritten;
  }

// For GUI apps append the text to the display window

  m_hIOWnd->AddText(w);

  return (int) numtowrite;
}


// *********************************************************************
// SetLastError
// ---------
// Set the last error message that LastError will return.
// This uses the same format as printf etc.
// *********************************************************************

void CRhsIO::SetLastError(const WCHAR* ErrMsg, ...)
{ va_list ap;

// Build the output string

  va_start(ap, ErrMsg);
  vswprintf(m_LastError, LEN_LASTERRBUF, ErrMsg, ap);
  va_end(ap);
}


// *********************************************************************
// LastError
// ---------
// Copy the last error message to the provided buffer
// *********************************************************************

const WCHAR* CRhsIO::LastError(WCHAR* ErrBuf, int BufLen)
{
  lstrcpyn(ErrBuf, m_LastError, BufLen);
  return ErrBuf;
}


// *********************************************************************
// ParseCmdLine
// ------------
// Function to take a string and parse it into the usual C argc and
// argv[] variables.
// This code is adapted from the Visual C++ 9 run time library source.
//  *cmdstart - pointer to command line
//  **argv    - argv array
//  *argc     - returns number of argv entries created
//  *args     - buffer for argument text
//  *numchars - number of characters used in args buffer
// *********************************************************************

#define NULCHAR    '\0'
#define DQUOTECHAR '"'
#define SPACECHAR  ' '
#define TABCHAR    '\x09'
#define SLASHCHAR  '\\'

void CRhsIO::ParseCmdLine(WCHAR *cmdstart, WCHAR **argv, int *argc, WCHAR *args, int *numchars)
{ int inquote;       // 1 = inside quotes
  int copychar;      // 1 = copy char to *args
  unsigned numslash; // num of backslashes seen
  WCHAR *p;
  WCHAR c;

  *numchars = 0;
  *argc = 1;         // the program name at least

// First scan the program name, copy it, and count the bytes

  p = cmdstart;
  if (argv)
    *argv++ = args;

// A quoted program name is handled here. The handling is much simpler
// than for other arguments. Basically, whatever lies between the leading
// double-quote and next one, or a terminal null character is simply
// accepted. Fancier handling is not required because the program name must
// be a legal NTFS/HPFS file name. Note that the double-quote characters
// are not copied, nor do they contribute to numchars.

  inquote = FALSE;

  do
  { if (*p == DQUOTECHAR )
    { inquote = !inquote;
      c = (WCHAR) *p++;
      continue;
    }

    ++*numchars;
    if (args)
      *args++ = *p;

    c = (WCHAR) *p++;
  } while (c != NULCHAR && (inquote || (c !=SPACECHAR && c != TABCHAR)));

// If we haven't hit a \0 append \0 to the first argument

  if ( c == NULCHAR )
  { p--;
  }
  else
  { if (args)
      *(args-1) = NULCHAR;
  }

  inquote = 0;

// Now go through the rest of the command line. There is extra processing
// required for arguments. \" embeds a quote, and "" embeds a quote if
// and only if it's in quoted text.

  for(;;)
  {

// Step past spaces and tabs

    if (*p)
    { while (*p == SPACECHAR || *p == TABCHAR)
        ++p;
    }

// If we've reached the end of the string stop looping

    if (*p == NULCHAR)
      break;

// Got an argument

    if (argv)
      *argv++ = args;
    ++*argc;

// Loop through scanning one argument

    for (;;)
    { copychar = 1;

// Rules: 2N backslashes + " ==> N backslashes and begin/end quote
//        2N+1 backslashes + " ==> N backslashes + literal "
//        N backslashes ==> N backslashes

      numslash = 0;
      while (*p == SLASHCHAR)
      {

// Count number of backslashes for use below

        ++p;
        ++numslash;
      }

// Process double quotes

      if (*p == DQUOTECHAR)
      {
                /* if 2N backslashes before, start/end quote, otherwise
                    copy literally */
        if (numslash % 2 == 0)
        { if (inquote && p[1] == DQUOTECHAR)
          { p++;    /* Double quote inside quoted string */
          }
          else
          {    /* skip first quote char and copy second */
            copychar = 0;       /* don't copy quote */
            inquote = !inquote;
          }
        }
        numslash /= 2;          /* divide numslash by two */
      }

// Copy slashes

      while (numslash--)
      { if (args)
          *args++ = SLASHCHAR;
        ++*numchars;
      }

// If at end of arg, break loop

      if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
        break;

// Copy character into argument

      if (copychar)
      { if (args)
          *args++ = *p;
        ++*numchars;
      }
      ++p;
    }

// Null-terminate the argument

    if (args)
      *args++ = NULCHAR;
    ++*numchars;
  }
}


// *********************************************************************
// GetParentPID
// ------------
// Find the parent process ID and filename
// Return zero if the parent cannot be found
// *********************************************************************

DWORD CRhsIO::GetParentPID(WCHAR* ParentName)
{ DWORD our_pid, parent_pid;
  HANDLE snapshot ;
  PROCESSENTRY32 procentry;

// First find our PID and use that to get the parent PID

  our_pid = GetCurrentProcessId();
  parent_pid = 0;

// Get a handle to a Toolhelp snapshot of the systems processes

  snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

// Loop over all processes looking for ourselves

  memset((LPVOID)&procentry,0,sizeof(PROCESSENTRY32));
  procentry.dwSize = sizeof(PROCESSENTRY32) ;
  Process32First(snapshot, &procentry);

  do
  { if (our_pid == procentry.th32ProcessID)
    { parent_pid = procentry.th32ParentProcessID;
      break;
    }

    procentry.dwSize = sizeof(PROCESSENTRY32) ;
  } while (Process32Next(snapshot, &procentry));

// Now loop over all processes looking for our parent

  memset((LPVOID)&procentry,0,sizeof(PROCESSENTRY32));
  procentry.dwSize = sizeof(PROCESSENTRY32) ;
  Process32First(snapshot, &procentry);

  do
  { if (parent_pid == procentry.th32ProcessID)
    { lstrcpy(ParentName, procentry.szExeFile);
      break;
    }

    procentry.dwSize = sizeof(PROCESSENTRY32) ;
  } while (Process32Next(snapshot, &procentry));

// Return the parent PID.
// This will be zero if the parent was not found.

   return parent_pid;
}


//**********************************************************************
// CRhsIOWnd
// ===========
// Implementation of main window
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

LRESULT CALLBACK CRhsIOWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


//**********************************************************************
// CRhsIOWnd::CRhsIOWnd
// ------------------------
//**********************************************************************

CRhsIOWnd::CRhsIOWnd(void)
{
  m_hInstance = NULL;
  lstrcpy(m_AppTitle, L"");

  m_hWnd       = NULL;
  m_hAccel     = NULL;

  m_hEditWnd   = NULL;
  m_EditFont   = NULL;

  m_Timer      = 0;
  m_TimerCount = 0;

  m_hSearchDlg = NULL;
  m_SearchMsg  = 0;
  lstrcpy(m_SearchDlgText, L"");

  m_hMainThread  = NULL;
  m_ThreadComplete = FALSE;

  m_DisplayFull = FALSE;
  m_BufFile     = NULL;
  lstrcpy(m_BufFileName, L"");

  m_Registry  = NULL;

  lstrcpy(m_LastError, L"");
}


CRhsIOWnd::~CRhsIOWnd(void)
{
  if (m_hWnd)
    DestroyWindow(m_hWnd);

  if (m_Registry)
    delete m_Registry;
}


//**********************************************************************
// CRhsIOWnd::Open
// -----------------
//**********************************************************************

BOOL CRhsIOWnd::Open(HINSTANCE hInstance, WCHAR* Title, int Left, int Top, int Width, int Depth)
{ WNDCLASSEX wce;
  RECT r;
  WCHAR tempdir[MAX_PATH+1];

  lstrcpy(m_LastError, L"");

// If the window has already been opened then just resize it

  if (m_hWnd)
  { MoveWindow(m_hWnd, Left, Top, Width, Depth, TRUE);
    return(TRUE);
  }

// Save the instance handle

  m_hInstance = hInstance;
  lstrcpy(m_AppTitle, Title);

// Load the accelerators

  m_hAccel = LoadAccelerators(m_hInstance, L"RhsIOAccel");

// Register the class

  wce.cbSize        = sizeof(WNDCLASSEX);
  wce.style         = 0;
  wce.lpfnWndProc   = CRhsIOWndProc;
  wce.cbClsExtra    = 0;
  wce.cbWndExtra    = sizeof(LONG_PTR);
  wce.hInstance     = m_hInstance;
  wce.hIcon         = LoadIcon(m_hInstance, L"CRhsIOIcon");
  wce.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wce.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wce.lpszMenuName  = L"CRhsIOMenu";
  wce.lpszClassName = L"CRhsIOWndClass";
  wce.hIconSm       = (HICON) LoadImage(m_hInstance, L"CRhsIOIconSmall", IMAGE_ICON, 16, 16, 0);

  RegisterClassEx(&wce);

// Create the registry object

  m_Registry = new CRhsRegistry(L"Software\\Rhs\\CRhsIO");

// Check if default size is required

  r.left   = Left  == CW_USEDEFAULT ? m_Registry->GetInt(L"Left",  CW_USEDEFAULT) : Left;
  r.top    = Top   == CW_USEDEFAULT ? m_Registry->GetInt(L"Top",   CW_USEDEFAULT) : Top;
  r.right  = Width == CW_USEDEFAULT ? m_Registry->GetInt(L"Width", CW_USEDEFAULT) : Width;
  r.bottom = Depth == CW_USEDEFAULT ? m_Registry->GetInt(L"Depth", CW_USEDEFAULT) : Depth;

// Create the window

  m_hWnd = CreateWindow(L"CRhsIOWndClass",
                        Title,
                        WS_OVERLAPPEDWINDOW,
                        r.left, r.top, r.right, r.bottom,
                        NULL,
                        NULL,
                        m_hInstance,
                        NULL
                       );

  if (!m_hWnd)
  { FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, m_LastError, LEN_LASTERRBUF, NULL);
    return FALSE;
  }

  FitOnScreen(TRUE, TRUE);

// Create the popup menu

  m_PopMenu = CreatePopupMenu();

  AppendMenu(m_PopMenu, MF_STRING, IDM_POPUP_COPY, L"Copy");
  AppendMenu(m_PopMenu, MF_STRING, IDM_POPUP_SEARCH, L"Search");
  AppendMenu(m_PopMenu, MF_STRING, IDM_POPUP_URL, L"Open &URL");

// Save our object in the parent windows extra data

  SetWindowLongPtr(m_hWnd, 0, (LONG_PTR) this);

// Load the rich edit library

  LoadLibrary(L"riched20.dll");

// Create the child edit control

  GetClientRect(m_hWnd, &r);

  m_hEditWnd = CreateWindow(RICHEDIT_CLASS, L"",
                            WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
                            ES_MULTILINE | ES_READONLY | ES_NOHIDESEL |
                            ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                            0, 0, r.right, r.bottom,
                            m_hWnd,
                            (HMENU) IDC_EDIT,
                            m_hInstance,
                            NULL
                           );

  if (!m_hEditWnd)
  { FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, m_LastError, LEN_LASTERRBUF, NULL);
    MessageBox(m_hWnd, m_LastError, m_AppTitle, MB_OK);
    return FALSE;
  }

  SendMessage(m_hEditWnd, EM_LIMITTEXT, 0, 0);
  SendMessage(m_hEditWnd, EM_SETBKGNDCOLOR, 0, 0x00FFFFFF);

// Load the font for the edit window

  m_EditFont = new CRhsFont;

  m_EditFont->SetFont(-11, 0, 0, 0,
                      FW_NORMAL, FALSE, FALSE, FALSE,
                      ANSI_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
                      DEFAULT_QUALITY, FIXED_PITCH, L"Lucida Console"
                     );
  m_EditFont->ReadFont(m_Registry);

  SendMessage(m_hEditWnd, WM_SETFONT, (WPARAM) m_EditFont->hFont(), 0L);

// Show the window

  ShowWindow(m_hWnd, SW_SHOWNORMAL);
  UpdateWindow(m_hWnd);
  SetFocus(m_hEditWnd);

// Create the buffer file

  GetTempPath(MAX_PATH, tempdir);
  GetTempFileName(tempdir, L"RhsIO", 0, m_BufFileName);
  m_BufFile = CreateFile(m_BufFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);

// Create a half second timer for the main window

  m_Timer = SetTimer(m_hWnd, 1, 500, NULL);

// Register a message for the Find dialog

  m_SearchMsg = RegisterWindowMessage(FINDMSGSTRING);

// Return indicating success

  return TRUE;
}


//**********************************************************************
// AddText
// -------
// Add text to the edit control
//**********************************************************************

BOOL CRhsIOWnd::AddText(WCHAR* Text)
{ int curlen, textlen;
  DWORD d;
  char s[1024];

// Write the text to the buffer file. Data is always written as ANSI.

  if (m_BufFile != INVALID_HANDLE_VALUE)
  { WideCharToMultiByte(CP_ACP, 0, Text, -1, s, 1024, NULL, 0);
    WriteFile(m_BufFile, s, lstrlenA(s), &d, NULL);
  }

// If the display is full stop now

  if (m_DisplayFull)
    return TRUE;

// Graphical IO: Check that the new text will fit.
// If the text won't fit append a "Display full" warning.

  curlen = (int) SendMessage(m_hEditWnd, WM_GETTEXTLENGTH, 0, 0);
  textlen = lstrlen(Text);

  if (curlen + textlen > LEN_DISPLAYBUF - LEN_TXT_BUFFULL)
  { SendMessage(m_hEditWnd, EM_SETSEL, curlen, curlen);
    SendMessage(m_hEditWnd, EM_REPLACESEL, 0, (LPARAM) TXT_BUFFULL);
    SendMessage(m_hEditWnd, EM_SETSEL, curlen, curlen);

    m_DisplayFull = TRUE;

    return TRUE;
  }

// Add the new text to the edit control

  SendMessage(m_hEditWnd, EM_SETSEL, curlen, curlen);
  SendMessage(m_hEditWnd, EM_REPLACESEL, 0, (LPARAM) Text);
  SendMessage(m_hEditWnd, EM_SETSEL, curlen, curlen);

// Return indicating success

  return TRUE;
}


//**********************************************************************
// KillWorkerThread
// ----------------
// Kill the worker thread
//**********************************************************************

int CRhsIOWnd::KillWorkerThread(void)
{

  if (!m_ThreadComplete)
    if (m_hMainThread)
      m_RhsIO->SetAbort(TRUE);

  return 0;
}


//**********************************************************************
// SaveBufFile
// -----------
// Select the buffer file to a location of the users choice
//**********************************************************************

int CRhsIOWnd::SaveBufFile(void)
{ OPENFILENAME ofn;
  WCHAR filename[MAX_PATH+1];

// Get the new file name

  lstrcpy(filename, L"");

  ofn.lStructSize       = sizeof(OPENFILENAME);
  ofn.hwndOwner         = m_hWnd;
  ofn.hInstance         = m_hInstance;
  ofn.lpstrFilter       = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
  ofn.lpstrCustomFilter = NULL;
  ofn.nFilterIndex      = 1;
  ofn.lpstrFile         = filename;
  ofn.nMaxFile          = MAX_PATH;
  ofn.lpstrFileTitle    = m_AppTitle;
  ofn.nMaxFileTitle     = MAX_PATH;
  ofn.lpstrInitialDir   = NULL;
  ofn.lpstrTitle        = m_AppTitle;
  ofn.Flags             = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
  ofn.lpstrDefExt       = L"txt";

  if (!GetSaveFileName(&ofn))
    return 0;

// Copy the buffer file to the requested location

  if (!CopyFile(m_BufFileName, filename, FALSE))
  { FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, m_LastError, LEN_LASTERRBUF, NULL);
    MessageBox(m_hWnd, m_LastError, m_AppTitle, MB_OK);
    return 0;
  }

// All done

  return 0;
}


//**********************************************************************
// SearchOutput
// ------------
// Search the output written to the edit control
//**********************************************************************

BOOL CRhsIOWnd::SearchOutput(void)
{ int len;
  MSG msg;
  FINDREPLACE fr;

// Only one find at a time is allowed

  if (m_hSearchDlg)
    return(FALSE);

// Check there is something to search

  len = GetWindowTextLength(m_hEditWnd);

  if (len == 0)
    return TRUE;

// Initialise the findreplace structure

  fr.lStructSize    = sizeof(FINDREPLACE);
  fr.hwndOwner      = m_hWnd;
  fr.hInstance      = NULL;
  fr.Flags          = FR_DOWN;
  fr.lpstrFindWhat  = m_SearchDlgText;
  fr.wFindWhatLen   = LEN_SEARCHDLGTEXT;
  fr.lCustData      = 0;
  fr.lpfnHook       = NULL;
  fr.lpTemplateName = NULL;

// Create the find dialog

  m_hSearchDlg = FindText(&fr);

  if (!m_hSearchDlg)
    return FALSE;

// Use a local message loop to process dialog messages properly

  while (m_hSearchDlg)
  { if (GetMessage(&msg, NULL, 0, 0))
    { if (!IsDialogMessage(m_hSearchDlg, &msg))
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  m_hSearchDlg = NULL;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// OnSearchMsg
// -----------
// Process messages sent by a Find dialog
//**********************************************************************

long CRhsIOWnd::OnSearchMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ int offset;
  WPARAM flags;
  FINDREPLACE FAR *fr;
  FINDTEXTW ft;

// Check it is really a find/replace message

  if (message != m_SearchMsg)
    return(FALSE);

// Copy the FINDREPLACE block

  fr = (FINDREPLACE*) lParam;

// DIALOGTERM means dialog was closed

  if (fr->Flags & FR_DIALOGTERM)
  { m_hSearchDlg = NULL;
    return 0;
  }

// Find next occurrence of search string

  if (fr->Flags & FR_FINDNEXT)
  { SendMessage(m_hEditWnd, EM_GETSEL, (LPARAM) NULL, (LPARAM) &offset);
    if (fr->Flags & FR_DOWN)
      offset++;
    else
      offset--;
    if (offset < 0)
      offset++;
    SendMessage(m_hEditWnd, EM_SETSEL, offset, offset);

    ft.chrg.cpMin = offset;
    ft.chrg.cpMax = -1;

    flags = 0;
    if (fr->Flags & FR_MATCHCASE) flags |= FR_MATCHCASE;
    if (fr->Flags & FR_WHOLEWORD) flags |= FR_WHOLEWORD;
    if (fr->Flags & FR_DOWN)      flags |= FR_DOWN;

    ft.lpstrText  = fr->lpstrFindWhat;

    offset = (int) SendMessage(m_hEditWnd, EM_FINDTEXTW, flags, (LPARAM) &ft);
    if (offset == -1)
    { MessageBox(m_hSearchDlg, L"No matches found", m_AppTitle, MB_OK);
    }
    else
    { SendMessage(m_hEditWnd, EM_SETSEL, offset, offset + lstrlen(fr->lpstrFindWhat));
      SendMessage(m_hEditWnd, EM_SCROLLCARET, 0, 0);
    }
  }

// All done

  return 0;
}


//**********************************************************************
// ChooseFont
// ----------
// Select the font for the edit control
//**********************************************************************

int CRhsIOWnd::ChooseFont(void)
{

  if (m_EditFont->ChooseFont(m_hWnd, m_AppTitle))
  { m_EditFont->SaveFont(m_Registry);
    SendMessage(m_hEditWnd, WM_SETFONT, (WPARAM) m_EditFont->hFont(), 0L);
  }

  return 0;
}


//**********************************************************************
// CRhsIOWnd::WebSearch
// ----------------------
// Launch a web search for the selected text
//**********************************************************************

// We're going to hard code in a maximum length for the search text
// The text limit is half this value to allow room for URL escaping
#define MAX_SEARCHLEN 256

void CRhsIOWnd::WebSearch(void)
{
  DWORD len_escapedtext = MAX_SEARCHLEN;
  WCHAR seltext[MAX_SEARCHLEN];
  WCHAR escapedtext[MAX_SEARCHLEN];
  WCHAR url[MAX_SEARCHLEN+32];

// Check the length of the selection

  DWORD selstart, selend;
  SendMessage(m_hEditWnd, EM_GETSEL, (WPARAM) &selstart, (LPARAM) &selend);
  if (selend == selstart)
    return;
  if (selend - selstart > MAX_SEARCHLEN/2)
  {
    m_RhsIO->printf(L"The current selection is too large for a web search");
    return;
  }

// Get the selected text

  SendMessage(m_hEditWnd, EM_GETSELTEXT, 0, (LPARAM) seltext);

// And we need to escape the text

  if (UrlEscape(seltext, escapedtext, &len_escapedtext, 0) != S_OK)
  {
    m_RhsIO->printf(L"The current selection is too large for a web search");
    return;
  }

// Set the search URL

  lstrcpy(url, L"https://www.google.com/search?q=");
  lstrcat(url, escapedtext);

// Use the shell to launch the search

  INT_PTR h = (INT_PTR) ShellExecute(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
  if (h <= 32)
  {
    m_RhsIO->printf(L"The search failed. The following info may be useful for debugging: %x %s\n", h, url);
    return;
  }
}


//**********************************************************************
// CRhsIOWnd::OpenURL
// --------------------
// Open the selected text as a URL
//**********************************************************************

void CRhsIOWnd::OpenURL(void)
{
  DWORD len_url = MAX_SEARCHLEN;
  WCHAR url[MAX_SEARCHLEN];

// Check the length of the selection

  DWORD selstart, selend;
  SendMessage(m_hEditWnd, EM_GETSEL, (WPARAM) &selstart, (LPARAM) &selend);
  if (selend == selstart)
    return;
  if (selend - selstart > MAX_SEARCHLEN)
  {
    m_RhsIO->printf(L"The current selection is too large to open as a URL");
    return;
  }

// Get the selected text and check it's a URL

  SendMessage(m_hEditWnd, EM_GETSELTEXT, 0, (LPARAM) url);
  if (strcmpleft(L"http://", url) != 0 && strcmpleft(L"https://", url) != 0)
  {
    m_RhsIO->printf(L"The selected text:\n%s\nis not a URL", url);
    return;
  }

// Use the shell to open the URL

  INT_PTR h = (INT_PTR) ShellExecute(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
  if (h <= 32)
  {
    m_RhsIO->printf(L"The search failed. The following info may be useful for debugging: %x %s\n", h, url);
    return;
  }
}


//**********************************************************************
// CRhsIOWnd::OnTimer
// ------------------
// Handle WM_TIMER messages
//**********************************************************************

long CRhsIOWnd::OnTimer(void)
{ int i;
  WCHAR s[256];

// If the worker thread has completed there is nothing to do

  if (m_ThreadComplete)
    return 0;

// Check if the thread has completed

  if (WaitForSingleObject(m_hMainThread, 0) == WAIT_OBJECT_0)
  { m_ThreadComplete = TRUE;
    CloseHandle(m_hMainThread);
    m_hMainThread = NULL;

    swprintf(s, 255, L"%s - completed", m_AppTitle);
    SetWindowText(m_hWnd, s);

    KillTimer(m_hWnd, m_Timer);
    return 0;
  }

// Thread is still active

  swprintf(s, 255, L"%s - working ....", m_AppTitle);

  m_TimerCount++;
  if (m_TimerCount > 3) m_TimerCount = 0;
  i = lstrlen(s) - (3 - m_TimerCount);
  s[i] = 0;

  SetWindowText(m_hWnd, s);

// All donw   e

  return 0;
}


//**********************************************************************
// CRhsIOWnd::FitOnScreen
// ------------------------
// Adjust the window size and position so it fits on the desktop
//**********************************************************************

BOOL CRhsIOWnd::FitOnScreen(BOOL Origin, BOOL Size)
{ int width, depth;
  RECT r;

// Check the window exists

  if (!m_hWnd)
    return(FALSE);

// Get some dimensions

  GetWindowRect(m_hWnd, &r);
  width = GetSystemMetrics(SM_CXSCREEN);
  depth = GetSystemMetrics(SM_CYSCREEN);

// If required move the window so its top left corner is on screen

  if (Origin)
  { r.left = r.left >= 0 ? r.left : 0;
    r.top  = r.top >= 0 ? r.top : 0;
    r.left = r.left < width ? r.left : width - 96;
    r.top  = r.top < depth ? r.top : depth - 96;
    SetWindowPos(m_hWnd, NULL, r.left, r.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  }

// If required resize and move the window so it's wholly on screen

  if (Size)
  { r.right  -= r.left;
    r.bottom -= r.top;
    r.right  = r.right <= width ? r.right : width;
    r.bottom = r.bottom <= depth ? r.bottom : depth;
    r.left = r.left + r.right < width ? r.left : width - r.right;
    r.top  = r.top + r.bottom < depth ? r.top  : depth - r.bottom;
    SetWindowPos(m_hWnd, NULL, r.left, r.top, r.right, r.bottom, SWP_NOZORDER);
  }

// Return indicating success

  return(TRUE);
}


//**********************************************************************
// CRhsIOWnd::OnSize
// -------------------
// Handle WM_SIZE
//**********************************************************************

LRESULT CRhsIOWnd::OnSize(UINT fwSizeType)
{ RECT r;

// Save the window size to the registry

  if (fwSizeType == SIZE_RESTORED)
  { GetWindowRect(m_hWnd, &r);
    m_Registry->WriteInt(L"Left",  r.left);
    m_Registry->WriteInt(L"Top",   r.top);
    m_Registry->WriteInt(L"Width", r.right - r.left);
    m_Registry->WriteInt(L"Depth", r.bottom - r.top);
  }

// Resize the edit control

  GetClientRect(m_hWnd, &r);
  MoveWindow(m_hEditWnd, 0, 0, r.right, r.bottom, TRUE);

// Done

  return 0;
}


//**********************************************************************
// CRhsIOWnd::OnMove
// -------------------
// Handle WM_MOVE
//**********************************************************************

LRESULT CRhsIOWnd::OnMove(void)
{ RECT r;

  if (!IsIconic(m_hWnd) && !IsZoomed(m_hWnd))
  { GetWindowRect(m_hWnd, &r);
    m_Registry->WriteInt(L"Left", r.left);
    m_Registry->WriteInt(L"Top", r.top);
  }

  return 0;
}


//**********************************************************************
// CRhsIOWnd::OnPaint
// --------------------
// Paint the window contents
//**********************************************************************

LRESULT CRhsIOWnd::OnPaint(HDC hDC)
{
  return 0;
}


//**********************************************************************
// CRhsIOWnd::OnDestroy
// ----------------------
// Windows is about to be destroyed
//**********************************************************************

LRESULT CRhsIOWnd::OnDestroy(void)
{

// Close the buffer file

  CloseHandle(m_BufFile);

// Signal we are exiting the app

  PostQuitMessage(0);
  return 0;
}


//**********************************************************************
// CRhsIOWnd::OnCommand
// ----------------------
// Handle WM_COMMAND messages
//**********************************************************************

LRESULT CRhsIOWnd::OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl)
{

// Process the message

  switch (wID)
  {

// Kill the worker thread

    case IDM_FILE_KILL:
      KillWorkerThread();
      return 0;

// Save the text

    case IDM_FILE_SAVEAS:
      SaveBufFile();
      return 0;

// Exit

    case IDM_FILE_EXIT:
      DestroyWindow(m_hWnd);
      return 0;

// Search the text

    case IDM_EDIT_FIND:
      SearchOutput();
      return 0;

// Choose the font

    case IDM_OPTIONS_FONT:
      ChooseFont();
      return 0;

  // Web search for the text from the edit control

    case IDM_POPUP_SEARCH:
      WebSearch();
      return 0;

// Open selected text as a URL

    case IDM_POPUP_URL:
      OpenURL();
      return 0;

// Copy text from the edit control

    case IDM_POPUP_COPY:
      SendMessage(m_hEditWnd, WM_COPY, 0, 0);
      return 0;
  }

  return 0;
}


//**********************************************************************
// CRhsIOWnd::OnInitMenuPopup
// ==========================
// Initialise the menus
//**********************************************************************

LRESULT CRhsIOWnd::OnInitMenuPopup(HMENU hPopup, UINT uPos, BOOL fSystemMenu)
{

  EnableMenuItem(hPopup, IDM_FILE_KILL, MF_BYCOMMAND | (!m_ThreadComplete && m_hMainThread ? MF_ENABLED : MF_GRAYED));

  return 0;
}


//**********************************************************************
// CRhsIOWnd::OnRightClick
// -----------------------
// Display the right click menu
//**********************************************************************

void CRhsIOWnd::OnRightClick(int x, int y)
{
  CHARRANGE r;

// If there is no selection do nothing

  SendMessage(m_hEditWnd, EM_EXGETSEL, 0, (LPARAM) &r);
  if (r.cpMin != r.cpMax)
  {
    POINT p;

// Get the position of the click
    p.x = x;
    p.y = y;
    ClientToScreen(m_hWnd, &p);

// Work out how to align the menu

    UINT flags = 0;

    if (p.x > GetSystemMetrics(SM_CXSCREEN)/2)
    {
      flags |= TPM_RIGHTALIGN;
      p.x += 1;
    }

    if (p.y > GetSystemMetrics(SM_CYSCREEN)/2)
    {
      flags |= TPM_BOTTOMALIGN;
      p.y += 1;
    }

// And show the menu

    TrackPopupMenu(m_PopMenu, flags, p.x, p.y, 0, m_hWnd, NULL);
  }
}


//**********************************************************************
// OnMessage
// ---------
// Message handler
//**********************************************************************

LRESULT CRhsIOWnd::OnMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ HDC hdc;
  PAINTSTRUCT ps;

// Message might be from a Find dialog

  if (message == m_SearchMsg)
    return OnSearchMsg(hWnd, message, wParam, lParam);

// Process messages for the main window

  switch (message)
  {

// Message from menu or control

    case WM_COMMAND:
      return OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);

// Initialise the menus

    case WM_INITMENUPOPUP:
      return OnInitMenuPopup((HMENU) wParam, (UINT) LOWORD(lParam), (BOOL) HIWORD(lParam));

// Detect a right click in the edit control

    case WM_PARENTNOTIFY:
      if (LOWORD(wParam) == WM_RBUTTONDOWN)
        OnRightClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return 0;

// Timer tick

    case WM_TIMER:
      return OnTimer();

// Window moved

    case WM_MOVE:
      return OnMove();

// Window resized

    case WM_SIZE:
      return OnSize((UINT) wParam);

// Window needs repainting

    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      OnPaint(hdc);
      EndPaint(hWnd, &ps);
      return 0;

// Pass the focus on to the edit control

    case WM_SETFOCUS:
      SetFocus(m_hEditWnd);
      return 0;

// Window destroyed

    case WM_DESTROY:
      return OnDestroy();
  }

// Pass unhandled messages to Windows

  return DefWindowProc(hWnd, message, wParam, lParam);
}


// *********************************************************************
// TranslateAccelerator
// --------------------
// Translate accelerator mesages for the main window
// *********************************************************************

int CRhsIOWnd::TranslateAccelerator(LPMSG Msg)
{ int retcode;

  retcode = ::TranslateAccelerator(m_hWnd, m_hAccel, Msg);

  return retcode;
}


// *********************************************************************
// LastError
// ---------
// Copy the last error message to the provided buffer
// *********************************************************************

const WCHAR* CRhsIOWnd::LastError(WCHAR* ErrBuf, int BufLen)
{
  lstrcpyn(ErrBuf, m_LastError, BufLen);
  return ErrBuf;
}


//**********************************************************************
// CRhsIOWndProc
// ---------------
// Message handler
//**********************************************************************

LRESULT CALLBACK CRhsIOWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ CRhsIOWnd* mainwnd;

// Get the object

  mainwnd = (CRhsIOWnd*) GetWindowLongPtr(hWnd, 0);
  if (!mainwnd)
    return(DefWindowProc(hWnd, message, wParam, lParam));

// Handle the message

  return(mainwnd->OnMessage(hWnd, message, wParam, lParam));
}


//**********************************************************************
// strcmpleft
// ----------
// Compare s1 with the first len(characters) of s2
//**********************************************************************

int strcmpleft(const WCHAR* s1, const WCHAR* s2)
{
  for (int i = 0; s1[i] != '\0'; i++)
  {
    if (s1[i] < s2[i])
      return -1;
    else if (s1[i] > s2[i])
      return 1;
  }
  return 0;
}