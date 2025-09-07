//**********************************************************************
// CRhsDate
// ========
//
// 23/4/97 Modified to add SetDate and SetTime from strings, and add
// Style argument to Format.
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <stdio.h>
#include "CRhsDate.h"


//**********************************************************************
// Global variables
//**********************************************************************

static BOOL strtoint(WCHAR* s, int* d);


//**********************************************************************
// There isn't much for the constructor to do
//**********************************************************************

CRhsDate::CRhsDate()
{
  m_Date = (RHSDATE) -1;
}

CRhsDate::~CRhsDate()
{
}


//**********************************************************************
// Assignment just copies the internal double
//**********************************************************************

CRhsDate& CRhsDate::operator=(CRhsDate& Date)
{
  m_Date = Date.m_Date >= 0 ? Date.m_Date : -1;
  return(*this);
}


//**********************************************************************
// We can add intervals
//**********************************************************************

CRhsDate& CRhsDate::operator+(CRhsDateInterval& Add)
{ CRhsDate* rhsdate;

  if (!Add.IsValid())
    return(*this);

  rhsdate = new CRhsDate(*this);
  *rhsdate = *this;
  *rhsdate += Add;

  return(*rhsdate);
}


CRhsDate& CRhsDate::operator+=(CRhsDateInterval& Add)
{ int months, years, leaps, thismonth, thisyear;
  CRhsDateInterval interval;
  SYSTEMTIME st;

// Check the interval is valid

  if (!Add.IsValid())
    return(*this);

// Add the interval

  switch (Add.GetType())
  {

// Seconds, minutes, hours, days and weeks are easy

    case CRHSDATEINTERVAL_SECOND:
      m_Date += Add.GetCount()/86400.0; // 86400 = 24*60*60 = seconds per day;
      break;

    case CRHSDATEINTERVAL_MINUTE:
      m_Date += Add.GetCount()/1440.0; // 1440 = 24*60 = minutes per day;
      break;

    case CRHSDATEINTERVAL_HOUR:
      m_Date += Add.GetCount()/24.0;
      break;

    case CRHSDATEINTERVAL_DAY:
      m_Date += Add.GetCount();
      break;

    case CRHSDATEINTERVAL_WEEK:
      m_Date += Add.GetCount()*7;
      break;

// Months are tricky because of the variable number of days per month

    case CRHSDATEINTERVAL_MONTH:
      GetDate(&st);
      st.wMonth += Add.GetCount();

      if (st.wMonth > 12)
      { st.wYear += (st.wMonth - 1)/12;
        st.wMonth = ((st.wMonth - 1) % 12) + 1;
      }

      if (st.wDay == 29 && st.wMonth == 2 && st.wYear % 4 != 0)
      { st.wDay = 1;
        st.wMonth = 3;
      }
      SetDate(&st);
      break;

// Years are tricky because of the variable number of days per year

    case CRHSDATEINTERVAL_YEAR:
      GetDate(&st);
      st.wYear += Add.GetCount();
      if (st.wDay == 29 && st.wMonth == 2 && st.wYear % 4 != 0)
      { st.wDay = 1;
        st.wMonth = 3;
      }
      SetDate(&st);
      break;
  }

  return(*this);
}


//**********************************************************************
// We can subtract intervals
//**********************************************************************

CRhsDate& CRhsDate::operator-(CRhsDateInterval& Subtract)
{ CRhsDate* rhsdate;

  if (!Subtract.IsValid())
    return(*this);

  rhsdate = new CRhsDate(*this);
  *rhsdate = *this;
  *rhsdate -= Subtract;

  return(*rhsdate);
}


CRhsDate& CRhsDate::operator-=(CRhsDateInterval& Subtract)
{ int months, years, leaps, thismonth, thisyear;
  double date;
  CRhsDateInterval interval;
  SYSTEMTIME st;

// Check the interval is valid

  if (!Subtract.IsValid())
    return(*this);

// Add the interval

  date = m_Date;

  switch (Subtract.GetType())
  {

// Seconds, minutes, hours, days and weeks are easy

    case CRHSDATEINTERVAL_SECOND:
      date -= Subtract.GetCount()/86400.0; // 86400 = 24*60*60 = seconds per day;
      break;

    case CRHSDATEINTERVAL_MINUTE:
      date -= Subtract.GetCount()/1440.0; // 1440 = 24*60 = minutes per day;
      break;

    case CRHSDATEINTERVAL_HOUR:
      date -= Subtract.GetCount()/24.0;
      break;

    case CRHSDATEINTERVAL_DAY:
      date -= Subtract.GetCount();
      break;

    case CRHSDATEINTERVAL_WEEK:
      date -= Subtract.GetCount()*7;
      break;

// Months are tricky because of the variable number of days per month

    case CRHSDATEINTERVAL_MONTH:
      GetDate(&st);
      months = st.wMonth;
      months -= Subtract.GetCount();

      if (months < 1)
      { years = (months/12) - 1;
        st.wYear = (WORD) ((int) st.wYear + years);
        st.wMonth = (WORD) (months - years*12);
      }
      else
      { st.wMonth = (WORD) months;
      }

      if (st.wDay == 29 && st.wMonth == 2 && st.wYear % 4 != 0)
      { st.wDay = 1;
        st.wMonth = 3;
      }
      SetDate(&st);
      date = m_Date;
      break;

// Years are tricky because of the variable number of days per year

    case CRHSDATEINTERVAL_YEAR:
      GetDate(&st);
      st.wYear -= Subtract.GetCount();
      if (st.wDay == 29 && st.wMonth == 2 && st.wYear % 4 != 0)
      { st.wDay = 1;
        st.wMonth = 3;
      }
      SetDate(&st);
      date = m_Date;
      break;
  }

// Do the subtraction only if it would produce a valid date

  if (date > 0)
    m_Date = date;

  return(*this);
}


//**********************************************************************
// Variety of comparison tests
//**********************************************************************

int CRhsDate::operator==(CRhsDate& Test)
{
  return(m_Date == Test.m_Date ? 1 : 0);
}


int CRhsDate::operator!=(CRhsDate& Test)
{
  return(m_Date != Test.m_Date ? 1 : 0);
}


int CRhsDate::operator<(CRhsDate& Test)
{
  if (m_Date < 0 && Test.m_Date < 0)
    return(0);
  else if (m_Date < Test.m_Date)
    return(1);
  else
    return(0);
}


int CRhsDate::operator>(CRhsDate& Test)
{
  if (m_Date < 0 && Test.m_Date < 0)
    return(0);
  else if (m_Date > Test.m_Date)
    return(1);
  else
    return(0);
}


int CRhsDate::operator<=(CRhsDate& Test)
{
  if (m_Date < 0 && Test.m_Date < 0)
    return(0);
  else if (m_Date <= Test.m_Date)
    return(1);
  else
    return(0);
}


int CRhsDate::operator>=(CRhsDate& Test)
{
  if (m_Date < 0 && Test.m_Date < 0)
    return(0);
  else if (m_Date >= Test.m_Date)
    return(1);
  else
    return(0);
}


//**********************************************************************
//**********************************************************************

BOOL CRhsDate::SetDate(int Year, int Month, int Day, int Hour, int Minute, int Second)
{

// Two digit years have the current century added

  if (Year >= 0 && Year < 100)
  { SYSTEMTIME st;
    GetLocalTime(&st);
    Year += st.wYear - (st.wYear % 100);
  }

// Check the date is valid

  if (Year < 1900 || Year > 9999)
    return(FALSE);

  if (Month < 1 || Month > 31)
    return(FALSE);

  if (Day < 1 || Day > DaysPerMonth(Month, Year))
    return(FALSE);

  if (Hour < 0 || Hour > 23)
    return(FALSE);

  if (Minute < 0 || Minute > 59)
    return(FALSE);

  if (Second < 0 || Second > 59)
    return(FALSE);

// The date is OK so convert it

  m_Date = DateTimeToDouble(Year, Month, Day, Hour, Minute, Second);

// And return indicating success

  return(TRUE);
}

BOOL CRhsDate::SetDate(SYSTEMTIME* st)
{
  return(SetDate(st->wYear, st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond));
}

void CRhsDate::SetDate(void)
{ SYSTEMTIME st;

  GetLocalTime(&st);
  SetDate(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

void CRhsDate::SetDate(double RawDate)
{
  m_Date = RawDate;
}


//**********************************************************************
// Set the date and time from strings
//**********************************************************************

BOOL CRhsDate::SetDate(WCHAR* Date, WCHAR* Time, BOOL PM)
{ RHSDATE old_date;

// Some obvious checks to avoid GPFs

  if (!Date || !Time)
    return(FALSE);

// Set the date and time

  old_date = m_Date;

  if (!SetDate(Date) || !SetTime(Time, PM))
  { m_Date = old_date;
    return(FALSE);
  }

// Return indicating success

  return(TRUE);
}


//**********************************************************************
// Set the date from a string
//**********************************************************************

BOOL CRhsDate::SetDate(WCHAR* Date)
{ int first, second, third, order;
  WORD century;
  WCHAR sep;
  WCHAR* p;
  WCHAR* one;
  WCHAR* two;
  WCHAR* three;
  SYSTEMTIME st;
  WCHAR strtime[64], s[256];

// Some obvious checks to avoid GPFs

  if (!Date)
    return(FALSE);

// First split the string into three parts

  GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDATE, s, 255);
  sep = s[0];

  lstrcpyn(strtime, Date, 64);
  p = strtime;

  for ( ; *p == ' ' || *p == '\t'; p++);
  for (one = p; *p != ' ' && *p != '\t' && *p != sep && *p != '\0'; p++);
  if (*p == ' ' || *p == '\t')
    for (*p++ = '\0'; *p != sep && *p != '\0'; p++);
  if (*p == sep)
    *p++ = '\0';

  for (; *p == ' ' || *p == '\t'; p++);
  for (two = p; *p != ' ' && *p != '\t' && *p != sep && *p != '\0'; p++);
  if (*p == ' ' || *p == '\t')
    for (*p++ = '\0'; *p != sep && *p != '\0'; p++);
  if (*p == sep)
    *p++ = '\0';

  for (; *p == ' ' || *p == '\t'; p++);
  for (three = p; *p != ' ' && *p != '\t' && *p != sep && *p != '\0'; p++);
  if (*p == ' ' || *p == '\t')
    for (*p++ = '\0'; *p != sep && *p != '\0'; p++);
  if (*p == sep)
    *p++ = '\0';

// Now convert the three parts into integers

  if (!strtoint(one, &first) || !strtoint(two, &second) || !strtoint(three, &third))
    return(FALSE);

  order = 0;
  GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE, s, 2);

  GetLocalTime(&st);
  century = st.wYear - st.wYear % 100;

  DoubleToDateTime(m_Date, &st);

  switch (s[0])
  { case '1': // 1 Day - Month - Year
      st.wDay   = (WORD) first;
      st.wMonth = (WORD) second;
      st.wYear  = (WORD) third;
      break;

    case '2': // 2 Year - Month - Day
      st.wYear  = (WORD) first;
      st.wMonth = (WORD) second;
      st.wDay   = (WORD) third;
      break;

    default: // 0 Month - Day - Year
      st.wMonth = (WORD) first;
      st.wDay   = (WORD) second;
      st.wYear  = (WORD) third;
      break;
  }

  if (st.wYear >= 0 && st.wYear <= 99)
    st.wYear += century;

  st.wDayOfWeek = (WORD) -1;

// Check the date by attempting to format it

  if (GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, s, 255) == 0)
    return(0);

// All present and correct, save the time

  m_Date = DateTimeToDouble(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

// And return indicating success

  return(TRUE);
}


//**********************************************************************
// Set the time from a string
//**********************************************************************

BOOL CRhsDate::SetTime(WCHAR* Time, BOOL PM)
{ int hour, minute, second;
  WCHAR sep;
  WCHAR* p;
  WCHAR* one;
  WCHAR* two;
  WCHAR* three;
  SYSTEMTIME st;
  WCHAR strtime[64], s[256];

// Some obvious checks to avoid GPFs

  if (!Time)
    return(FALSE);

// First split the string into three parts

  GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, s, 255);
  sep = s[0];

  lstrcpyn(strtime, Time, 64);
  p = strtime;

  for ( ; *p == ' ' || *p == '\t'; p++);
  for (one = p; *p != ' ' && *p != '\t' && *p != sep && *p != '\0'; p++);
  if (*p == ' ' || *p == '\t')
    for (*p++ = '\0'; *p != sep && *p != '\0'; p++);
  if (*p == sep)
    *p++ = '\0';

  for (; *p == ' ' || *p == '\t'; p++);
  for (two = p; *p != ' ' && *p != '\t' && *p != sep && *p != '\0'; p++);
  if (*p == ' ' || *p == '\t')
    for (*p++ = '\0'; *p != sep && *p != '\0'; p++);
  if (*p == sep)
    *p++ = '\0';

  for (; *p == ' ' || *p == '\t'; p++);
  for (three = p; *p != ' ' && *p != '\t' && *p != sep && *p != '\0'; p++);
  if (*p == ' ' || *p == '\t')
    for (*p++ = '\0'; *p != sep && *p != '\0'; p++);
  if (*p == sep)
    *p++ = '\0';

  if (*three == '\0') // Seconds may be omitted and default to 0
    three = L"0";

// Now convert the three parts into integers

  if (!strtoint(one, &hour) || !strtoint(two, &minute) || !strtoint(three, &second))
    return(FALSE);

  if (PM)
    if (hour >= 0 && hour <= 11)
      hour += 12;

// Check the time by attempting to format it

  FillMemory(&st, sizeof(SYSTEMTIME), 0);
  st.wHour   = (WORD) hour;
  st.wMinute = (WORD) minute;
  st.wSecond = (WORD) second;

  if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, s, 255) == 0)
    return(FALSE);

// All present and correct, save the time

  DoubleToDateTime(m_Date, &st);
  st.wHour   = (WORD) hour;
  st.wMinute = (WORD) minute;
  st.wSecond = (WORD) second;
  m_Date = DateTimeToDouble(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

// And return indicating success

  return(TRUE);
}


//**********************************************************************
//**********************************************************************

void CRhsDate::GetDate(SYSTEMTIME* st)
{
  DoubleToDateTime(m_Date, st);
}


void CRhsDate::GetDate(int* Year, int* Month, int* Day, int* Hour, int* Minute, int* Second, int* DayOfWeek)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  *Day       = (int) st.wDay;
  *Month     = (int) st.wMonth;
  *Year      = (int) st.wYear;
  *Hour      = (int) st.wHour;
  *Minute    = (int) st.wMinute;
  *Second    = (int) st.wSecond;
  *DayOfWeek = (int) st.wDayOfWeek;
}


//**********************************************************************
//**********************************************************************

int CRhsDate::Day(void)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  return((int) st.wDay);
}

int CRhsDate::Month(void)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  return((int) st.wMonth);
}

int CRhsDate::Year(void)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  return((int) st.wYear);
}

int CRhsDate::Hour(void)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  return((int) st.wHour);
}

int CRhsDate::Minute(void)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  return((int) st.wMinute);
}

int CRhsDate::Second(void)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  return((int) st.wSecond);
}


//**********************************************************************
// CRhsDate::DayOfWeek
// -------------------
// Return the day of the week
// This has two forms.  The simple form returns the day of the week as
// an integer.  The other form returns the day name as a string.
//  Style 0: "Monday", "Tuesday" etc
//  Style 1: "1st", "2nd", "3rd", "4th" etc
//**********************************************************************

int CRhsDate::DayOfWeek(void)
{ SYSTEMTIME st;

  DoubleToDateTime(m_Date, &st);
  return((int) st.wDayOfWeek);
}


WCHAR* CRhsDate::DayOfWeek(WCHAR* s, int MaxLen, int Style)
{ SYSTEMTIME st;
  WCHAR format[8];

  if (!s || MaxLen < 3)
    return(NULL);

  DoubleToDateTime(m_Date, &st);

  switch (Style)
  { case 1:
      lstrcpy(format, L"dd");
      break;

    default:
      lstrcpyn(format, L"dddd", MaxLen);
      break;
  }

  GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, format, s, MaxLen);

  if (Style == 1 && MaxLen - lstrlen(s) >= 2)
  { if (st.wDay % 10 == 1)
      lstrcat(s, L"st");
    else if (st.wDay % 10 == 2)
      lstrcat(s, L"nd");
    else if (st.wDay % 10 == 3)
      lstrcat(s, L"rd");
    else
      lstrcat(s, L"th");
  }

  return(s);
}


//**********************************************************************
// CRhsDate::Format
// ----------------
// Format the date and time into a string.
// See the FormatDate method for description of styles available
//**********************************************************************

WCHAR* CRhsDate::Format(WCHAR* Buffer, int MaxLen, int DateStyle, int TimeStyle)
{ int i;

// Format the date

  FormatDate(Buffer, MaxLen, DateStyle);
  if (lstrlen(Buffer) == 0)
    return(Buffer);

// If there is space left in the buffer append the time

  i = lstrlen(Buffer);

  if (i+1 < MaxLen)
  { Buffer[i++] = ' ';
    Buffer[i] = '\0';
    FormatTime(Buffer + i, MaxLen - i, TimeStyle);
  }

// Return the formatted date and time

  return(Buffer);
}


//**********************************************************************
// CRhsDate::FormatDate
// --------------------
// Format the date into a string
// Style: 0 - "Monday 1st January 1999"
//        1 - "1st January 1999"
//        2 - "Mon 1 Jan 99"
//        3 - "1 Jan 99"
//        4 - "01/01/99"
//        5 - "1999-01-01" i.e. ANSI SQL
//**********************************************************************

WCHAR* CRhsDate::FormatDate(WCHAR* Buffer, int MaxLen, int Style)
{ SYSTEMTIME st;
  WCHAR format[80];

// Test date is valid

  if (!Buffer)
    return(NULL);

  lstrcpy(Buffer, L"");

  if (MaxLen == 0)
    return(Buffer);

  if (!IsValid())
    return(Buffer);

// Ok, format date according to style

  GetDate(&st);

  switch (Style)
  {

// ANSI SQL format

    case 5:
      lstrcpy(format, L"yyyy-MM-dd");
      break;

// Very short

    case 4:
      GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, format, 79);
      break;

// Shortest format with month name

    case 3:
      lstrcpy(format, L"dd MMM yy");
      break;

// Shortest format with month and day name

    case 2:
      lstrcpy(format, L"ddd dd MMM yy");
      break;

// Long format without day

    case 1:
      switch (st.wDay)
      { case 1:
        case 21:
        case 31:
          lstrcpy(format, L"dd'st' MMMM yyyy");
          break;

        case 2:
        case 22:
          lstrcpy(format, L"dd'nd' MMMM yyyy");
          break;

        case 3:
        case 23:
          lstrcpy(format, L"dd'rd' MMMM yyyy");
          break;

        default:
          lstrcpy(format, L"dd'th' MMMM yyyy");
          break;
      }
      break;

// Full details

    default:
      switch (st.wDay)
      { case 1:
        case 21:
        case 31:
          lstrcpy(format, L"dddd dd'st' MMMM yyyy");
          break;

        case 2:
        case 22:
          lstrcpy(format, L"dddd dd'nd' MMMM yyyy");
          break;

        case 3:
        case 23:
          lstrcpy(format, L"dddd dd'rd' MMMM yyyy");
          break;

        default:
          lstrcpy(format, L"dddd dd'th' MMMM yyyy");
          break;
      }
      break;

// End of style switch statement

  }

// Format the date

  GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, format, Buffer, MaxLen);

// All done

  return(Buffer);
}


//**********************************************************************
// CRhsDate::FormatTime
// --------------------
// Format the time into a string
// Style: 0 - 00:00
//        1 - 00:00:00
//**********************************************************************

WCHAR* CRhsDate::FormatTime(WCHAR* Buffer, int MaxLen, int Style)
{ SYSTEMTIME st;
  WCHAR time[64];

// Test date is valid

  if (!Buffer)
    return(NULL);

  lstrcpy(Buffer, L"");

  if (MaxLen == 0)
    return(Buffer);

  if (!IsValid())
    return(Buffer);

// Ok, format time according to style

  GetDate(&st);

  if (Style == 0)
    wsprintf(time, L"%02i:%02i:%02i", st.wHour, st.wMinute, st.wSecond);
  else
    wsprintf(time, L"%02i:%02i", st.wHour, st.wMinute);

  lstrcpyn(Buffer, time, MaxLen);

// All done

  return(Buffer);
}


//**********************************************************************
// Various utility functions to do date conversions
//**********************************************************************

double CRhsDate::DateTimeToDouble(int Year, int Month, int Day, int Hour, int Minute, int Second)
{ int i;
  double d;

  d = ((Year - 1900)/4)*1461;

  if (Year % 4 > 0)
    d += (Year % 4)*365 + 1;

  for (i = 1; i < Month; i++)
    d += DaysPerMonth(i, Year);

  d += Day;

  d += (Hour + (Minute + Second/60.0)/60.0)/24.0;

  return(d);
}


//**********************************************************************
// CRhsDate::DoubleToDateTime
// ---------------------------
// Convert a date in double format to SYSTEMTIME format
// Invalid dates are left as 1st Jan, 1900, 00:00:00
// 1st Jan 1900 = 1: a leap year
// 1st Jan 1901 = 367
//**********************************************************************

void CRhsDate::DoubleToDateTime(double Date, SYSTEMTIME* st)
{ int leaps;
  double d;

// Initialise the date to midnight, 1st Jan 1900

  st->wDay = st->wMonth = 1;
  st->wYear = 1900;
  st->wHour = st->wMinute = st->wSecond = 0;

  if (Date < 0)
    return;

// Working out the year is a bit of a faff because of those tricky leap years
// Each four year cycle is leap + 3*normal e.g. 1900,1901,1902,1903

  d = Date + 0.5/(24*60*60); // Add 0.5 second to avoid rounding errors
  leaps = (int) ((d-1)/1461);
  d -= leaps*1461;

// The next year after the last cycle is a leap year hence the test for
// days remaining > 366.  The remaining years are normal i.e. 365 days.

  st->wYear = leaps*4 + 1900;
  if ((int) d > 366)
  { st->wYear++;
    d -= 366;

    while ((int) d > 365)
    { st->wYear++;
      d -= 365;
    }
  }

// Phew!  Got the year, the rest is easy.

  for (st->wMonth = 1; st->wMonth <= 12; st->wMonth++)
  { if ((int) d <= DaysPerMonth(st->wMonth, st->wYear))
      break;
    d -= DaysPerMonth(st->wMonth, st->wYear);
  }

  st->wDay = (WORD) d;
  d -= st->wDay;

  d *= 24;
  st->wHour = (WORD) d;
  d -= st->wHour;

  d *= 60;
  st->wMinute = (WORD) d;
  d -= st->wMinute;

  d *= 60;
  st->wSecond = (WORD) d;

  st->wMilliseconds = 0;

  st->wDayOfWeek = ((int) Date + 6) % 7;
}


int CRhsDate::DaysPerMonth(int Month, int Year)
{ static int days_per_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (Month < 1 || Month > 12)
  { return(0);
  }
  else if (Month == 2 && Year % 4 == 0)
  { return(29);
  }
  else
  { return(days_per_month[Month-1]);
  }
}


//**********************************************************************
// The CRhsDateInterval class is little more than a data structure
//**********************************************************************

CRhsDateInterval::CRhsDateInterval()
{
  m_Type = (WORD) CRHSDATEINTERVAL_INVALID;
  m_Count = (WORD) CRHSDATEINTERVAL_INVALID;
}


CRhsDateInterval::CRhsDateInterval(int Type, int Count)
{
  if (Type > 0 && Type <= CRHSDATEINTERVAL_MAXTYPE && Count >= 0)
  { m_Type = (WORD) Type;
    m_Count = (WORD) Count;
  }
  else
  { m_Type = m_Count = (WORD) CRHSDATEINTERVAL_INVALID;
  }
}


CRhsDateInterval::~CRhsDateInterval()
{
}


//**********************************************************************
// Set the interval
//**********************************************************************

BOOL CRhsDateInterval::Set(int Type, int Count)
{
  if (Type > 0 && Type < 8 && Count > 0)
  { m_Type = (WORD) Type;
    m_Count = (WORD) Count;
    return(TRUE);
  }
  else
  { m_Type = m_Count = 0;
    return(FALSE);
  }
}

BOOL CRhsDateInterval::SetType(int Type)
{
  if (Type > 0 && Type < 8)
  { m_Type = (WORD) Type;
    return(TRUE);
  }
  else
  { m_Type = 0;
    return(FALSE);
  }
}


BOOL CRhsDateInterval::SetCount(int Count)
{
  if (Count > 0)
    m_Count = (WORD) Count;
  else
    m_Count = 0;

  return(TRUE);
}


//**********************************************************************
// Get the interval
//**********************************************************************

BOOL CRhsDateInterval::Get(int* Type, int* Count)
{
  if (IsValid())
  { *Type = m_Type;
    *Count = m_Count;
    return(TRUE);
  }
  else
  { return(FALSE);
  }
}


//**********************************************************************
// Attempt to simplify the interval
// Days cannot be simplified to anything other than weeks because the
// number of days in a month and year is dependant on the absolute date.
//**********************************************************************

BOOL CRhsDateInterval::Simplify(void)
{ BOOL simplified = FALSE;

  if (!IsValid())
    return(FALSE);

  for (;;)
  { switch (m_Type)
    { case CRHSDATEINTERVAL_SECOND:
        if (m_Count % 60 == 0)
        { m_Type = CRHSDATEINTERVAL_MINUTE;
          m_Count /= 60;
          simplified = TRUE;
        }
        else
        { return(simplified);
        }
        break;

      case CRHSDATEINTERVAL_MINUTE:
        if (m_Count % 60 == 0)
        { m_Type = CRHSDATEINTERVAL_HOUR;
          m_Count /= 60;
          simplified = TRUE;
        }
        else
        { return(simplified);
        }
        break;

      case CRHSDATEINTERVAL_HOUR:
        if (m_Count % 24 == 0)
        { m_Type = CRHSDATEINTERVAL_DAY;
          m_Count /= 24;
          simplified = TRUE;
        }
        else
        { return(simplified);
        }
        break;

      case CRHSDATEINTERVAL_DAY:
        if (m_Count % 7 == 0)
        { m_Type = CRHSDATEINTERVAL_WEEK;
          m_Count /= 7;
          simplified = TRUE;
        }
        else
        { return(simplified);
        }
        break;

      case CRHSDATEINTERVAL_MONTH:
        if (m_Count % 12 == 0)
        { m_Type = CRHSDATEINTERVAL_YEAR;
          m_Count /= 12;
          simplified = TRUE;
        }
        else
        { return(simplified);
        }
        break;

      default:
        return(simplified);
    }
  }

  return(FALSE);
}


//**********************************************************************
// Format the interval as a string
//**********************************************************************

WCHAR* CRhsDateInterval::Format(WCHAR* Buffer, int MaxLen)
{ WCHAR s[256];

  lstrcpy(Buffer, L"");

  if (!IsValid())
    return(Buffer);

  switch (m_Type)
  { case CRHSDATEINTERVAL_SECOND:
      wsprintf(s, L"%i second", m_Count);
      break;

    case CRHSDATEINTERVAL_MINUTE:
      wsprintf(s, L"%i minute", m_Count);
      break;

    case CRHSDATEINTERVAL_HOUR:
      wsprintf(s, L"%i hour", m_Count);
      break;

    case CRHSDATEINTERVAL_DAY:
      wsprintf(s, L"%i day", m_Count);
      break;

    case CRHSDATEINTERVAL_WEEK:
      wsprintf(s, L"%i week", m_Count);
      break;

    case CRHSDATEINTERVAL_MONTH:
      wsprintf(s, L"%i month", m_Count);
      break;

    case CRHSDATEINTERVAL_YEAR:
      wsprintf(s, L"%i year", m_Count);
      break;
  }

  if (m_Count > 1)
    lstrcat(s, L"s");

  lstrcpyn(Buffer, s, MaxLen);
  Buffer[MaxLen] = '\0';

  return(Buffer);
}


//**********************************************************************
// Convert string to int
//**********************************************************************

static BOOL strtoint(WCHAR* s, int* d)
{ WCHAR* start;
  WCHAR* end;

  for (start = s; *start == ' ' || *start == '\t'; start++);
  *d = (WORD) wcstol(start, &end, 10);

  return(*end == '\0' ? TRUE : FALSE);
}


