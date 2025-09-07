//**********************************************************************
// GetSNTPTime
// ===========
//
// John Rennie
// jrennie@cix.compulink.co.uk
// 19th June 1999
//**********************************************************************

#include <windows.h>
#include <stdio.h>
#include <CRhsSocket/CRhsSocket.h>
#include "CSNTPTimestamp.h"


//**********************************************************************
// class CSNTPMessage
// ------------------
// This class is just a structure with some self-initialisation
//**********************************************************************

class CSNTPMessage
{
  public:
    inline CSNTPMessage()
    { FillMemory(this, sizeof(CSNTPMessage), 0);
      status = 0x1B;  // LI = 0, VN = 3 and mode = 3 (client)
    }

    BYTE status,
         stratum,
         poll,
         precision;

    DWORD root_delay,
          root_dispersion,
          reference_identifier;

    CSNTPTimestamp reference_timestamp,
                   originate_timestamp,
                   receive_timestamp,
                   transmit_timestamp;
};


//**********************************************************************
// Prototypes and Global Variables
// -------------------------------
//**********************************************************************

const char* GetLastErrorMessage(void);
BOOL strtoint(const char* s, int* i);

#define SYNTAX "Syntax: GetSNTPTime <time server> [<max change in seconds>]"


//**********************************************************************
// main
// ----
//**********************************************************************

int main(int argc, char* argv[])
{ int bytes_sent, bytes_received, change, max_change;
  SYSTEMTIME st;
  CRhsSocket skt;
  CSNTPTimestamp current_time, new_time;
  CSNTPMessage msg;
  char s[256];

// Check the arguments

  if (argc == 2)
  { if (lstrcmp(argv[1], "-?") == 0 || lstrcmp(argv[1], "/?") == 0)
    { printf(SYNTAX);
      return 0;
    }
  }

  if (argc < 2 || argc > 3)
  { printf(SYNTAX);
    return 0;
  }

  if (argc != 3)
  { max_change = 3600;
  }
  else
  { if (!strtoint(argv[2], &max_change))
    { printf("The maximum change \"%s\" is not a valid integer.\n", argv[2]);
      return 2;
    }
  }

// Initialise the WinSock library

  skt.Startup();

// Create a socket for use with UDP

  if (!skt.CreateSocket(SOCK_DGRAM))
  { printf("Failed to create socket: %s\n", skt.GetLastErrorMessage(s, 255));
    return 2;
  }

// Set the host and peer so that sendto and recvfrom know what server to
// contact.

  gethostname(s, 256);
  skt.SetHost(s, 123);
  skt.SetPeer(argv[1], 123);

// Allow broadcasts; I'm not sure if this is actually necessary but it
// doesn't hurt.

  bytes_sent = 1;
  bytes_sent = setsockopt(skt.Socket(), SOL_SOCKET, SO_BROADCAST, (const char*) &bytes_sent, sizeof(bytes_sent));

// Send the message and read the response

  GetSystemTime(&st);
  current_time.SetSystemTime(&st);
  msg.transmit_timestamp = current_time;

  bytes_sent = skt.sendto((const char*) &msg, sizeof(CSNTPMessage), 0);

  if (bytes_sent != sizeof(CSNTPMessage))
  { printf("Failed to send message to %s: %s\n", argv[1], skt.GetLastErrorMessage(s, 255));
    return 2;
  }

  printf("Waiting for reply ...\n");

// Use select first and allow 5 seconds for a reply

  bytes_received = skt.select(TRUE, FALSE, FALSE, 5000);

  if (bytes_received == 0 || bytes_received == SOCKET_ERROR)
  { printf("The server %s has failed to reply\n", argv[1]);
    return 2;
  }

// There is data ready so read it

  bytes_received = skt.recvfrom((char*) &msg, sizeof(CSNTPMessage), 0);

  if (bytes_received != sizeof(CSNTPMessage))
  { printf("Failed to read whole message from %s: %s\n", argv[1], skt.GetLastErrorMessage(s, 255));
    return 2;
  }

// We are now done with the IP stuff

  skt.CloseSocket();

  skt.Cleanup();

// Now process the message; for now ignore transmission delays and just set the
// new time equal to the transmit time.

  new_time = msg.transmit_timestamp;

// Check the change is not too big

  change = (int) current_time.SwapBytes(current_time.seconds)
         - (int) new_time.SwapBytes(new_time.seconds);

  if (change < 0)
    change = -change;

  if (change > max_change)
  { printf("The time change required is %i seconds which is too big.\n", change);

    GetSystemTime(&st);
    printf("The current time is:                %i/%i/%i %02i:%02i:%02i.%03i\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    new_time.GetSystemTime(&st);
    printf("The time returned by the server is: %i/%i/%i %02i:%02i:%02i.%03i\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    return 2;
  }

// OK change the time

  GetSystemTime(&st);
  printf("The current time is: %i/%i/%i %02i:%02i:%02i.%03i\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

  new_time.GetSystemTime(&st);
  SetSystemTime(&st);

  if (GetLastError() != 0)
  { printf("Failed to change the system time: %s\n", GetLastErrorMessage());
    return 2;
  }

  GetSystemTime(&st);
  printf("Time changed to:     %i/%i/%i %02i:%02i:%02i.%03i\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

// All done

  return 0;
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
//**********************************************************************

const char* GetLastErrorMessage(void)
{ DWORD d;
  static char errmsg[1024];

  d = GetLastError();

  lstrcpy(errmsg, "<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 1023, NULL);
  return(errmsg);
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


