//**********************************************************************
// CRhsSocket
// ==========
//
// C++ class to encapsulate a WinSock socket
//
// J. Rennie
// john.rennie@ratsauce.co.uk
// 16th April 1998
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "CRhsSocket.h"


//**********************************************************************
// For 16 bit windows we need a few definitions
//**********************************************************************

#ifndef WIN32

#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif

#ifndef SHORT
#define SHORT int
#endif

#endif


//**********************************************************************
// Constructor & Destructor
// ------------------------
// The destructor closes the socket and event handles if they are open.
// Remember that OVERLAPPED i/o doesn't exist in 16 bit Windows
//**********************************************************************

CRhsSocket::CRhsSocket()
{
  m_Socket = INVALID_SOCKET;

  memset(&m_HostAddr, 0, sizeof(m_HostAddr));
  memset(&m_PeerAddr, 0, sizeof(m_PeerAddr));

  ((struct sockaddr_in*) &m_HostAddr)->sin_family = AF_INET;
  ((struct sockaddr_in*) &m_PeerAddr)->sin_family = AF_INET;

  m_Bound = FALSE;

#ifdef WIN32
  FillMemory(&m_ReadOverlapped, sizeof(m_ReadOverlapped), 0);
  m_ReadOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  FillMemory(&m_WriteOverlapped, sizeof(m_WriteOverlapped), 0);
  m_WriteOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif

  m_Timeout = INFINITE;
}


CRhsSocket::~CRhsSocket()
{
  if (m_Socket != INVALID_SOCKET)
    closesocket(m_Socket);

#ifdef WIN32
  CloseHandle(m_ReadOverlapped.hEvent);
  CloseHandle(m_WriteOverlapped.hEvent);
#endif
}


//**********************************************************************
// CRhsSocket::Startup
// -------------------
// Windows sockets startup.  You don't need to call this if WSAStartup
// has been called, it's just for convenience.
//**********************************************************************

BOOL CRhsSocket::Startup(void)
{ WSADATA wd;

  m_LastError = WSAStartup(MAKEWORD(1, 1), &wd);

  return(m_LastError == 0 ? TRUE : FALSE);
}


void CRhsSocket::Cleanup(void)
{
   WSACleanup();
}


//**********************************************************************
// CRhsSocket::Connect
// -------------------
// Connect to a server
// This method will do all initialisation and binding if necessary, so
// usually just this one call is all that is needed to connect a socket.
//**********************************************************************

BOOL CRhsSocket::Connect(const char* Peer, int PeerPort, const char* Host, int HostPort)
{

// If we aren't already bound set the host and bind

  if (!Bind(Host, HostPort))
    if (m_LastError != 0)
      return FALSE;

// Set the peer

  if (!SetPeer(Peer, PeerPort))
    return FALSE;

// Connect

  if (connect(m_Socket, &m_PeerAddr, sizeof(m_PeerAddr)) != 0)
  { m_LastError = WSAGetLastError();
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CRhsSocket::Listen
// ------------------
// Start listening on a socket
//**********************************************************************

BOOL CRhsSocket::Listen(char* Host, int Port)
{

// If we aren't already bound set the host and bind

  if (!Bind(Host, Port))
    if (m_LastError != 0)
      return FALSE;

// Listen on the port

  if (listen(m_Socket, 5) != 0)
  { m_LastError = WSAGetLastError();
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CRhsSocket::Accept
// ------------------
// Wait for a client connection
// This method blocks until a connection is made or an error occurs.
//**********************************************************************

CRhsSocket* CRhsSocket::Accept(char* Peer, int MaxLen)
{ int len;
  CRhsSocket* accepted;
  char* peer;

// Allocate a new object to hold the accepted socket

  accepted = new CRhsSocket;
  if (!accepted)
  { m_LastError = WSAGetLastError();
    return(NULL);
  }

// Accept a connection

  len = sizeof(struct sockaddr);
  accepted->m_Socket = accept(m_Socket, &accepted->m_PeerAddr, &len);

  if (accepted->m_Socket == INVALID_SOCKET)
  { m_LastError = WSAGetLastError();
    delete accepted;
    return(NULL);
  }

// We've got a connection so fill in the other bits of the new object

  accepted->m_HostAddr = m_HostAddr;
  accepted->m_Bound = TRUE;

// If required get the name of the connected client

  if (Peer)
  { peer = inet_ntoa(((struct sockaddr_in*) &accepted->m_PeerAddr)->sin_addr);
    if (peer)
    { lstrcpyn(Peer, peer, MaxLen);
      Peer[MaxLen] = '\0';
    }
    else
    { lstrcpy(Peer, "");
    }
  }

// Return the new object

  return(accepted);
}


//**********************************************************************
// CRhsSocket::printf
// ------------------
// Just like the ANSI C printf except that it uses the Win32 sprintf
// which is more limited.  The main restriction is no floating point
// support.
//**********************************************************************

int CRhsSocket::printf(const char* Format, ...)
{ int i;
  va_list ap;
  char s[0x1000];

  va_start(ap, Format);
  wvsprintf(s, Format, ap);
  va_end(ap);

  i = WriteTimeout(s, lstrlen(s), 0);

  return(i);
}


//**********************************************************************
// CRhsSocket::gets
// ----------------
// Reads a string terminated by \n.  NB the terminating \n or \r\n is
// stripped.
//
// If a non-NULL value is returned then at least one character was read.
// The test for no data read or an error is a return value of NULL.
//
// CRhsSocket::puts
// ----------------
// Returns TRUE if at least one character was written or FALSE if no
// characters were written or an error occurred.
//**********************************************************************

char* CRhsSocket::gets(char* Data, int MaxLen)
{ int i, j;

  for (i = 0; i < MaxLen; i++)
  { j = ReadTimeout(Data + i, 1, 0);
    if (j == SOCKET_ERROR || j == 0)
    { Data[i] = '\0';
      return(i > 0 ? Data : NULL);
    }

    if (Data[i] == '\n')
      break;
  }

  Data[i] = '\0';

  if (i > 0)
    if (Data[i-1] == '\r')
      Data[i-1] = '\0';

  return(Data);
}


BOOL CRhsSocket::puts(const char* Data)
{ int tosend, sent, retries, i;

  tosend = lstrlen(Data);
  sent = retries = 0;

  while (tosend > 0)
  { i = WriteTimeout(Data + sent, tosend, 0);
    if (i == SOCKET_ERROR || i == 0)
      return(sent > 0 ? TRUE : FALSE);

    sent += i;
    tosend -= i;

    if (++retries > 5)
    { m_LastError = 0;
      return FALSE;
    }
  }

  return TRUE;
}


//**********************************************************************
// CRhsSocket::sgetc
// CRhsSocket::sputc
// -----------------
// Just like getc and putc.  The reason for not calling the methods getc
// and putc is to avoid the preprocessor interpreting them as macros.
//**********************************************************************

char CRhsSocket::sgetc(void)
{ int i;
  char c;

  i = ReadTimeout(&c, 1, 0);
  if (i == SOCKET_ERROR || i == 0)
    return(0);

  return(c);
}


char CRhsSocket::sputc(char c)
{ int i;

  i = WriteTimeout(&c, 1, 0);
  if (i == SOCKET_ERROR || i == 0)
    return(0);

  return(c);
}


//**********************************************************************
// CRhsSocket::read
// CRhsSocket::write
// -----------------
// Like the C read and write functions
//**********************************************************************

int CRhsSocket::read(void* Data, int Size, int NumObj)
{ int i;

  i = ReadTimeout((char*) Data, Size*NumObj, 0);

  return(i);
}


int CRhsSocket::write(const void* Data, int Size, int NumObj)
{ int i;

  i = WriteTimeout((char*) Data, Size*NumObj, 0);

  return(i);
}


//**********************************************************************
// CRhsSocket::recv
// CRhsSocket::send
// ----------------
// Standard sockets recv and send
//**********************************************************************

int CRhsSocket::recv(char* buf, int len, int flags)
{ int i;

  i = ::recv(m_Socket, buf, len, flags);

  if (i == SOCKET_ERROR)
    m_LastError = WSAGetLastError();

// If the function reports success but no data was read this means the
// connection has been closed

  if (i == 0)
    m_LastError = WSAENOTCONN;

  return(i);
}


int CRhsSocket::send(const char* buf, int len, int flags)
{ int i;

  i = ::send(m_Socket, buf, len, flags);

  if (i == SOCKET_ERROR)
    m_LastError = WSAGetLastError();

  return(i);
}


int CRhsSocket::recvfrom(char* buf, int len, int flags)
{ int i;

  i = sizeof(m_PeerAddr);
  i = ::recvfrom(m_Socket, buf, len, flags, &m_PeerAddr, &i);

  if (i == SOCKET_ERROR)
    m_LastError = WSAGetLastError();

  return(i);
}


int CRhsSocket::sendto(const char* buf, int len, int flags)
{ int i;

  i = sizeof(m_PeerAddr);
  i = ::sendto(m_Socket, buf, len, flags, &m_PeerAddr, i);

  if (i == SOCKET_ERROR)
    m_LastError = WSAGetLastError();

  return(i);
}


//**********************************************************************
// CRhsSocket::select
// ------------------
// This is a simplified form of the sockets select function.
//
// Args:
//   Read    - check for data waiting to be read (default TRUE)
//   Write   - check for data waiting to be written (default FALSE)
//   Error   - check for errors (default FALSE)
//   Timeout - timout for select in milliseconds (default 0)
//
// Just calling select() checks for data waiting in the input buffer.
// Supply arguments for other checks.
//**********************************************************************

int CRhsSocket::select(BOOL Read, BOOL Write, BOOL Error, DWORD Timeout)
{ int select_result;
  fd_set* fd_read = NULL;
  fd_set* fd_write = NULL;
  fd_set* fd_error = NULL;
  struct timeval timeout;

// Check at least one select has been asked for

  if (!Read && !Write && !Error)
    return 0;

// Initialise the fd_sets and the timeout

  if (Read)
  { fd_read = (fd_set*) malloc(sizeof(fd_set));
    FD_ZERO(fd_read);
    FD_SET(m_Socket, fd_read);
  }

  if (Write)
  { fd_write = (fd_set*) malloc(sizeof(fd_set));
    FD_ZERO(fd_write);
    FD_SET(m_Socket, fd_write);
  }

  if (Error)
  { fd_error = (fd_set*) malloc(sizeof(fd_set));
    FD_ZERO(fd_error);
    FD_SET(m_Socket, fd_error);
  }

  timeout.tv_sec = Timeout/1000;
  timeout.tv_usec = (Timeout - (timeout.tv_sec*1000))*1000;

// Do the select

  select_result = ::select(0, fd_read, fd_write, fd_error, &timeout);

  if (select_result == 0)
    m_LastError = WSAETIMEDOUT;

// Tidy up

  if (fd_read)
    free(fd_read);

  if (fd_write)
    free(fd_write);

  if (fd_error)
    free(fd_error);

// All done

  return select_result;
}


//**********************************************************************
// CRhsSocket::ReadTimeout
// -----------------------
// This method is used internally by all methods that read data.  It
// uses overlapped i/o to queue a read then waits for the response so
// the method is blocking but will fail if nothing is received by the
// timeout.  Use the Timeout method to set the timeout.
//
// NB If using a timeout the buffer must be at least of size len+1 as a
// terminating \0 is added.
//
// OVERLAPPED i/o doesn't exist in 16 bit Windows so the timeout will
// always be infinite (or whatever the sockets implementation uses.
//**********************************************************************

int CRhsSocket::ReadTimeout(char* buf, int len, int flags)
{ int i;

// If there is no timeout use the Berkeley recv function

  if (m_Timeout == INFINITE)
  { i = ::recv(m_Socket, buf, len, flags);
    if (i == SOCKET_ERROR)
      m_LastError = WSAGetLastError();

// If the function reports success but no data was read this means the
// connection has been closed

    if (i == 0)
      m_LastError = WSAENOTCONN;
  }

// If there is a timeout use overlapped i/o

#ifdef WIN32
  else
  { BOOL b;

    FillMemory(buf, len+1, 0);
    ResetEvent(m_ReadOverlapped.hEvent);
    b = ReadFile((HANDLE) m_Socket, buf, len, NULL, &m_ReadOverlapped);

    if (!b && ::GetLastError() != ERROR_IO_PENDING)
    { i = SOCKET_ERROR;
      m_LastError = ::GetLastError();
    }
    else
    { if (WaitForSingleObject(m_ReadOverlapped.hEvent, m_Timeout) == WAIT_TIMEOUT)
      { i = 0;
        m_LastError = ::GetLastError();
      }
      else
      { i = lstrlen(buf);
        m_LastError = 0;
      }
    }
  }
#else
  else
  { i = SOCKET_ERROR;
  }
#endif

// All done

  return(i);
}


//**********************************************************************
// CRhsSocket::WriteTimeout
// ------------------------
// Obvious partner to ReadTimeout.
//
// OVERLAPPED i/o doesn't exist in 16 bit Windows so the timeout will
// always be infinite (or whatever the sockets implementation uses.
//**********************************************************************

int CRhsSocket::WriteTimeout(const char* buf, int len, int flags)
{ int i;

// If there is no timeout use the Berkeley send function

  if (m_Timeout == INFINITE)
  { i = ::send(m_Socket, buf, len, flags);
    if (i == SOCKET_ERROR)
      m_LastError = WSAGetLastError();
  }

// If there is a timeout use overlapped i/o

#ifdef WIN32
  else
  { BOOL b;
    DWORD d = 0;

    ResetEvent(m_WriteOverlapped.hEvent);
    b = WriteFile((HANDLE) m_Socket, buf, len, NULL, &m_WriteOverlapped);

    if (!b && ::GetLastError() != ERROR_IO_PENDING)
    { i = SOCKET_ERROR;
      m_LastError = ::GetLastError();
    }
    else
    { if (WaitForSingleObject(m_WriteOverlapped.hEvent, m_Timeout) == WAIT_TIMEOUT)
      { i = SOCKET_ERROR;
        m_LastError = ::GetLastError();
      }
      else
      { GetOverlappedResult((HANDLE) m_Socket, &m_WriteOverlapped, &d, FALSE);
        i = (int) d;
      }
    }
  }
#else
  else
  { i = SOCKET_ERROR;
  }
#endif

// All done

  return(i);
}


//**********************************************************************
// CRhsSocket::Shutdown
// --------------------
// Like the sockets shutdown function
//**********************************************************************

BOOL CRhsSocket::Shutdown(int How)
{
  if (shutdown(m_Socket, How) != 0)
  { m_LastError = WSAGetLastError();
    return FALSE;
  }

  return TRUE;
}


//**********************************************************************
// CRhsSocket::CloseSocket
// -----------------------
// This closes the socket and resets all internal variables.
// The object is now ready for reuse.
//**********************************************************************

BOOL CRhsSocket::CloseSocket(void)
{

// Close the socket

  if (closesocket(m_Socket) != 0)
  { m_LastError = WSAGetLastError();
    return FALSE;
  }

// Reset all internal variables to defaults

  m_Socket = INVALID_SOCKET;

  memset(&m_HostAddr, 0, sizeof(m_HostAddr));
  memset(&m_PeerAddr, 0, sizeof(m_PeerAddr));

  ((struct sockaddr_in*) &m_HostAddr)->sin_family = AF_INET;
  ((struct sockaddr_in*) &m_PeerAddr)->sin_family = AF_INET;

  m_Bound = FALSE;

  m_Timeout = INFINITE;

// All done

  return TRUE;
}


//**********************************************************************
// CRhsSocket::Bind
// ----------------
// Like the sockets bind function
//**********************************************************************

BOOL CRhsSocket::Bind(const char* Host, int Port)
{

// If we are already bound return FALSE but with no last error

  m_LastError = 0;

  if (m_Bound)
    return FALSE;

// If the socket hasn't been created create it now

  if (m_Socket == INVALID_SOCKET)
    if (!CreateSocket())
      return FALSE;

// If a host name was supplied then use it

  if (!SetHost(Host, Port))
    return FALSE;

// Now bind the socket

  if (bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) != 0)
  { m_LastError = WSAGetLastError();
    return FALSE;
  }

  m_Bound = TRUE;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CRhsSocket::BindInRange
// -----------------------
// Bind a socket from a range of ports
//**********************************************************************

BOOL CRhsSocket::BindInRange(char* Host, int Low, int High, int Retries)
{ int port, i;

// If we are already bound return FALSE but with no last error

  m_LastError = 0;

  if (m_Bound)
    return FALSE;

// If the socket hasn't been created create it now

  if (m_Socket == INVALID_SOCKET)
    if (!CreateSocket())
      return FALSE;

// If the number of retries is greater than the range then work through the
// range sequentially.

  if (Retries > High - Low)
  { for (port = Low; port <= High; port++)
    { if (!SetHost(Host, port))
        return FALSE;

      if (bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) == 0)
        break;
    }

    if (port > High)
    { m_LastError = WSAGetLastError();
      return FALSE;
    }
  }

// Else select numbers within the range at random

  else
  { for (i = 0; i < Retries; i++)
    { port = Low + (rand() % (High - Low));

      if (!SetHost(Host, port))
        return FALSE;

      if (bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) == 0)
        break;
    }

    if (i >= Retries)
    { m_LastError = WSAGetLastError();
      return FALSE;
    }
  }

// Return indicating success

  m_Bound = TRUE;

  return TRUE;
}


//**********************************************************************
// CRhsSocket::Linger
// ------------------
// Set the linger option
//**********************************************************************

BOOL CRhsSocket::Linger(BOOL LingerOn, int LingerTime)
{ int i;
  BOOL b;
  struct linger l;

// If the socket hasn't been created create it now

  if (m_Socket == INVALID_SOCKET)
    if (!CreateSocket())
      return FALSE;

// Set the lingering

  if (LingerOn)
  { l.l_onoff = 1;
    l.l_linger = LingerTime;
    i = setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (const char*) &l, sizeof(l));
  }
  else
  { b = 1; // TRUE turns linger off
    i = setsockopt(m_Socket, SOL_SOCKET, SO_DONTLINGER, (const char*) &b, sizeof(b));
  }

// Check for errors

  if (i != 0)
  { m_LastError = WSAGetLastError();
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CRhsSocket::CreateSocket
// ------------------------
// Create the socket
// You almost never need to call this directly as other methods like
// Connect call it when required.
//**********************************************************************

BOOL CRhsSocket::CreateSocket(DWORD Type)
{

// If the socket has already been created just return TRUE

  if (m_Socket != INVALID_SOCKET)
    return TRUE;

// Create the socket

  m_Socket = socket(PF_INET, Type, 0);

  if (m_Socket == INVALID_SOCKET)
  { m_LastError = WSAGetLastError();
    return FALSE;
  }

// By default set no lingering

// I suspect this may be causing problems
//  Linger(FALSE, 0);

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CRhsSocket::Socket
// ------------------
// Attach to an existing socket
// NB there is no checking that the socket is valid
//**********************************************************************

BOOL CRhsSocket::Socket(SOCKET Socket, BOOL Bound)
{ int i;

  if (Socket == INVALID_SOCKET)
    return FALSE;

  if (m_Socket != INVALID_SOCKET)
    CloseSocket();

  m_Socket  = Socket;
  m_Bound   = Bound;
  m_Timeout = INFINITE;

  i = sizeof(m_HostAddr);
  getsockname(m_Socket, &m_HostAddr, &i);
  i = sizeof(m_PeerAddr);
  getpeername(m_Socket, &m_PeerAddr, &i);

  return TRUE;
}


//**********************************************************************
// CRhsSocket::SetPeer
// CRhsSocket::GetPeer
// -------------------
// Set and et the peer
//**********************************************************************

BOOL CRhsSocket::SetPeer(const char* Peer, int Port)
{ struct hostent* he;

// If a peer name was supplied then use it

  if (Peer)
  { he = gethostbyname(Peer);
    if (!he)
    { unsigned long ipaddress = inet_addr(Peer);
      if (ipaddress == INADDR_NONE)
      { m_LastError = WSAGetLastError();
        return FALSE;
      }

      he = gethostbyaddr((const char*) &ipaddress, 4, PF_INET);
      if (!he)
      { m_LastError = WSAGetLastError();
        return FALSE;
      }
    }

    memcpy((void*) &((struct sockaddr_in*) &m_PeerAddr)->sin_addr, (void*) he->h_addr_list[0], 4);
  }

// If a port name was supplied use it

  if (Port > 0)
    ((struct sockaddr_in*) &m_PeerAddr)->sin_port = htons((SHORT) Port);

// Return indicating success

  return TRUE;
}


BOOL CRhsSocket::GetPeer(char* Peer, int MaxLen, int* Port)
{ char* peer;

  if (Peer)
  { peer = inet_ntoa(((struct sockaddr_in*) &m_PeerAddr)->sin_addr);
    if (!peer)
    { m_LastError = WSAGetLastError();
      return FALSE;
    }

    lstrcpyn(Peer, peer, MaxLen);
    Peer[MaxLen] = '\0';
  }

  if (Port)
    *Port = ntohs(((struct sockaddr_in*) &m_PeerAddr)->sin_port);

  return FALSE;
}


//**********************************************************************
// CRhsSocket::SetHost
// CRhsSocket::GetHost
// -------------------
// Set and get the host
//**********************************************************************

BOOL CRhsSocket::SetHost(const char* Host, int Port)
{ struct hostent* he;

// If a host name was supplied then use it

  if (Host)
  { he = gethostbyname(Host);
    if (!he)
    { unsigned long ipaddress = inet_addr(Host);
      if (ipaddress == INADDR_NONE)
      { m_LastError = WSAGetLastError();
        return FALSE;
      }

      he = gethostbyaddr((const char*) &ipaddress, 4, PF_INET);
      if (!he)
      { m_LastError = WSAGetLastError();
        return FALSE;
      }
    }

    memcpy((void*) &((struct sockaddr_in*) &m_HostAddr)->sin_addr, (void*) he->h_addr_list[0], 4);
  }

// If a port name was supplied use it

  if (Port > 0)
    ((struct sockaddr_in*) &m_HostAddr)->sin_port = htons((SHORT) Port);

// Return indicating success

  return TRUE;
}


BOOL CRhsSocket::GetHost(char* Host, int MaxLen, int* Port)
{ char* host;

  if (Host)
  { host = inet_ntoa(((struct sockaddr_in*) &m_HostAddr)->sin_addr);
    if (!host)
    { m_LastError = WSAGetLastError();
      return FALSE;
    }

    lstrcpyn(Host, host, MaxLen);
    Host[MaxLen] = '\0';
  }

  if (Port)
    *Port = ntohs(((struct sockaddr_in*) &m_HostAddr)->sin_port);

  return FALSE;
}


//**********************************************************************
// CRhsSocket::GetLastErrorMessage
// -------------------------------
// Horrible hack!  I can't get FormatMessage to work so I've put in the
// error messages by hand.
//**********************************************************************

char* CRhsSocket::GetLastErrorMessage(char* Error, int MaxLen)
{ char s[256];

  switch (m_LastError)
  { case 0:
      lstrcpyn(Error, "No error", MaxLen);
      break;

    case WSANOTINITIALISED:
      lstrcpyn(Error, "A successful WSAStartup must occur before using this function.", MaxLen);
      break;

    case WSAENETDOWN:
      lstrcpyn(Error, "The Windows Sockets implementation has detected that the network subsystem has failed.", MaxLen);
      break;

    case WSAEADDRINUSE:
      lstrcpyn(Error, "The specified address is already in use.", MaxLen);
      break;

    case WSAEINTR:
      lstrcpyn(Error, "The (blocking) call was canceled using WSACancelBlockingCall.", MaxLen);
      break;

    case WSAEINPROGRESS:
      lstrcpyn(Error, "A blocking Windows Sockets call is in progress.", MaxLen);
      break;

    case WSAEADDRNOTAVAIL:
      lstrcpyn(Error, "The specified address is not available from the local computer.", MaxLen);
      break;

    case WSAEAFNOSUPPORT:
      lstrcpyn(Error, "Addresses in the specified family cannot be used with this socket.", MaxLen);
      break;

    case WSAECONNREFUSED:
      lstrcpyn(Error, "The attempt to connect was forcefully rejected.", MaxLen);
      break;

//    case WSAEDESTADDREQ:
//      lstrcpyn(Error, "A destination address is required.", MaxLen);
//      break;

    case WSAEFAULT:
      lstrcpyn(Error, "The namelen argument is incorrect.", MaxLen);
      break;

    case WSAEINVAL:
      lstrcpyn(Error, "The socket is not already bound to an address.", MaxLen);
      break;

    case WSAEISCONN:
      lstrcpyn(Error, "The socket is already connected.", MaxLen);
      break;

    case WSAENOTCONN:
      lstrcpyn(Error, "The socket is not connected.", MaxLen);
      break;

    case WSAEMFILE:
      lstrcpyn(Error, "No more file descriptors are available.", MaxLen);
      break;

    case WSAENETUNREACH:
      lstrcpyn(Error, "The network can't be reached from this host at this time.", MaxLen);
      break;

    case WSAENOBUFS:
      lstrcpyn(Error, "No buffer space is available. The socket cannot be connected.", MaxLen);
      break;

    case WSAENOTSOCK:
      lstrcpyn(Error, "The descriptor is not a socket.", MaxLen);
      break;

    case WSAETIMEDOUT:
      lstrcpyn(Error, "Attempt to connect timed out without establishing a connection.", MaxLen);
      break;

    case WSAEWOULDBLOCK :
      lstrcpyn(Error, "The socket is marked as nonblocking and the connection cannot be completed immediately. It is possible to select the socket while it is connecting by selecting it for writing.", MaxLen);
      break;

    case WSAHOST_NOT_FOUND :
      lstrcpyn(Error, "The host cannot be found.", MaxLen);
      break;

    case WSAEACCES :
      lstrcpyn(Error, "Address/port is already in use.", MaxLen);
      break;

    case 11004:
      lstrcpyn(Error, "DNS responds: no data record of requested type.", MaxLen);
      break;

    default:
      wsprintf(s, "Unidentified error: %i - %08X", m_LastError, m_LastError);
      lstrcpyn(Error, s, MaxLen);
      break;
  }

  Error[MaxLen] = '\0';

  return(Error);
}


//**********************************************************************
// End of rhssock.cpp
//**********************************************************************

