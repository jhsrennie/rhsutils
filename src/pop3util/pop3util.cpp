//**********************************************************************
// POP3Util
// ========
// Command line utility for doing various useful things with POP3
// mailboxes.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 07/05/2005
//**********************************************************************

#define STRICT
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include "CRhsPOP3Connection.h"
#include "CRhsPOP3Message.h"
#include "utils.h"


//**********************************************************************
// Prototypes
// ----------
//**********************************************************************

BOOL GetMessage(int Msg, BOOL HeaderOnly, const char* Filename);
BOOL ExecCommand(char* Command, char* Arg1, char* Arg2);

BOOL FileAllMessages(void);
BOOL FilterMessage(const char* Filter, BOOL Delete);
BOOL DeleteMessage(int Msg);
BOOL Reset(void);

BOOL GetCommand(char* Command, char* Arg1, char* Arg2);
BOOL ParseCommand(char* Input, char* Command, char* Arg1, char* Arg2);


//**********************************************************************
// Constants
// ---------
//**********************************************************************

#define SYNTAX "POP3Util v1.1.1\nSYNTAX: pop3util <POP3 server> <Username> <Password> [<Command>]\n"

#define HELP \
"Commands:\n" \
"  stat:              display number of mails and total size\n" \
"  list:              display list of mail numbers and sizes\n" \
"  header <no.>:      display the header for the requested message\n" \
"  read <no.>:        retrieve the requested message\n" \
"  file <no.> <file>: save the requested message to the file\n" \
"  file all:          save all messages msgXXXX.txt\n" \
"  del <no.>:         delete the requested message\n" \
"  del <filter>:      delete all messages containing the filter text\n" \
"  del all:           delete all messages\n" \
"  filter <filter>:   list all messages containing the filter text\n" \
"  reset:             abandon all changes (i.e. deletions)\n" \
"  exit or quit:      exit the utility\n" \
"  help:              display this message\n"


//**********************************************************************
// Globals
// -------
//**********************************************************************

CRhsPOP3Connection g_pop3;
CRhsPOP3Message    g_msg;


//**********************************************************************
// main
// ----
//**********************************************************************

int main(int argc, char* argv[])
{ char command[256], arg1[256], arg2[256], s[256];

// Check the arguments

  if (argc == 2)
  { if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "/?") == 0)
    { printf(SYNTAX);
      return 0;
    }
  }

  if (argc < 4 || argc > 5)
  { printf(SYNTAX);
    return 2;
  }

// Connect

  if (!g_pop3.Connect(argv[1], argv[2], argv[3]))
  { printf("Connection failed: %s\n", g_pop3.LastError(s, 255));
    return 2;
  }

  g_msg.SetConnection(&g_pop3);

// If a command was supplied then execute it and exit

  if (argc == 5)
  { printf("Executing command: %s\n", argv[4]);
    if (ParseCommand(argv[4], command, arg1, arg2))
      if (!ExecCommand(command, arg1, arg2))
        printf("Unknown command: %s\n", command);
  }

// Sit reading commands

  else
  {while (GetCommand(command, arg1, arg2))
    { if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0)
        break;

      if (!ExecCommand(command, arg1, arg2))
        printf("Unknown command: %s\n", command);
    }
  }

// Disconnect

  g_pop3.Disconnect();

// All done

  return 0;
}


//**********************************************************************
// ExecCommand
// -----------
// Execute a command
// Returns FALSE on an unknown command
//**********************************************************************

BOOL ExecCommand(char* Command, char* Arg1, char* Arg2)
{ int num_mails, total_size, i;
  char s[256];

// stat

  if (strcmp(Command, "stat") == 0)
  { if (!g_pop3.Stat(&num_mails, &total_size))
    { printf("%s\n", g_pop3.LastError(s, 255));
      return 2;
    }

    printf("%i %i\n", num_mails, total_size);
  }

// list

  else if (strcmp(Command, "list") == 0)
  { int* mail;
    int* size;

    if (!g_pop3.Stat(&num_mails, &total_size))
    { printf("Error, STAT returned: %s\n", g_pop3.LastError(s, 255));
      return FALSE;
    }

    mail = new int[num_mails];
    size = new int[num_mails];

    num_mails = g_pop3.List(mail, size, num_mails);

    for (i = 0; i < num_mails; i++)
      printf("%i\t%i\n", mail[i], size[i]);

    delete mail;
    delete size;
  }

// header <no.>

  else if (strcmp(Command, "header") == 0)
  { if (!strtoint(Arg1, &i))
    { printf("The message number %s is not a valid integer\n", Arg1);
    }
    else
    { GetMessage(i, TRUE, NULL);
    }
  }

// read <no.>
// retr <no.>

  else if (strcmp(Command, "read") == 0 || strcmp(Command, "retr") == 0)
  { if (!strtoint(Arg1, &i))
    { printf("The message number %s is not a valid integer\n", Arg1);
    }
    else
    { GetMessage(i, FALSE, NULL);
    }
  }

// file <no.> <file>
// file all

  else if (strcmp(Command, "file") == 0)
  { if (strcmp(Arg1, "all") == 0)
    { FileAllMessages();
    }
    else
    { if (!strtoint(Arg1, &i))
      { printf("The message number %s is not a valid integer\n", Arg1);
      }
      else if (strlen(Arg2) == 0)
      { printf("You must supply the file name to file the message\n");
      }
      else
      { GetMessage(i, FALSE, Arg2);
      }
    }
  }

// del <no.>
// del all

  else if (strcmp(Command, "del") == 0)
  { if (strcmp(Arg1, "all") == 0)
    { DeleteMessage(-1);
    }
    else
    { if (!strtoint(Arg1, &i))
      { printf("The message number %s is not a valid integer\n", Arg1);
      }
      else
      { DeleteMessage(i);
      }
    }
  }

// filter check <filter>
// filter delete <filter>

  else if (strcmp(Command, "filter") == 0)
  { if (strlen(Arg2) == 0)
    { printf("You must supply the filter expression as the second argument\n");
      return FALSE;
    }

    if (strcmp(Arg1, "check") == 0)
    { FilterMessage(Arg2, FALSE);
    }
    else if (strcmp(Arg1, "delete") == 0)
    { FilterMessage(Arg2, TRUE);
    }
    else
    { printf("Use the command \"filter check\" to see mails matching the filter\n");
      printf("Use the command \"filter delete\" to delete mails matching the filter\n");
    }
  }

// reset

  else if (strcmp(Command, "reset") == 0)
  { Reset();
  }

// help

  else if (strcmp(Command, "help") == 0)
  { printf(HELP);
  }

// Unrecognised commad

  else
  { return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// GetMessage
// ----------
// Retrieve a message by number.
// If HeaderOnly is true just retrieve the header.
// If Filename is non-null write the message to that file.
//**********************************************************************

BOOL GetMessage(int Msg, BOOL HeaderOnly, const char* Filename)
{ char s[256];

// Get the message

  if (!g_msg.ReadMessage(Msg, HeaderOnly))
  { printf("%s\n", g_msg.LastError(s, 255));
    return FALSE;
  }

// Save or display the message

  if (Filename)
  { if (!g_msg.SaveToFile(Filename))
    { printf("%s\n", g_msg.LastError(s, 255));
      return FALSE;
    }
    else
    { printf("Message %i saved to %s\n", Msg, Filename);
    }
  }
  else
  { g_msg.SaveToFile(stdout);
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// FileAllMessages
// ---------------
// Retrieve all messages and write them to a file.
// The file names are msgXXXX where XXXX is the message number.
//**********************************************************************

BOOL FileAllMessages(void)
{ int num_mails, total_size, i;
  int* mail;
  int* size;
  char s[256];

// Get the total number of e-mails

  if (!g_pop3.Stat(&num_mails, &total_size))
  { printf("Error, STAT returned: %s\n", g_pop3.LastError(s, 255));
    return FALSE;
  }

// Allocate buffers for the mail numbers and sizes

  mail = new int[num_mails];
  size = new int[num_mails];

// Get the numbers and sizes of the e-mails

  num_mails = g_pop3.List(mail, size, num_mails);

// Go through the e-mails saving each one to a file

  for (i = 0; i < num_mails; i++)
  { sprintf(s, "msg%04i.txt", mail[i]);
    if (!GetMessage(mail[i], FALSE, s))
      break;
  }

  delete mail;
  delete size;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// FilterMessage
// -------------
// List all messages containing the filter text.
// If Delete is true delete the messages.
//**********************************************************************

BOOL FilterMessage(const char* Filter, BOOL Delete)
{ int num_mails, total_size, i;
  int* mail;
  int* size;
  char s[256];

  printf("Filtering messages containing \"%s\":\n", Filter);
  if (Delete)
    printf("Filtered messages will be deleted\n");

// Get the total number of e-mails

  if (!g_pop3.Stat(&num_mails, &total_size))
  { printf("Error, STAT returned: %s\n", g_pop3.LastError(s, 255));
    return FALSE;
  }

// Allocate buffers for the mail numbers and sizes

  mail = new int[num_mails];
  size = new int[num_mails];

// Get the numbers and sizes of the e-mails

  num_mails = g_pop3.List(mail, size, num_mails);

// Go through the e-mails filtering each one

  for (i = 0; i < num_mails; i++)
  { printf("%i - %i: ", mail[i], size[i]);

// Read the next header

    if (!g_msg.ReadMessage(mail[i], TRUE))
    { printf("%s\n", g_msg.LastError(s, 255));
      break;
    }

// Check it to see if it contains the filter string

    if (g_msg.Filter(Filter))
    { if (Delete)
      { if (!g_pop3.Delete(mail[i]))
          printf("Delete failed: %s\n", g_pop3.LastError(s, 255));
        else
          printf("Deleted\n");
      }
      else
      { printf("Matched\n");
      }
    }
    else
    { printf("Not matched\n");
    }
  }

// Delete buffers

  delete mail;
  delete size;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// DeleteMessage
// -------------
// Delete a message by number.
// If Msg is -1 delete all messages.
//**********************************************************************

BOOL DeleteMessage(int Msg)
{ char s[256];

// If Msg is not -1 just delete the one message

  if (Msg != -1)
  { if (!g_pop3.Delete(Msg))
    { printf("%s\n", g_pop3.LastError(s, 255));
      return FALSE;
    }
    else
    { return TRUE;
    }
  }

// Else delete all messages

  printf("delete all is not implemented yet\n");

// Return indicating success

  return TRUE;
}


//**********************************************************************
// Reset
// -----
// Abandon changes i.e. deletions
//**********************************************************************

BOOL Reset(void)
{ char s[256];

// Send the RSET comamnd

  if (!g_pop3.Reset())
  { printf("%s\n", g_pop3.LastError(s, 255));
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// GetCommand
// ----------
// Get a command from the user and parse the arguments
//**********************************************************************

BOOL GetCommand(char* Command, char* Arg1, char* Arg2)
{ char s[256];

  strcpy(Command, "");
  strcpy(Arg1, "");
  strcpy(Arg2, "");

// Get the command

  printf("POP3> ");

  if (!fgets(s, 255, stdin))
    return FALSE;

// Parse the command

  if (!ParseCommand(s, Command, Arg1, Arg2))
    return FALSE;

// All done

  return TRUE;
}


//**********************************************************************
// ParseCommand
// ------------
// Parse a command string
//**********************************************************************

BOOL ParseCommand(char* Input, char* Command, char* Arg1, char* Arg2)
{ int i, j;
  strcpy(Command, "");
  strcpy(Arg1, "");
  strcpy(Arg2, "");

// Parse the command and lowercase it

  for (i = 0; Input[i] == ' '; i++);

  for (j = 0; Input[i] != ' ' && Input[i] != '\n' && Input[i] != '\0'; i++, j++)
    Command[j] = Input[i];
  Command[j] = '\0';

  if (j == 0)
    return FALSE;

  CharLower(Command);

// Parse the first argument and lowercase it

  while (Input[i] == ' ')
    i++;

  for (j = 0; Input[i] != ' ' && Input[i] != '\n' && Input[i] != '\0'; i++, j++)
    Arg1[j] = Input[i];
  Arg1[j] = '\0';

  CharLower(Arg1);

// The second argument is what's left. This has spaces trimmed but
// it is not lower cased.

  while (Input[i] == ' ')
    i++;

  for (j = 0; Input[i] != '\n' && Input[i] != '\0'; i++, j++)
    Arg2[j] = Input[i];
  Arg2[j] = '\0';
  strtrim(Arg2);

// All done

  return TRUE;
}


