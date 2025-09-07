//**********************************************************************
// CSNTPTimestamp
// ==============
//
// Class to encapsulate an SNTP timestamp.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 19th June 1999
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include "CSNTPTimestamp.h"


//**********************************************************************
// Constants used by SNTPTimestamp class
// -------------------------------------
//**********************************************************************

#define SECONDS_1_1_1999      3124137600
#define SECONDS_PER_LEAPYEAR    31622400
#define SECONDS_PER_YEAR        31536000
#define SECONDS_PER_DAY            86400


//**********************************************************************
// CSNTPTimestamp::SetSystemTime
// -----------------------------
// Set the timestamp from the supplied SYSTEMTIME
//**********************************************************************

BOOL CSNTPTimestamp::SetSystemTime(SYSTEMTIME* st)
{ WORD w;

// Tedious but necessary checks that the date is valid

  if (st->wYear < 1999) // 1999 is our base date
    return FALSE;

  if (st->wMonth < 1 || st->wMonth > 12)
    return FALSE;

  if (st->wDay < 1 || st->wDay > DaysPerMonth(st->wMonth, st->wYear))
    return FALSE;

  if (st->wHour < 0 || st->wHour > 23)
    return FALSE;

  if (st->wMinute < 0 || st->wMinute > 59)
    return FALSE;

  if (st->wSecond < 0 || st->wSecond > 59)
    return FALSE;

  if (st->wMilliseconds < 0 || st->wMilliseconds > 999)
    return FALSE;

// Calculate the timestamp

  seconds = SECONDS_1_1_1999;

  for (w = st->wYear; w > 1999; w--)
  { if ((w - 1) % 4 == 0) // NB 2000 is a leap year; I think we can ignore the year 2100 :-)
      seconds += SECONDS_PER_LEAPYEAR;
    else
      seconds += SECONDS_PER_YEAR;
  }

  for (w = 1; w < st->wMonth; w++)
    seconds += SECONDS_PER_DAY*DaysPerMonth(w, st->wYear);

  seconds += SECONDS_PER_DAY*(st->wDay - 1) + 3600*st->wHour + 60*st->wMinute + st->wSecond;

  fraction = ((st->wMilliseconds*0x10000)/1000) << 16;

// Return the bytes to NTP order

  seconds = SwapBytes(seconds);
  fraction = SwapBytes(fraction);

// All done

  return TRUE;
}


//**********************************************************************
// CSNTPTimestamp::GetSystemTime
// -----------------------------
// Get the time from the timestamp as a SYSTEMTIME
//**********************************************************************

void CSNTPTimestamp::GetSystemTime(SYSTEMTIME* st)
{ DWORD secs;

// Initialise the date to midnight, 1st Jan 1999

  st->wDay = st->wMonth = 1;
  st->wYear = 1999;
  st->wHour = st->wMinute = st->wSecond = st->wMilliseconds = 0;

  secs = SwapBytes(seconds) - SECONDS_1_1_1999;

// Calculate the year; have to account for leap years

  for (;;)
  { if (st->wYear % 4 == 0) // NB 2000 is a leap year; I think we can ignore the year 2100 :-)
    { if (secs < SECONDS_PER_LEAPYEAR)
        break;
      else
        secs -= SECONDS_PER_LEAPYEAR;
    }
    else
    { if (secs < SECONDS_PER_YEAR)
        break;
      else
        secs -= SECONDS_PER_YEAR;
    }

    st->wYear++;
  }

// Got the year, the rest is easy.

  for (st->wMonth = 1; st->wMonth <= 12; st->wMonth++)
  { if (secs < SECONDS_PER_DAY*DaysPerMonth(st->wMonth, st->wYear))
      break;
    secs -= SECONDS_PER_DAY*DaysPerMonth(st->wMonth, st->wYear);
  }

  st->wDay = (WORD) (secs/SECONDS_PER_DAY + 1);
  secs -= (st->wDay - 1)*SECONDS_PER_DAY;

  st->wHour = (WORD) (secs/3600);
  secs -= st->wHour*3600;

  st->wMinute = (WORD) (secs/60);
  secs -= st->wMinute*60;

  st->wSecond = (WORD) secs;

// We only want the nearest millisecond here so take only the 16 most
// significant bits of the fraction.

  st->wMilliseconds = (WORD) (((SwapBytes(fraction)/65536)*1000)/65536);
}


//**********************************************************************
// CSNTPTimestamp::DaysPerMonth
// ----------------------------
// Return the number of days in the given month in the given year
//**********************************************************************

DWORD CSNTPTimestamp::DaysPerMonth(int Month, int Year)
{ DWORD days;
  static DWORD days_per_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (Month < 1 || Month > 12)
  { return 0;
  }
  else if (Month == 2 && Year % 4 == 0) // NB 2000 is a leap year; I think we can ignore the year 2100 :-)
  { days = 29;
  }
  else
  { days = days_per_month[Month-1];
  }

  return days;
}


//**********************************************************************
// CSNTPTimestamp::SwapBytes
// -------------------------
// Swap the byte order between big-endian and little-endian
//**********************************************************************

DWORD CSNTPTimestamp::SwapBytes(DWORD d)
{
  return ((d & 0xff) << 24) + (((d >> 8) & 0xff) << 16) + (((d >> 16) & 0xff) << 8) + ((d >> 24) &0xff);
}


