// *********************************************************************
// CSearchExpr.h
// =============
// *********************************************************************

#include <windows.h>
#include <string.h>
#include "CSearchExpr.h"


//**********************************************************************
// Prototypes
// ----------
//**********************************************************************

char* strstri(const char* Str, const char* SubStr);


//**********************************************************************
// CSearchExpr
// -----------
// A search expression is a string of the form:
//   "aaa;bbb;ccc"
// that is a number of substrings separated by semi-colons.
// The CSearchExpr checks if a string has ALL of the substrings.
//**********************************************************************

CSearchExpr::CSearchExpr()
{
  num_expr = 0;
}


//**********************************************************************
// CSearchExpr::ParseExpr
// ----------------------
// Take the input expression "aaa;bbb;ccc" and split it up into the
// individual search elements
//**********************************************************************

BOOL CSearchExpr::ParseExpr(const char* SearchExpr)
{
  int i = 0, j = 0;

  num_expr = 0;

  while (SearchExpr[i] != '\0')
  {
    for (j = 0; SearchExpr[i] != ';' && SearchExpr[i] != '\0'; i++, j++)
      text[num_expr][j] = SearchExpr[i];
    text[num_expr][j] = '\0';

    if (SearchExpr[i] == ';')
      i++;

    if (j > 0)
    {
      num_expr++;
      if (num_expr >= MAX_SEARCHTEXT)
        break;
    }
  }

  if (num_expr == 0)
    return FALSE;

  return TRUE;
}


//**********************************************************************
// CSearchExpr::FindText
// ---------------------
// See if the supplied text matches any of the search expressions
//**********************************************************************

BOOL CSearchExpr::FindText(const char* Input, BOOL IgnoreCase)
{ int i;
  BOOL hit = TRUE;

// There must be at least one search expression

  if (num_expr == 0)
    return FALSE;

// Case insensitive search

  if (IgnoreCase)
  { for (i = 0; i < num_expr; i++)
    {
      // We must match all expressions. On any miss stop immediately
      if (!strstri(Input, text[i]))
      { hit = FALSE;
        break;
      }
    }
  }

// Case sensitive search

  else
  { for (i= 0; i < num_expr; i++)
    {
      // We must match all expressions. On any miss stop immediately
      if (!strstr(Input, text[i]))
      { hit = FALSE;
        break;
      }
    }
  }

// Return indicating whether a match was found

  return hit;
}


//***********************************************************************
// strstri
// =======
// Case insensitive version of strstr
//***********************************************************************

static char IStrMap[256] =
{
  '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
  '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
  '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
  '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
  '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',  //  !"#$%&'
  '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',  // ()*+,-./
  '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',  // 01234567
  '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',  // 89:;<=>?
  '\x40', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67',  // @abcdefg
  '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',  // hijklmno
  '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',  // pqrstuvw
  '\x78', '\x79', '\x7a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',  // xyz[\]^_
  '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67',  // `abcdefg
  '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',  // hijklmno
  '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',  // pqrstuvw
  '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',  // xyz{|}~
  '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
  '\x88', '\x89', '\x9a', '\x8b', '\x9c', '\x8d', '\x9e', '\x8f',
  '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
  '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\xff',
  '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',  // ��������
  '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',  // ��������
  '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',  // ��������
  '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',  // ��������
  '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',  // ��������
  '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',  // ��������
  '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xd7',  // ��������
  '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xdf',  // ��������
  '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',  // ��������
  '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',  // ��������
  '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',  // ��������
  '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff'   // ��������
};


char* strstri(const char* Str, const char* SubStr)
{
  // Use unsigned char otherwise values > 127 will give a negative
  // offset into IStrMap.
  const unsigned char* str;
  const unsigned char* str2;
  const unsigned char* substr;

  for (str = (unsigned char*) Str; *str != '\0'; str++)
  { for (str2 = str, substr = (unsigned char*) SubStr; *str2 != '\0' && *substr != '\0'; str2++, substr++)
    {
      // . matches anything
      if (*substr == '.')
        continue;
      // If we have a mismatch stop immediately
      if (IStrMap[*str2] != IStrMap[*substr])
        break;
    }

    if (*substr == '\0')
      return((char*) str);
  }

  return NULL;
}


