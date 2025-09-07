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
BOOL Disconnect(void);

BOOL MailFrom(void);
BOOL RcptTo(void);
BOOL Subject(void);
BOOL Data(void);

BOOL strtoint(const char* s, int* i);


//**********************************************************************
// Constants
// ---------
//**********************************************************************

#define SYNTAX "smtputil <SMTP server> [<port>]\n"


//**********************************************************************
// Globals
// -------
//**********************************************************************

CRhsSocket smtp;

char mail_from[256], rcpt_to[256], subject[256];


//**********************************************************************
// main
// ----
//**********************************************************************

int main(int argc, char* argv[])
{ int port = 25;

// Check the arguments

  if (argc != 2 && argc != 3)
  { printf(SYNTAX);
    return 2;
  }

  if (argc >= 2)
  { if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "/?") == 0)
    { printf(SYNTAX);
      return 0;
    }
  }

  if (argc == 3)
  { if (!strtoint(argv[2], &port))
    { printf(SYNTAX);
      return 0;
    }
  }

// Connect to the SMTP server

  if (!Connect(argv[1], port))
    return 2;

// From address

  if (!MailFrom())
  { Disconnect();
    return 2;
  }

// To address

  if (!RcptTo())
  { Disconnect();
    return 2;
  }

// Data

  Subject();

  if (!Data())
  { Disconnect();
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

  printf("\n");
  printf("Connecting to %s on port %i ...\n", ServerName, Port);

// Connect.  There can be several lines of introduction, for example if
// the server does ESMTP.

  if (!smtp.Connect(ServerName, Port))
  { smtp.GetLastErrorMessage(s, 1023);
    printf("Failed: %s\n", s);
    return FALSE;
  }

  do
  { smtp.gets(s, 1023);
    printf("<- %s\n", s);

    if (strncmp(s, "220", 3) != 0)
    { printf("Server did not respond 220: aborting ...\n");

      Disconnect();
      return FALSE;
    }
  } while (smtp.select());

// Send HELO

  gethostname(s, 1023);
  struct hostent* he = gethostbyname(s);

  printf("\n");
  printf("-> HELO %s\r\n", s);
  smtp.printf("HELO %s\r\n", s);

  smtp.gets(s, 1023);
  printf("<- %s\n", s);

  if (strncmp(s, "250", 3) != 0)
  { printf("Server did not respond 250: aborting ...\n");

    Disconnect();
    return FALSE;
  }

// All done

  return TRUE;
}


//**********************************************************************
// MailFrom
// --------
// Prompt for the FROM address and send it to the SMTP server.
// Leaving the FROM address blank aborts the session.
//**********************************************************************

BOOL MailFrom(void)
{ char s[1024];

// Prompt for the from address

  printf("\n");
  printf("FROM address: ");
  gets(mail_from);

  if (strlen(mail_from) == 0)
  { printf("No FROM address: aborting...\n");
    return FALSE;
  }

// Send the from address

  printf("-> MAIL FROM: <%s>\r\n", mail_from);
  smtp.printf("MAIL FROM: <%s>\r\n", mail_from);

  smtp.gets(s, 1023);
  printf("<- %s\n", s);

  if (strncmp(s, "250", 3) != 0)
  { printf("Server did not respond 250: aborting ...\n");
    return FALSE;
  }

// All done

  return TRUE;
}


//**********************************************************************
// RcptTo
// ------
// Prompt for the TO address and send it to the SMTP server.
// Leaving the TO address blank aborts the session.
//**********************************************************************

BOOL RcptTo(void)
{ char s[1024];

// Prompt for the from address

  printf("\n");
  printf("TO address: ");
  gets(rcpt_to);

  if (strlen(rcpt_to) == 0)
  { printf("No TO address: aborting...\n");
    return FALSE;
  }

// Send the from address

  printf("-> RCPT TO: <%s>\r\n", rcpt_to);
  smtp.printf("RCPT TO: <%s>\r\n", rcpt_to);

  smtp.gets(s, 1023);
  printf("<- %s\n", s);

  if (strncmp(s, "250", 3) != 0)
  { printf("Server did not respond 250: aborting ...\n");
    return FALSE;
  }

// All done

  return TRUE;
}


//**********************************************************************
// Subject
// -------
// Prompt for the subject and send it to the SMTP server.
// Leaving the subject blank uses a default subject.
//**********************************************************************

BOOL Subject(void)
{

// Prompt for the from address

  printf("\n");
  printf("Subject: ");
  gets(subject);

  if (strlen(subject) == 0)
    strcpy(subject, "SMTPUtil test mail");

// All done

  return TRUE;
}


//**********************************************************************
// Data
// ----
// Send the message body.
// The function sends the headers then prompts the user to type in the
// message.
//**********************************************************************

BOOL Data(void)
{ SYSTEMTIME st;
  char s[1024];

// Start sending the data

  printf("\n");
  printf("-> DATA\r\n");
  smtp.printf("DATA\r\n");

  smtp.gets(s, 1023);
  printf("<- %s\n", s);

  if (strncmp(s, "354", 3) != 0)
  { printf("Server did not respond 354: aborting ...\n");
    return FALSE;
  }

// Send the headers

  GetLocalTime(&st);

  printf("-> Date: %02i/%02i/%04i\r\n", st.wDay, st.wMonth, st.wYear);
  smtp.printf("Date: %02i/%02i/%04i\r\n", st.wDay, st.wMonth, st.wYear);

  printf("-> From: %s\r\n", mail_from);
  smtp.printf("From: %s\r\n", mail_from);

  printf("-> To: %s\r\n", rcpt_to);
  smtp.printf("To: %s\r\n", rcpt_to);

  printf("-> Subject: %s\r\n\r\n", subject);
  smtp.printf("Subject: %s\r\n\r\n", subject);

// Now send the data

  printf("Enter the message body\n");
  printf("Type . to finish\n");

  for(;;)
  { printf("-> ");
    gets(s);
    smtp.printf("%s\r\n", s);

    if (strcmp(s, ".") == 0)
    { smtp.gets(s, 1023);
      printf("<- %s\n", s);
      break;
    }
  }

// All done

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


