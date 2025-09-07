//**********************************************************************
// CRhsFindFile
// ============
// Class to find files
//**********************************************************************

#ifndef _INC_CRHSFINDFILE
#define _INC_CRHSFINDFILE


//**********************************************************************
// CRhsFindFile
// ------------
//**********************************************************************

class CRhsFindFile
{
  public:
    CRhsFindFile();
    ~CRhsFindFile();

    BOOL First(const WCHAR* Search, WCHAR* Found, int Len);
    BOOL Next(WCHAR* Found, int Len);
    void Close(void);

    WCHAR* FullFilename(WCHAR* Name, int Len);
    WCHAR* Filename(WCHAR* Name, int Len);

    BOOL FileSize(DWORD* Low, DWORD* High);

    inline FILETIME CreationTime(void) { return(m_Data.ftCreationTime); }
    inline FILETIME LastAccessTime(void) { return(m_Data.ftLastAccessTime); }
    inline FILETIME LastWriteTime(void) { return(m_Data.ftLastWriteTime); }

    inline DWORD Attributes(void) { return(m_Handle ? m_Data.dwFileAttributes : (DWORD) -1); }

    inline DWORD FileSize(void) { return(m_Handle ? m_Data.nFileSizeLow : (DWORD) 0); }

  private:
    HANDLE m_Handle;
    WIN32_FIND_DATA m_Data;
    WCHAR* m_Path;
};


//**********************************************************************
// End of CRhsFindFile
//**********************************************************************

#endif // _INC_CRHSFINDFILE
