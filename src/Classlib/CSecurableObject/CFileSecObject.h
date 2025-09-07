//**********************************************************************
// CFileSecObject
// ================
// Manage security on files
//**********************************************************************

#ifndef _INC_CFILESECOBJECT
#define _INC_CFILESECOBJECT

#include "CSecurableObject.h"


//**********************************************************************
// CFileSecObject
//**********************************************************************

class CFileSecObject : public CSecureableObject
{
  public:
    CFileSecObject();
    CFileSecObject(WCHAR* FileName);
    ~CFileSecObject();

    virtual BOOL SetObjectSecurity(int bInheritACL = 0);
    virtual BOOL GetObjectSecurity(void);

    const WCHAR* RightsToText(DWORD Mask, WCHAR* Text);

    BOOL SetFileName(WCHAR* FileName);
    const WCHAR* GetFileName(WCHAR* FileName, int Len);

  private:
    WCHAR* m_FileName;
};


//**********************************************************************
// End of CFileSecObject
//**********************************************************************

#endif // _INC_CFILESECOBJECT

