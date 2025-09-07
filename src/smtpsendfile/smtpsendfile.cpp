//**********************************************************************
// smtputil
// ========
// Command line utility for testing an SMTP server.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 14/05/2013
//**********************************************************************

#define STRICT
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include <CRhsSocket/CRhsSocket.h>


//**********************************************************************
// Prototypes
// ----------
//**********************************************************************

BOOL Connect(const char* ServerName, int Port);
BOOL Sendfile(const char* To, const char* From, const char* Filename);
BOOL Disconnect(void);


//**********************************************************************
// Constants
// ---------
//**********************************************************************

#define SYNTAX "smtpsendfile <SMTP server> <to> <from> <filename>\n"


//**********************************************************************
// Globals
// -------
//**********************************************************************

CRhsSocket smtp;


//**********************************************************************
// main
// ----
//**********************************************************************

int main(int argc, char* argv[]) {
  // Check the arguments
  if (argc != 2 && argc != 5) {
    printf(SYNTAX);
    return 2;
  }

  if (argc >= 2) {
    if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "/?") == 0) {
      printf(SYNTAX);
      return 0;
    }
  }

  // Connect to the SMTP server
  if (!Connect(argv[1], 25))
    return 2;

  // Send the file
  if (!Sendfile(argv[2], argv[3], argv[4])) {
    Disconnect();
    return 2;
  }

  // Disconnect from the SMTP server
  Disconnect();

  // All done
  return 0;
}


//**********************************************************************
// Connect
// -----------------
// Connect to the mail server and check for a "220" response
//**********************************************************************

BOOL Connect(const char* ServerName, int Port)
{ char s[1024];

  smtp.Startup();

  printf("Connecting to %s on port %i ...\n", ServerName, Port);

  // Connect.  There can be several lines of introduction, for example if
  // the server does ESMTP.
  if (!smtp.Connect(ServerName, Port)) {
    smtp.GetLastErrorMessage(s, 1023);
    printf("Failed: %s\n", s);
    return FALSE;
  }

  do {
    smtp.gets(s, 1023);
    if (strncmp(s, "220", 3) != 0)
    { printf("Server did not respond 220: aborting ...\n");

      Disconnect();
      return FALSE;
    }
  } while (smtp.select());

  // Send HELO
  gethostname(s, 1023);
  struct hostent* he = gethostbyname(s);

  smtp.printf("HELO %s\r\n", s);
  smtp.gets(s, 1023);
  if (strncmp(s, "250", 3) != 0) {
    printf("Server did not respond 250: aborting ...\n");
    Disconnect();
    return FALSE;
  }

  // All done
  printf("Connected\n");
  return TRUE;
}


//**********************************************************************
// Disconnect
// ----------
// Disconnect from the mail server.  This closes the socket then
// deallocates it.
//**********************************************************************

BOOL Sendfile(const char* To, const char* From, const char* Filename) {
  FILE* f;
  char s[1024];

  // Open the file
  if (fopen_s(&f, Filename, "rb") != 0) {
    printf("Cannot open file %s\n", Filename);
    return FALSE;
  }

  // Send the from address
  smtp.printf("MAIL FROM: <%s>\r\n", From);
  smtp.gets(s, 1023);
  if (strncmp(s, "250", 3) != 0) {
    printf("Error sending FROM address: %s\n", s);
    return FALSE;
  }

// Send the from address

  smtp.printf("RCPT TO: <%s>\r\n", To);
  smtp.gets(s, 1023);
  if (strncmp(s, "250", 3) != 0) {
    printf("Error sending TO address: %s\n", s);
    return FALSE;
  }

  // Send the file
  smtp.printf("DATA\r\n");
  smtp.gets(s, 1023);
  if (strncmp(s, "354", 3) != 0) {
    printf("Server did not respond 354: aborting ...\n");
    return FALSE;
  }

  while (fgets(s, 1023, f))
    smtp.puts(s);
  smtp.puts(".\r\n");

  smtp.gets(s, 1023);
  if (strncmp(s, "250", 3) != 0) {
    printf("Error sending file: %s\n", s);
    return FALSE;
  }

  // Return indicating success
  printf("File %s sent\n", Filename);
  return TRUE;
}


//**********************************************************************
// Disconnect
// ----------
// Disconnect from the mail server.  This closes the socket then
// deallocates it.
//**********************************************************************

BOOL Disconnect(void)
{
  smtp.puts("QUIT\r\n");

  smtp.CloseSocket();

  smtp.Cleanup();

  return TRUE;
}
