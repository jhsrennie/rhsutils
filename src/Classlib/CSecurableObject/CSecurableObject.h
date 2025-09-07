//**********************************************************************
// Security classes
//**********************************************************************

#ifndef _INC_CSECUREABLEOBJECT
#define _INC_CSECUREABLEOBJECT


//**********************************************************************
// Constants for handling ACL inheritance in 2k and XP
//**********************************************************************

#ifndef UNPROTECTED_SACL_SECURITY_INFORMATION
#define UNPROTECTED_SACL_SECURITY_INFORMATION 0x10000000
#endif

#ifndef UNPROTECTED_DACL_SECURITY_INFORMATION
#define UNPROTECTED_DACL_SECURITY_INFORMATION 0x20000000
#endif

#ifndef PROTECTED_SACL_SECURITY_INFORMATION
#define PROTECTED_SACL_SECURITY_INFORMATION 0x40000000
#endif

#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION 0x80000000
#endif


//**********************************************************************
// Helper functions
//**********************************************************************

BOOL SetSpecificPrivilegeInAccessToken(const WCHAR* lpPrivType, BOOL bEnabled);
BOOL SetPrivilegeInAccessToken(BOOL);


//**********************************************************************
// CSecureableObject
// =================
// Base class, not instantiated directly.
//**********************************************************************

class CSecureableObject
{

// Public methods

  public:
    CSecureableObject(BOOL bProtected);
    ~CSecureableObject();

    BOOL SetRightsTo(const WCHAR* pAccountName, const WCHAR* pDomainName, DWORD dwAccessMask, BOOL bGranted, BOOL bInheritACL = FALSE);
    BOOL AddRightsTo(const WCHAR* pAccountName, const WCHAR* pDomainName, DWORD dwAccessMask, BOOL bGranted);
    BOOL RevokeRightsFor(const WCHAR* pAccountName, const WCHAR* pDomainName);

    int  GetNumEntries(void);
    BOOL GetRightsFromACE(WCHAR* pAccountName, WCHAR* pDomainName, DWORD* dwAccessMask, DWORD* dwType, DWORD* dwFlags, DWORD dwIndex);
    BOOL GetAllRightsFor(DWORD* Mask, DWORD dwIndex);

    const WCHAR* GetLastErrorMessage(void);

// Public data

  public:
    int m_iSecErrorCode;

// Protected methods

  protected:
    BOOL BuildSD(PSECURITY_DESCRIPTOR pSelfRelativeReturnSD);

    void FreeDataStructures(void);
    void ZeroOut(void);

    virtual BOOL SetObjectSecurity(int bInheritACL = 0)=0;
    virtual BOOL GetObjectSecurity(void)=0;

// Protected data

  protected:
    PSECURITY_DESCRIPTOR m_pSD;
    PACL m_pDACL;
    PACL m_pSACL;
    PSID m_pOwner;
    PSID m_pPrimaryGroup;

// Private methods

  private:
    BOOL GetSIDFromName(const WCHAR* pDomainName, const WCHAR* pAccountName, BYTE **pcSid, WCHAR **pcDomainName);
    BOOL GetNameFromSID(WCHAR* pDomainName, WCHAR* pAccountName, BYTE *pcSid);

};


//**********************************************************************
// End of security classes
//**********************************************************************

#endif // _INC_CSECUREABLEOBJECT

