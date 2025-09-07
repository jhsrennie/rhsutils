//**********************************************************************
// CRhsDate
// ========
//**********************************************************************

#ifndef _INC_CRHSDATE
#define _INC_CRHSDATE

//**********************************************************************
//**********************************************************************

#ifndef RHSDATE
typedef double RHSDATE;
#endif


//**********************************************************************
//**********************************************************************

#define CRHSDATEINTERVAL_SECOND 1
#define CRHSDATEINTERVAL_MINUTE 2
#define CRHSDATEINTERVAL_HOUR   3
#define CRHSDATEINTERVAL_DAY    4
#define CRHSDATEINTERVAL_WEEK   5
#define CRHSDATEINTERVAL_MONTH  6
#define CRHSDATEINTERVAL_YEAR   7

#define CRHSDATEINTERVAL_MAXTYPE  7
#define CRHSDATEINTERVAL_INVALID -1

class CRhsDateInterval
{

  public:
    CRhsDateInterval();
    CRhsDateInterval(int Type, int Count);
    ~CRhsDateInterval();

    BOOL Set(int Type, int Count);
    BOOL SetType(int Type);
    BOOL SetCount(int Count);

    inline void Unset(void) { m_Type = CRHSDATEINTERVAL_INVALID; m_Count = 0; }

    BOOL Get(int* Type, int* Count);
    inline int GetType(void)  { return(m_Type > 0 && m_Type <= CRHSDATEINTERVAL_MAXTYPE ? m_Type : CRHSDATEINTERVAL_INVALID); }
    inline int GetCount(void) { return(m_Type > 0 && m_Type <= CRHSDATEINTERVAL_MAXTYPE ? m_Count: CRHSDATEINTERVAL_INVALID); }

    BOOL Simplify(void);

    WCHAR* Format(WCHAR* Buffer, int MaxLen);

    inline BOOL IsValid(void) { return(m_Type > 0 && m_Type <= CRHSDATEINTERVAL_MAXTYPE && m_Count != (WORD) CRHSDATEINTERVAL_INVALID ? TRUE : FALSE); }

  private:
    WORD m_Type,
         m_Count;
};


//**********************************************************************
//**********************************************************************

class CRhsDate
{

  public:
    CRhsDate();
    ~CRhsDate();

    CRhsDate& CRhsDate::operator=(CRhsDate& Date);

    CRhsDate& operator+(CRhsDateInterval& Add);
    CRhsDate& operator+=(CRhsDateInterval& Add);
    CRhsDate& operator-(CRhsDateInterval& Subtract);
    CRhsDate& operator-=(CRhsDateInterval& Subtract);

    int operator==(CRhsDate& Test);
    int operator!=(CRhsDate& Test);
    int operator<(CRhsDate& Test);
    int operator>(CRhsDate& Test);
    int operator<=(CRhsDate& Test);
    int operator>=(CRhsDate& Test);

    void SetDate(void);
    void SetDate(double RawDate);
    BOOL SetDate(SYSTEMTIME* st);
    BOOL SetDate(int Year, int Month, int Day, int Hour, int Minute, int Second);
    BOOL SetDate(WCHAR* Date, WCHAR* Time, BOOL PM = FALSE);
    BOOL SetDate(WCHAR* Date);
    BOOL SetTime(WCHAR* Time, BOOL PM = FALSE);

    inline void Unset(void) { m_Date = -1; }

    inline double GetDate(void) { return(m_Date); }
    void GetDate(SYSTEMTIME* st);
    void GetDate(int* Year, int* Month, int* Day, int* Hour, int* Minute, int* Second, int* DayOfWeek);

    int Day(void);
    int Month(void);
    int Year(void);
    int Hour(void);
    int Minute(void);
    int Second(void);
    int DayOfWeek(void);
    WCHAR* DayOfWeek(WCHAR* s, int MaxLen, int Style = 0);

    double AddInterval(CRhsDateInterval* Interval);
    double SubtractInterval(CRhsDateInterval* Interval);

    WCHAR* Format(WCHAR* Buffer, int MaxLen, int DateStyle = 0, int TimeStyle = 0);
    WCHAR* FormatDate(WCHAR* Buffer, int MaxLen, int Style = 0);
    WCHAR* FormatTime(WCHAR* Buffer, int MaxLen, int Style = 0);

    double DateTimeToDouble(int Year, int Month, int Day, int Hour, int Minute, int Second);
    void   DoubleToDateTime(double Date, SYSTEMTIME* st);
    int    DaysPerMonth(int Month, int Year);

    inline BOOL IsValid(void) { return(m_Date >= 0 ? TRUE : FALSE); }

  private:
    double m_Date;
};


//**********************************************************************
//**********************************************************************

#endif // _INC_CRHSDATE
