//**********************************************************************
// httpgrabber
// ===========
//
// Grab data from an HTTP server
//
// Syntax: httpgrabber <server> <url> [<output> [<username> <password>]]
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 18th June 2009
//**********************************************************************

#include <windows.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <CRhsSocket/CRhsSocket.h>


//**********************************************************************
// Prototypes
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);

void Base64Encode(WCHAR* Input, char* Output);
char* wstrtoastr(const WCHAR* w, char* a, int len);
BOOL strtoint(const WCHAR* s, int* i);
const WCHAR* GetLastErrorMessage(void);


//**********************************************************************
// Global variables
//**********************************************************************

CRhsIO RhsIO;


//**********************************************************************
// Global variables
//**********************************************************************

#define SYNTAX \
L"httpgrabber [-pdata] <server> <path> [<output> [<username> <password>]]\r\n"

#define HELP \
L"httpgrabber [-pdata] <server> <path> [<output> [<username> <password>]]\r\n\r\n"         \
L"Grab data from an http server\r\n"                                            \
L"  e.g. httpgrabber www.microsoft.com /\r\n\r\n"                               \
L"By default the GET method is used. The -p option sets the method to POST\r\n" \
L"and sends the data following the -p.\r\n\r\n"                                     \
L"If <output> is omitted the output is written to the console.  To specify\r\n" \
L"a username and password and still write output to the console set the\r\n"    \
L"output to '.' e.g.\r\n"                                                       \
L"  httpgrabber www.microsoft.com / . myusername mypassword\r\n\r\n"            \
L"To use a proxy just use the command in the form:\r\n"                         \
L"  httpgrabber myproxy.com http://www.whatever.com/\r\n"                       \
L"that is use the proxy name as the server and use a full URL as the path.\r\n"

#define ARG_SERVER RhsIO.m_argv[argnum]
#define ARG_PATH   RhsIO.m_argv[argnum+1]
#define ARG_OUTPUT RhsIO.m_argv[argnum+2]
#define ARG_USER   RhsIO.m_argv[argnum+3]
#define ARG_PASS   RhsIO.m_argv[argnum+4]


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{
  int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"httpgrabber"))
  {
     wprintf(L"Error initialising: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  retcode = RhsIO.Run(rhsmain);

  return retcode;
}


//**********************************************************************
// Here we go
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused)
{
  int port, i;
  FILE* out;
  CRhsSocket skt;
  BOOL usepassword, usepost = FALSE;
  // The socket needs data writing as ASCII
  char *server, *postdata, password[256];
  // Buffers
#define LEN_BUF 1024
  char s[LEN_BUF];
  WCHAR w[LEN_BUF+1]; // +1 so it can hold a null terminated string

// Process flags

  int argnum = 1;

  while (argnum < RhsIO.m_argc)
  {
    if (RhsIO.m_argv[argnum][0] != '-')
      break;

    // Flags are case insensitive
    CharUpperBuff(RhsIO.m_argv[argnum]+1, 1);

    switch (RhsIO.m_argv[argnum][1])
    {
      case '?':
        RhsIO.printf(HELP);
        return 0;

      case 'P':
        // The POST data needs to be sent as ASCII not UNICODE
        postdata = wstrtoastr(RhsIO.m_argv[argnum]+2, NULL, 0);
        usepost = TRUE;
        break;

      default:
        RhsIO.printf(L"httpgrabber: Unknown flag \"%s\".\r\n%s", RhsIO.m_argv[argnum], SYNTAX);
        return 1;
    }

    argnum++;
  }

// Check there are at least two arguments left

  if (RhsIO.m_argc - argnum < 2)
  {
    RhsIO.printf(L"httpgrabber: Not enough arguments were given.\r\n%s", SYNTAX);
    return 2;
  }

// If required split the server into the server name and port

  port = 80;

  for (i = 0; ARG_SERVER[i] != ':' && ARG_SERVER[i] != '\0'; i++);

  if (ARG_SERVER[i] == ':')
  {
    ARG_SERVER[i] = 0;

    if (!strtoint(ARG_SERVER+i+1, &port))
    { RhsIO.printf(L"\"%s\" is not a valid port number\r\n", ARG_SERVER+i+1);
      return 2;
    }
  }

// Open the output stream

  out = stdout;

  if (RhsIO.m_argc - argnum >= 3)
  {
    if (lstrcmp(ARG_OUTPUT, L".") != 0)
    {
      if (_wfopen_s(&out, ARG_OUTPUT, L"wb") != 0)
      {
        RhsIO.printf(L"Cannot open output file \"%s\":\r\n%s", ARG_OUTPUT, GetLastErrorMessage());
        return 2;
      }
    }
  }

// If a username and password were supplied process them

  usepassword = FALSE;

  if (RhsIO.m_argc - argnum >= 5)
  {
    swprintf(w, LEN_BUF, L"%s:%s", ARG_USER, ARG_PASS);
    Base64Encode(w, password);
    usepassword = TRUE;
  }

// OK open the connection

  RhsIO.errprintf(L"Connecting to %s on port %i ...\r\n", ARG_SERVER, port);
  skt.Startup();

  server = wstrtoastr(ARG_SERVER, NULL, 0);
  if (!skt.Connect(server, port))
  {
    RhsIO.errprintf(L"Cannot open %s\r\n", ARG_SERVER);
    if (out != stdout)
    {
      fclose(out);
      DeleteFile(ARG_OUTPUT);
    }
    free(server);
    return 2;
  }
  free(server);

  RhsIO.errprintf(L"Connected\r\n\r\n");

// Send the header

  WCHAR* method;
  if (usepost)
    method = L"POST";
  else
    method = L"GET";

  swprintf(w, LEN_BUF,
           L"%s %s HTTP/1.0\r\n"
           L"Host: %s\r\n"
           L"User-Agent: Rawhttpgrabber\r\n"
           L"Accept: */*\r\n",
           method, ARG_PATH, ARG_SERVER
          );
  char* request = wstrtoastr(w, NULL, 0);
  skt.printf(request);
  free(request);

  if (usepassword)
    skt.printf("Authorization: Basic %s\r\n", password);

// If we are using POST send the data

  if (usepost)
    skt.printf("Content-Length: %d\r\n\r\n%s", lstrlenA(postdata), postdata);
  else
    skt.printf("\r\n");

// Read the response

  while ((i = skt.read(s, 1, LEN_BUF)) > 0)
  {
    if (out != stdout)
    {
      fwrite(s, 1, i, out);
      RhsIO.printf(L".");
    }
    else
    {
      MultiByteToWideChar(CP_ACP, 0, s, i, w, LEN_BUF);
      w[i] = 0;
      RhsIO.printf(L"%s", w);
    }
  }

  RhsIO.errprintf(L"\r\n***Done***\r\n");

// All done, close the connection

  skt.CloseSocket();
  skt.Cleanup();

  if (out != stdout)
    fclose(out);

// All done

  if (usepost)
    free(postdata);

  return 0;
}


//**********************************************************************
// Base64Encode
// ============
// Encode a string as Base64
//**********************************************************************

static char Base64EncodeMap[64] =
{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
 'w', 'x', 'y', 'z', '0', '1', '2', '3',
 '4', '5', '6', '7', '8', '9', '+', '/'
};

void Base64Encode(WCHAR* Input, char* Output)
{ int i, j;
  unsigned u, v;

  i = j = 0;

  while (Input[i] != '\0')
  {
    u = (unsigned) Input[i++];
    Output[j++] = Base64EncodeMap[(u >> 2) & 0x3F];
    u = (u & 0x03) << 4;

    if (Input[i] == '\0')
    {
      Output[j++] = Base64EncodeMap[u];
      Output[j++] = '=';
      Output[j++] = '=';
    }
    else
    {
      v = (unsigned) Input[i++];
      u = u + ((v >> 4) & 0x0F);
      Output[j++] = Base64EncodeMap[u];
      u = (v & 0x0F) << 2;

      if (Input[i] == '\0')
      {
        Output[j++] = Base64EncodeMap[u];
        Output[j++] = '=';
      }
      else
      {
        v = (unsigned) Input[i++];
        u = u + ((v >> 6) & 0x03);
        Output[j++] = Base64EncodeMap[u];
        Output[j++] = Base64EncodeMap[v & 0x3F];
      }
    }
  }

  Output[j] = '\0';
}


//**********************************************************************
// wstrtoastr
// ----------
// Convert a UNICODE string to ASCII
//**********************************************************************

char* wstrtoastr(const WCHAR* w, char* a, int len)
{
  // If the detination string is NULL we allocate a new array
  // The caller is responsible for freeing this.
  if (!a)
  {
    // Get the required buffer size
    len = WideCharToMultiByte(CP_ACP, 0, w, -1, NULL, 0, NULL, 0);
    a = (char*) malloc(len);
  }

  // Do the conversion
  a[0] = '\0';
  if (WideCharToMultiByte(CP_ACP, 0, w, -1, a, len, NULL, 0) == 0)
    return NULL;
  else
    return a;
}


//**********************************************************************
// strtoint
// --------
// Convert string to int
//**********************************************************************

BOOL strtoint(const WCHAR* s, int* i)
{
  const WCHAR* start;
  WCHAR* end;

  for (start = s; *start == ' ' || *start == '\t'; start++);
  *i = (int) wcstol(start, &end, 10);

  return end > start ? TRUE : FALSE;
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
//**********************************************************************

const WCHAR* GetLastErrorMessage(void)
{
  DWORD d;
  static WCHAR errmsg[1024];

  d = GetLastError();

  lstrcpy(errmsg, L"<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 1023, NULL);
  return errmsg;
}


