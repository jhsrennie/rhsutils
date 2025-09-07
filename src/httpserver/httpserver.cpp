//**********************************************************************
// httpserver
// ==========
//
// This is an extremely simplified web server.  It just prints
// everything it receives to the console then closes the connection.
// I wrote it so I could see what various HTTP clients were sending to
// servers.
//
// The server uses the classical approach of spawning a thread for
// each accepted connection.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 18th September 1997
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <CRhsSocket/CRhsSocket.h>


//**********************************************************************
// Function prototypes
// -------------------
//**********************************************************************

DWORD WINAPI ServiceConnection(LPVOID lpSocket);
BOOL strtoint(const char* s, int* i);


//**********************************************************************
// Global variables
// ----------------
//**********************************************************************

#define SYNTAX \
"httpserver [<port number, default = 80> [IP address]]\n"


//**********************************************************************
// main
// ----
//**********************************************************************

int main(int argc, char* argv[])
{ int port;
  DWORD d;
  HANDLE h;
  CRhsSocket s_listen;
  CRhsSocket* s_accepted;
  char host[256], s[256];

// Check the arguments

  if (argc == 2)
  { if (lstrcmp(argv[1], "-?") == 0 || lstrcmp(argv[1], "/?") == 0)
    { printf(SYNTAX);
      return(0);
    }
  }

  port = 80;
  if (argc > 1)
  { if (!strtoint(argv[1], &port))
    { printf("\"%s\" is not a valid port number\n", argv[1]);
      return(2);
    }
  }

// Start listening

  s_listen.Startup();

  if (argc > 2)
    lstrcpy(host, argv[2]);
  else
    gethostname(host, 255);
  printf("Opening port %i on host %s\n", port, host);

  if (!s_listen.Listen(host, port))
  { printf("Error listening for connection\n%s\n", s_listen.GetLastErrorMessage(s, 255));
    return(2);
  }

  s_listen.GetHost(s, 255, &port);
  printf("Listening on port %i on host %s\n\n", port, s);

// Whenever we accept a connection pass the accepted socket to a new thread

  for (;;)
  { s_accepted = s_listen.Accept(s, 255);
    if (!s_accepted)
      break;

    s_accepted->GetPeer(s, 255, &port);
    printf("Accepted connection from %s on port %i\n", s, port);

    h = CreateThread(NULL, 0, ServiceConnection, (LPVOID) s_accepted, 0, &d);
    CloseHandle(h);
  }

// All done

  return(0);
}


//**********************************************************************
// ServiceConnection
// -----------------
//**********************************************************************

DWORD WINAPI ServiceConnection(LPVOID lpSocket)
{ CRhsSocket* skt;
  char s[0x1000];

  skt = (CRhsSocket*) lpSocket;

// Now just print everything we receive to the console

  skt->Timeout(5000);

  while (skt->gets(s, 0x0FFF))
    printf("%s\n", s);

  skt->CloseSocket();

// Delete the socket now we're finished with it

  delete skt;

// The return value is not significant

  return(0);
}


//**********************************************************************
// strtoint
// --------
// Convert string to int
//**********************************************************************

BOOL strtoint(const char* s, int* i)
{ const char* start;
  char* end;

  for (start = s; *start == ' ' || *start == '\t'; start++);
  *i = (int) strtol(start, &end, 10);

  return(end > start ? TRUE : FALSE);
}


