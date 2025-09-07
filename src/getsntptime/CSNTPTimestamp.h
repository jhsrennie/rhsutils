//**********************************************************************
// class SNTPTimestamp
// -------------------
// This class encapsulates an NTP timestamp and provides methods to
// manipulate it.
//**********************************************************************

#ifndef _INC_CSNTPTIMESTAMP
#define _INC_CSNTPTIMESTAMP

class CSNTPTimestamp
{
  public:
    inline CSNTPTimestamp() { seconds = fraction = 0; }

    BOOL SetSystemTime(SYSTEMTIME* st);
    void GetSystemTime(SYSTEMTIME* st);

    DWORD DaysPerMonth(int Month, int Year);
    DWORD SwapBytes(DWORD d);

    DWORD seconds,
          fraction;
};

#endif // _INC_CSNTPTIMESTAMP

