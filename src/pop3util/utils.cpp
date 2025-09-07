//**********************************************************************
// utils.cpp
// =========
// Various useful routines used by the POP3Util application
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 07/05/2005
//**********************************************************************

#define STRICT
#include <windows.h>
#include <stdio.h>
#include <string.h>


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


//**********************************************************************
// strtrim
// -------
// Remove leading and trailing spaces from a string in situ
//**********************************************************************

char* strtrim(char* s)
{ int start, i;

// Find the first non-space character

  for (start = 0; s[start] == ' '; start++);

// Find the end of the string

  for (i = start; s[i] != '\0'; i++);

// Work back from the end of the string removing trailing spaces

  if (i > 0)
    while (i > 0 && s[i-1] == ' ')
      i--;

  s[i] = '\0';

// Now copy the string backwards over any leading spaces

  if (start > 0)
    for (i = 0; s[start] != '\0'; i++, start++)
      s[i] = s[start];

// Return the trimmed string

  return(s);
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
//**********************************************************************

const char* GetLastErrorMessage(void)
{ DWORD d;
  static char errmsg[1024];

  d = GetLastError();

  strcpy(errmsg, "<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 1023, NULL);
  return(errmsg);
}


