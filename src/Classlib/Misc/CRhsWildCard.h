//**********************************************************************
// CRhsWildCard
// ============
// Class to find files
//**********************************************************************

#ifndef _INC_CRhsWildCard
#define _INC_CRhsWildCard


//**********************************************************************
// CRhsWildCard
// ------------
//**********************************************************************

class CRhsWildCard
{
  public:
    CRhsWildCard();
    ~CRhsWildCard();

    BOOL Match(const WCHAR* SrcPat, const WCHAR* Source);
    BOOL Complete(const WCHAR* SrcPat, const WCHAR* Source, const WCHAR* DstPat, WCHAR* Destination, int BufLen);

    BOOL CompareChar(WCHAR a, WCHAR b);
    const WCHAR* FindString(const WCHAR* searchin, const WCHAR* searchexpr);

    inline const WCHAR* LastError(void) { return m_LastError; }

  private:
    BOOL m_CaseSensitive;
    WCHAR m_LastError[256];
};


//**********************************************************************
// End of CRhsWildCard
//**********************************************************************

#endif // _INC_CRhsWildCard
