//**********************************************************************
// CRhsWildCard
// ============
// Class to find files
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <stdio.h>
#include "CRhsWildCard.h"


//**********************************************************************
// CRhsWildCard
// ------------
//**********************************************************************

CRhsWildCard::CRhsWildCard()
{
  m_CaseSensitive = FALSE;
  lstrcpy(m_LastError, L"");
}

CRhsWildCard::~CRhsWildCard()
{
}


//**********************************************************************
// Match
// -----
// Test if the string Source matches the wildcard pattern SrcPat
//**********************************************************************

BOOL CRhsWildCard::Match(const WCHAR* SrcPat, const WCHAR* Source)
{

// Return indicating that the string matches the pattern

  return TRUE;
}


//**********************************************************************
// Complete
// --------
// Use SrcPat and Source to identify the text represented by wildcards
// and substitute the text in DstPat to build the string Destination.
//**********************************************************************

BOOL CRhsWildCard::Complete(const WCHAR* SrcPat, const WCHAR* Source, const WCHAR* DstPat, WCHAR* Destination, int BufLen)
{ int bufend, srcpat, source, dstpat, dest, i, j;
  const WCHAR* p;
  WCHAR s[256];

// bufend is the offset of the last charactert in the buffer

  bufend = BufLen - 1;

// Loop over all characters in DstPat

  srcpat = source = dstpat = dest = 0;

  while (SrcPat[srcpat] != '\0')
  {

// Step through SrcPat and Source until we hit a * or ? (or \0)
// - if SrcPat and Source don't match flag an error

    while (SrcPat[srcpat] != '*' && SrcPat[srcpat] != '?' && SrcPat[srcpat] != '\0')
    { if (!CompareChar(SrcPat[srcpat], Source[source]))
      { lstrcpy(m_LastError, L"Source pattern and source string do not match");
        return FALSE;
      }

      srcpat++;
      source++;
    }

// Copy from DstPat to Destination until we hit a * or ? (or \0)
// - if SrcPat and DstPat don't match flag an error

    while (DstPat[dstpat] != '*' && DstPat[dstpat] != '?' && DstPat[dstpat] != '\0')
    { if (dest >= bufend)
      { Destination[bufend] = '\0';
        lstrcpy(m_LastError, L"Destination buffer is not long enough");
        return FALSE;
      }
      Destination[dest++] = DstPat[dstpat++];
    }

    if (!CompareChar(SrcPat[srcpat], DstPat[dstpat]))
    { lstrcpy(m_LastError, L"Source and destination patterns do not match");
      return FALSE;
    }

// If we hit a ? we just need to copy one character from Source to
// Destination

    if (SrcPat[srcpat] == '?')
    { if (Source[source] == '\0')
      { lstrcpy(m_LastError, L"End of source string when matching ? in source pattern");
        return FALSE;
      }

      if (dest >= bufend)
      { Destination[bufend] = '\0';
        lstrcpy(m_LastError, L"Destination buffer is not long enough");
        return FALSE;
      }

      Destination[dest++] = Source[source++];
      srcpat++;
      dstpat++;
    }

// Processing * is more complicated.

    if (SrcPat[srcpat] == '*')
    {

// Multiple * get treated as one; jump past repeated *

      for (i = srcpat; SrcPat[i] == '*'; i++);

// Get the part of SrcPat following the wildcard

      for (j = 0; SrcPat[i] != '*' && SrcPat[i] != '?' && SrcPat[i] != '\0'; i++, j++)
        s[j] = SrcPat[i];
      s[j] = '\0';

// Now find this part of SrcPat in Source. If s = "" the * matches the
// remaining part of Source.

      if (s[0] == '\0')
      { i = lstrlen(Source);
      }
      else
      { p = FindString(Source+source, s);
        if (!p)
        { lstrcpy(m_LastError, L"End of source string when matching * in source pattern");
          return FALSE;
        }

        i = p - Source;
      }

// At this point Source[i] is the character after the end of the part of
// Source that matches the * in SrcPat. Copy this part to Destination.

      while (source < i)
      { if (dest >= bufend)
        { Destination[bufend] = '\0';
          lstrcpy(m_LastError, L"Destination buffer is not long enough");
          return FALSE;
        }
        Destination[dest++] = Source[source++];
      }

// And advance past the * in SrcPat and DstPat

      while (SrcPat[srcpat] == '*')
        srcpat++;
      dstpat++;
    }
  }

// Finally tack on any remaining bits of DstPat

  while (DstPat[dstpat] != '\0')
  { if (dest >= bufend)
    { Destination[bufend] = '\0';
      lstrcpy(m_LastError, L"Destination buffer is not long enough");
      return FALSE;
    }
    Destination[dest++] = DstPat[dstpat++];
  }
  Destination[dest] = '\0';

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CompareChar
// -----------
// Compare the two characters and return TRUE if they are the same or
// FALSE if they differ. The member variable m_CaseSensitive determines
// whether the comparison is case sensitive.
//**********************************************************************

BOOL CRhsWildCard::CompareChar(WCHAR a, WCHAR b)
{

// If the cmparison is case sensitive the comparison is straightforward

  if (m_CaseSensitive)
  { if (a == b)
      return TRUE;
    else
      return FALSE;
  }

// Otherwise we have to lowercase both characters then compare them

  else
  { if (CharLower((LPTSTR) a) == CharLower((LPTSTR) b))
      return TRUE;
    else
      return FALSE;
  }
}


//**********************************************************************
// FindString
// ----------
// Find a substring within a string and return a pointer to the first
// occurrence of the substring.
// The search is case insensitive.
//**********************************************************************

const WCHAR* CRhsWildCard::FindString(const WCHAR* searchin, const WCHAR* searchexpr)
{ const WCHAR *cp = (WCHAR *) searchin;
  const WCHAR *s1, *s2;

  if (!*searchexpr)
    return searchin;

  while (*cp)
  { s1 = cp;
    s2 = searchexpr;

    while (*s1 && *s2 && CompareChar(*s1, *s2))
      s1++, s2++;

    if (!*s2)
      return(cp);

    cp++;
  }

  return(NULL);
}


