//**********************************************************************
// CRegSecObject
// ================
// Manage security on files
//**********************************************************************

#ifndef _INC_CREGSECOBJECT
#define _INC_CREGSECOBJECT

#include "CSecurableObject.h"


//**********************************************************************
// CRegSecObject
//**********************************************************************

class CRegSecObject : public CSecureableObject
{
  public:
    CRegSecObject();
    CRegSecObject(WCHAR* KeyName);
    ~CRegSecObject();

    virtual BOOL SetObjectSecurity(int bInheritACL = 0);
    virtual BOOL GetObjectSecurity(void);

    const WCHAR* RightsToText(DWORD Mask, WCHAR* Text);

    BOOL SetKeyName(WCHAR* KeyName);
    const WCHAR* GetKeyName(WCHAR* KeyName, int KeyLen);

  private:
    WCHAR* m_KeyName;
};


//**********************************************************************
// End of CRegSecObject
//**********************************************************************

#endif // _INC_CREGSECOBJECT

