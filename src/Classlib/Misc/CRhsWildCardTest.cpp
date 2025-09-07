#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <stdio.h>
#include "CRhsWildCard.h"

int wmain(int argc, WCHAR* argv[])
{ BOOL b;
  CRhsWildCard wc;
  WCHAR s[256];

  b = wc.Complete(argv[1], argv[2], argv[3], s, 256);

  if (!b)
  { wprintf(L"Error: %s\n", wc.LastError());
  }
  { wprintf(L"%s\n", s);
  }

  return 0;
}
