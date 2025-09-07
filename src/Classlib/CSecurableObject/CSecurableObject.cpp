//**********************************************************************
// CSecurableObject
// ================
// Root class for the sucrity classes.
// This class provides methods to modify ACLs. Subclasses need only
// provide methods specific to their object type.
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <stdio.h>
#include "CSecurableObject.h"


//**********************************************************************
// CSecureableObject
// =================
// Constructor
// This does a lot of work.  It creates the internal security descriptor
// then it creates a DACL and SACL and adds them to the descriptor.  If
// the object is protected the ACLs have WORLD:No access, if they are
// unprotected it adds WORLD:All access.
//**********************************************************************

CSecureableObject::CSecureableObject(BOOL bProtected)
{ DWORD dwDACLLength;
  PSID pcSid = NULL;
  SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
  int iTempSidLength;

// Initialise internal variables

  m_pSD = NULL;
  m_pDACL = NULL;
  m_pSACL = NULL;
  m_pOwner = NULL;
  m_pPrimaryGroup = NULL;

// Allocate and initialise the security descriptor

  m_pSD = malloc(sizeof(SECURITY_DESCRIPTOR));
  if (!m_pSD)
  { m_iSecErrorCode = GetLastError();
    return;
  }

  if (!InitializeSecurityDescriptor(m_pSD, SECURITY_DESCRIPTOR_REVISION))
  { m_iSecErrorCode = GetLastError();
    return;
  }

// Allocate a SID big enough to contain one sub-authority

  iTempSidLength = GetSidLengthRequired(1); // this can not fail

  pcSid = (PSID) malloc(iTempSidLength);
  if (!pcSid)
  { m_iSecErrorCode = GetLastError();
    return;
  }

// Allocate and initialise an ACL with space for one entry

  dwDACLLength = sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD) + iTempSidLength;
  m_pDACL = (PACL) malloc(dwDACLLength);
  if (!m_pDACL)
  { m_iSecErrorCode = GetLastError();
    free(pcSid);
    return;
  }

  if (!InitializeAcl(m_pDACL, dwDACLLength, ACL_REVISION) || !InitializeSid(pcSid, &siaWorld, 1))
  { m_iSecErrorCode = GetLastError();
    free(pcSid);
    return;
  }

// Set the first (and only) subauthority in pcSid to WORLD.  If the object is
// protected add this group to the ACL with no access

  *(GetSidSubAuthority(pcSid, 0)) = SECURITY_WORLD_RID;

  if (bProtected)
  { if (!AddAccessAllowedAce(m_pDACL, ACL_REVISION, NULL, pcSid))  // RAc
    { m_iSecErrorCode = GetLastError();
      free(pcSid);
      return;
    }
  }

// If the object is unprotected add WORLD to the ACL with full access

  else
  { if (!AddAccessAllowedAce(m_pDACL, ACL_REVISION, GENERIC_ALL|STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL, pcSid))
    { m_iSecErrorCode = GetLastError();
      free(pcSid);
      return;
    }
  }

// Now add the ACL to our security descriptor as the DACL

  if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pcSid);
    return;
  }

// It's a bit on the tedious side, but we need to go through the same procedure
// to build the SACL

  if (pcSid)
    free (pcSid);
  pcSid = NULL;

  iTempSidLength = GetSidLengthRequired(1);
  pcSid = (PSID)malloc(iTempSidLength);
  if (!pcSid)
  { m_iSecErrorCode = GetLastError();
    return;
  }

  dwDACLLength = sizeof (ACL) + sizeof (SYSTEM_AUDIT_ACE) - sizeof (DWORD) + iTempSidLength;
  m_pSACL = (PACL) malloc(dwDACLLength);
  if (!m_pSACL)
  { m_iSecErrorCode = GetLastError();
    free(pcSid);
    return;
  }

  if (!InitializeAcl(m_pSACL, dwDACLLength, ACL_REVISION) || !InitializeSid(pcSid, &siaWorld, 1))
  { m_iSecErrorCode = GetLastError();
    free(pcSid);
    return;
  }

  *(GetSidSubAuthority(pcSid, 0)) = SECURITY_WORLD_RID;

  if (!AddAuditAccessAce(m_pSACL, ACL_REVISION, GENERIC_ALL|STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL, pcSid, TRUE, TRUE))
  { m_iSecErrorCode = GetLastError();
    free(pcSid);
    return;
  }

  if (!SetSecurityDescriptorSacl(m_pSD, TRUE, m_pSACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pcSid);
    return;
  }

// All finished

  if (pcSid)
   free (pcSid);
}


//**********************************************************************
// CSecureableObject
// =================
// Destructor
// All the destructor has to do is free up the data structures we
// allocated.
//**********************************************************************

CSecureableObject::~CSecureableObject()
{
  FreeDataStructures();
}


//**********************************************************************
// CSecureableObject::SetRightsTo
// ==============================
// Set access rights to just the supplied account
//**********************************************************************

BOOL CSecureableObject::SetRightsTo(const WCHAR* pAccountName, const WCHAR* pDomainName, DWORD dwAccessMask, BOOL bGranted, BOOL bInheritACL)
{ DWORD dwDACLLength;
  WCHAR *pcDomainName;
  BYTE *pcSid;
  PACL pNewACL=NULL;
  PSECURITY_DESCRIPTOR pNewSD=NULL;

// First get the SID for the username we are adding rights for

  if (!GetSIDFromName(pDomainName, pAccountName, &pcSid, &pcDomainName))
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

// Allocate and initialise a new security descriptor and DACL

  dwDACLLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pcSid) - sizeof(DWORD);

  pNewSD = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH + dwDACLLength);
  if (!pNewSD)
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

  if (!InitializeSecurityDescriptor(pNewSD, SECURITY_DESCRIPTOR_REVISION))
  { m_iSecErrorCode = GetLastError();
    free(pNewSD);
    return FALSE;
  }

  pNewACL = (PACL) malloc(dwDACLLength);
  if (!pNewACL)
  { m_iSecErrorCode = GetLastError();
    free(pNewSD);
    return FALSE;
  }

  if (!InitializeAcl(pNewACL, dwDACLLength, ACL_REVISION))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

// Add either an access denied ACE

  if (!bGranted)
  { if (!AddAccessDeniedAce(pNewACL, ACL_REVISION, dwAccessMask, (PSID)pcSid))
    { m_iSecErrorCode = GetLastError();
      free(pNewACL);
      free(pNewSD);
      return FALSE;
    }
  }

// Or an access allowed ACE

  else
  { if (!AddAccessAllowedAce(pNewACL, ACL_REVISION, dwAccessMask, (PSID)pcSid))
    { m_iSecErrorCode = GetLastError();
      free(pNewACL);
      free(pNewSD);
      return FALSE;
    }
  }

// Now check everything is OK and set the new ACL

  if (!IsValidAcl(pNewACL))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!IsValidSecurityDescriptor(pNewSD))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!SetSecurityDescriptorDacl(pNewSD, TRUE, pNewACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

// Copy our laboriously built DACL into the SD and save the new SD.

  if (m_pOwner && !SetSecurityDescriptorOwner(pNewSD, m_pOwner, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (m_pSACL && !SetSecurityDescriptorSacl(pNewSD, TRUE, m_pSACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (m_pPrimaryGroup && !SetSecurityDescriptorGroup(pNewSD, m_pPrimaryGroup, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  free(m_pSD);
  m_pSD = pNewSD;

  free(m_pDACL);
  m_pDACL=pNewACL;

// The final job is to set the descriptor.  As with GetObjectSecurity the subclass
// will handle this bit.
// If this fails the error code has already been set, so don't set it.

  if (!SetObjectSecurity(bInheritACL))
  { free(m_pDACL);
    free(m_pSD);
    return FALSE;
  }

// All done

  return TRUE;
}


//**********************************************************************
// CSecureableObject::AddRightsTo
// ==============================
// Add access rights to our SD.
//**********************************************************************

BOOL CSecureableObject::AddRightsTo(const WCHAR* pAccountName, const WCHAR* pDomainName, DWORD dwAccessMask, BOOL bGranted)
{ DWORD dwDACLLength;
  WCHAR *pcDomainName;
  BYTE *pcSid;
  BOOL bHasDacl, bHasDefaulted;
  PACL pCurrentAcl=NULL;
  PACL pNewACL=NULL;
  PSECURITY_DESCRIPTOR pNewSD=NULL;
  UINT dwNumberOfAces;
  DWORD dwAceSize;
  DWORD dwCurrentSecurityDescriptorLength;
  UINT iLoop;
  void *pTempACL;

// First get the SID for the username we are adding rights for

  if (!GetSIDFromName(pDomainName, pAccountName, &pcSid, &pcDomainName))
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

// Now call the pure virtual function GetObjectSecurity to build our security
// descriptor, and then retrieve the current DACL.  Subclasses have to provide
// this function.
// If this fails the error code has already been set, so don't set it.

  if (!GetObjectSecurity())
  { return FALSE;
  }

  if (!GetSecurityDescriptorDacl(m_pSD, &bHasDacl, (PACL *)&pCurrentAcl, &bHasDefaulted))
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

// Allocate a new DACL with size equal to the current DACL plus room for another
// access control entry.  (We assume that pcSid is valid to get its length).
// Note that the DACL can be NULL.

  if (pCurrentAcl)
  { dwNumberOfAces = pCurrentAcl->AceCount;
    dwAceSize = pCurrentAcl->AclSize;
  }
  else
  { dwNumberOfAces = 0;
    dwAceSize = sizeof(ACL);
  }

  dwDACLLength = dwAceSize + sizeof(ACCESS_ALLOWED_ACE) - sizeof (DWORD) + GetLengthSid(pcSid);
  pNewACL = (PACL) malloc(dwDACLLength);
  if (!pNewACL)
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

// Allocate and initialise a new security descriptor with room for the new DACL

  dwCurrentSecurityDescriptorLength = GetSecurityDescriptorLength(m_pSD);
  pNewSD = malloc(dwDACLLength + dwCurrentSecurityDescriptorLength);
  if (!pNewSD)
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    return FALSE;
  }

  if (!InitializeSecurityDescriptor(pNewSD, SECURITY_DESCRIPTOR_REVISION))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!InitializeAcl(pNewACL, dwDACLLength, ACL_REVISION))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

// Now if we are adding an access denied entry we must put the entry first
// in the new ACL

  if (!bGranted)
  { if (!AddAccessDeniedAce(pNewACL, ACL_REVISION, dwAccessMask, (PSID)pcSid))
    { m_iSecErrorCode = GetLastError();
      free(pNewACL);
      free(pNewSD);
      return FALSE;
    }
  }

// For access denied we now append the contents of the old ACL

  if (pCurrentAcl)
  { for (iLoop = 0; iLoop < dwNumberOfAces; iLoop++)
    { if (!GetAce(pCurrentAcl, iLoop, &pTempACL))
      { m_iSecErrorCode = GetLastError();
        free(pNewACL);
        free(pNewSD);
        return FALSE;
      }

      if (!AddAce(pNewACL, ACL_REVISION, MAXDWORD, pTempACL, ((PACE_HEADER)pTempACL)->AceSize))
      { m_iSecErrorCode = GetLastError();
        free(pNewACL);
        free(pNewSD);
        return FALSE;
      }
    }
  }

// For access granted we add the new ACE to the end of the ACL

  if (bGranted)
  { if (!AddAccessAllowedAce(pNewACL, ACL_REVISION, dwAccessMask, (PSID)pcSid))
    { m_iSecErrorCode = GetLastError();
      free(pNewACL);
      free(pNewSD);
      return FALSE;
    }
  }

// Now check everything is OK and set the new ACL

  if (!IsValidAcl(pNewACL))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!IsValidSecurityDescriptor(pNewSD))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!SetSecurityDescriptorDacl(pNewSD, TRUE, pNewACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

// Copy our laboriously built DACL into the SD and save the new SD

  if (m_pOwner && !SetSecurityDescriptorOwner(pNewSD, m_pOwner, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (m_pSACL && !SetSecurityDescriptorSacl(pNewSD, TRUE, m_pSACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (m_pPrimaryGroup && !SetSecurityDescriptorGroup(pNewSD, m_pPrimaryGroup, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  free(m_pSD);
  m_pSD = pNewSD;

  free(m_pDACL);
  m_pDACL=pNewACL;

// The final job is to set the descriptor.  As with GetObjectSecurity the subclass
// will handle this bit.
// If this fails the error code has already been set, so don't set it.

  if (!SetObjectSecurity(0))
  { free(m_pDACL);
    free(m_pSD);
    return FALSE;
  }

// All done

  return TRUE;
}


//**********************************************************************
// CSecureableObject::RevokeRightsFor
// ==================================
// Remove an access control entry from the security descriptor
//**********************************************************************

BOOL CSecureableObject::RevokeRightsFor(const WCHAR* pAccountName, const WCHAR* pDomainName)
{
  PACL pNewACL = NULL;
  PACL pCurrentAcl=NULL;
  DWORD dwDACLLength;
  WCHAR *pcDomainName;
  BYTE *pcSid;
  PSECURITY_DESCRIPTOR pNewSD = NULL;
  UINT iOffendingIndex;
  BOOL bHasDacl;
  BOOL bHasDefaulted;
  UINT dwNumberOfAces;
  DWORD dwAceSize;

// First get the SID for the username

  if (!GetSIDFromName(pDomainName, pAccountName, &pcSid, &pcDomainName))
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

// Now call the pure virtual function GetObjectSecurity to build our security
// descriptor, and then retrieve the current DACL.  Subclasses have to provide
// this function.
// If this fails the error code has already been set, so don't set it.

  if (!GetObjectSecurity())
  { return FALSE;
  }

  if (!GetSecurityDescriptorDacl(m_pSD, &bHasDacl, (PACL *)&pCurrentAcl, &bHasDefaulted))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!pCurrentAcl)
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

// Now we search through the DACL looking for our SID.  Search backwards
// so we find the most recently added ACE.

  dwNumberOfAces = pCurrentAcl->AceCount;
  dwAceSize = pCurrentAcl->AclSize;
  DWORD dwCurrentSecurityDescriptorLength;
  dwCurrentSecurityDescriptorLength=GetSecurityDescriptorLength(m_pSD);
  UINT iLoop;
  void *pTempACL;

//  for (iLoop = 0; iLoop < dwNumberOfAces; iLoop++)
  for (iLoop = dwNumberOfAces - 1; iLoop >= 0; iLoop--)
  { if (! GetAce(pCurrentAcl, iLoop, &pTempACL))
    { m_iSecErrorCode = GetLastError();
      return FALSE;
    }

   if (EqualSid ((PSID) &(((PACCESS_ALLOWED_ACE)pTempACL)->SidStart), pcSid))
     break;
  }

// If we didn't find the SID quit with a privilege not held error

  if (iLoop < 0)
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

  iOffendingIndex = iLoop;

// Allocate and initialise a new ACL

  dwDACLLength = dwAceSize - ((PACE_HEADER) pTempACL)->AceSize;

  pNewACL = (PACL) malloc(dwDACLLength);
  if (!pNewACL)
  { m_iSecErrorCode = GetLastError();
    return FALSE;
  }

  pNewSD = malloc(dwDACLLength+dwCurrentSecurityDescriptorLength);
  if (!pNewSD)
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    return FALSE;
  }

  if (!InitializeSecurityDescriptor(pNewSD, SECURITY_DESCRIPTOR_REVISION))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!InitializeAcl(pNewACL, dwDACLLength, ACL_REVISION))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

// Now we copy the old ace to the new one leaving out the offending SID

  for (iLoop = 0; iLoop < dwNumberOfAces; iLoop++)
  { if (iLoop == iOffendingIndex)
      continue;

    if (!GetAce(pCurrentAcl, iLoop, &pTempACL))
    { m_iSecErrorCode = GetLastError();
      free(pNewACL);
      free(pNewSD);
      return FALSE;
    }

    if (!AddAce(pNewACL, ACL_REVISION, MAXDWORD, pTempACL, ((PACE_HEADER)pTempACL)->AceSize))
    { m_iSecErrorCode = GetLastError();
      free(pNewACL);
      free(pNewSD);
      return FALSE;
    }
  }

// Put the new ACL into the new SD

  if (!IsValidAcl(pNewACL))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!IsValidSecurityDescriptor(pNewSD))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (!SetSecurityDescriptorDacl(pNewSD, TRUE, pNewACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

// now copy all of the other relevant stuff to the new SD

  if (m_pOwner && !SetSecurityDescriptorOwner(pNewSD, m_pOwner, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (m_pSACL && !SetSecurityDescriptorSacl(pNewSD, TRUE, m_pSACL, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  if (m_pPrimaryGroup && !SetSecurityDescriptorGroup(pNewSD, m_pPrimaryGroup, FALSE))
  { m_iSecErrorCode = GetLastError();
    free(pNewACL);
    free(pNewSD);
    return FALSE;
  }

  free(m_pSD);
  m_pSD = pNewSD;

  free(m_pDACL);
  m_pDACL=pNewACL;

// Finally set the new descriptor
// If this fails the error code has already been set, so don't set it.

  if (!SetObjectSecurity())
  { free(m_pDACL);
    free(m_pSD);
    return FALSE;
  }

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CSecureableObject::GetNumEntries
// --------------------------------
// Return the number of entries in the ACL
//**********************************************************************

int CSecureableObject::GetNumEntries(void)
{
  PACL pCurrentAcl=NULL;
  BOOL bHasDacl;
  BOOL bHasDefaulted;

// We need the DACL to get the number of entries

  if (!GetSecurityDescriptorDacl(m_pSD, &bHasDacl, (PACL *)&pCurrentAcl, &bHasDefaulted))
    return 0;

// The SD may not contain an ACL

  if (!bHasDacl || !pCurrentAcl)
    return 0;

// Return the number of entries

  return (int) (pCurrentAcl->AceCount);
}


//**********************************************************************
// CSecureableObject::GetRightsFromACE
// -----------------------------------
// Return the username, access mask and ACE type for the specified index
// The pAccountName and pDomainName buffers must be at least 64 bytes as
// GetNameFromSID assumes the buffers are at least 64 bytes long.
//**********************************************************************

BOOL CSecureableObject::GetRightsFromACE(WCHAR* pAccountName, WCHAR* pDomainName, DWORD* dwAccessMask, DWORD* dwType, DWORD* dwFlags, DWORD dwIndex)
{
  PACL pAcl=NULL;
  BYTE* pAce = NULL;
  BYTE* pcSid;
  BOOL bHasDacl;
  BOOL bHasDefaulted;

// We need the DACL to get the number of entries

  if (!GetSecurityDescriptorDacl(m_pSD, &bHasDacl, (PACL*)&pAcl, &bHasDefaulted))
    return(FALSE);

// The SD may not contain an ACL

  if (!bHasDacl || !pAcl)
    return FALSE;

// Check the ACE exists

  if (dwIndex >= pAcl->AceCount)
    return FALSE;

// Get a pointer to the ACE structure

  if (!GetAce(pAcl, dwIndex, (void**) &pAce))
    return FALSE;

// Get the type and flags

  *dwType = ((ACE_HEADER*) pAce)->AceType;
  *dwFlags = ((ACE_HEADER*) pAce)->AceFlags;

// Finally get the access mask

  if (*dwType == 0)
    *dwAccessMask = ((ACCESS_DENIED_ACE*) pAce)->Mask;
  else
    *dwAccessMask = ((ACCESS_ALLOWED_ACE*) pAce)->Mask;

// Get the SID and convert it to a trustee name

  pcSid = (BYTE*) &(((ACCESS_ALLOWED_ACE*) pAce)->SidStart);
  GetNameFromSID(pDomainName, pAccountName, pcSid);

// And return indicating success

  return TRUE;
}


//**********************************************************************
// CSecureableObject::GetAllRightsFor
// ----------------------------------
// This function looks up the SID for the specified index then it goes
// throught the ACL finding all entries for that SID.
// Note that the function will only find ACEs matching the current type,
// i.e. all the access allowed or all the access denied ACEs.
//**********************************************************************

BOOL CSecureableObject::GetAllRightsFor(DWORD* Mask, DWORD dwIndex)
{ DWORD type, numentries, i;
  PACL pAcl=NULL;
  BYTE* pAce = NULL;
  BYTE* pTargetSid;
  BYTE* pSid;
  BOOL bHasDacl;
  BOOL bHasDefaulted;

// We need the DACL to get the number of entries

  if (!GetSecurityDescriptorDacl(m_pSD, &bHasDacl, (PACL*)&pAcl, &bHasDefaulted))
    return(FALSE);

// The SD may not contain an ACL

  if (!bHasDacl || !pAcl)
    return FALSE;

// Check the ACE exists

  numentries = pAcl->AceCount;

  if (dwIndex >= pAcl->AceCount)
    return FALSE;

// Get the Target SID

  if (!GetAce(pAcl, dwIndex, (void**) &pAce))
    return FALSE;

  pTargetSid = (BYTE*) &(((ACCESS_ALLOWED_ACE*) pAce)->SidStart);
  type = ((ACE_HEADER*) pAce)->AceType;

// Now roam the ACL looking for all occurrences of this SID

  *Mask = 0;

  for (i = 0; i < numentries; i++)
  { if (!GetAce(pAcl, i, (void**) &pAce))
      continue;

// If the type doesn't match go on to the next ACE
    if (type != ((ACE_HEADER*) pAce)->AceType)
      continue;

// If the SID doesn't match go on to the next ACE
    pSid = (BYTE*) &(((ACCESS_ALLOWED_ACE*) pAce)->SidStart);
    if (!EqualSid(pTargetSid, pSid))
      continue;

    if (type == 0) // 0 is access denied
      *Mask |= ((ACCESS_DENIED_ACE*) pAce)->Mask;
    else
      *Mask |= ((ACCESS_ALLOWED_ACE*) pAce)->Mask;
  }

  return TRUE;
}


//**********************************************************************
// CSecureableObject::GetSIDFromName
// =================================
// Look up the SID and details for a username
// If the function returns TRUE the calling app is responsible for
// freeing pcSid and pcDomainName.
//**********************************************************************

BOOL CSecureableObject::GetSIDFromName(const WCHAR* pDomainName, const WCHAR* pAccountName, BYTE **pcSid, WCHAR **pcDomainName)
{ SID_NAME_USE su;
  DWORD dwSidLength=0;
  unsigned long iBufLen=0;

  *pcSid = NULL;
  *pcDomainName = NULL;

// We first must determine the SID that belongs to the user. This happens in
// two steps: First, determine how much space is needed for the SID and
// domain buffers. Then, call the function again with the previously allocated
// buffers.

   if (!(LookupAccountName(pDomainName, pAccountName, *pcSid, &dwSidLength, *pcDomainName, &iBufLen, &su)))
   {

// If the call didn't fail with error ERROR_INSUFFICIENT_BUFFER something
// went wrong

     if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
       goto ErrorExit;

// Allocate the buffers; NB the calling app must free these

     *pcSid = (BYTE*) malloc(dwSidLength);
     if (!*pcSid)
       goto ErrorExit;

     *pcDomainName = (WCHAR*) malloc(iBufLen);
     if (!*pcDomainName)
       goto ErrorExit;

// Try the lookup again; this time a failure is fatal

     if (!LookupAccountName(pDomainName, pAccountName, *pcSid, &dwSidLength, *pcDomainName, &iBufLen, &su))
       goto ErrorExit;

// The SID should be valid, but let's check anyway

     if (!IsValidSid(*pcSid))
       goto ErrorExit;
  }

// Return indicating success

  return TRUE;

// OK, gotos are horrid think of it as a try-except structure :-)

  ErrorExit:
    m_iSecErrorCode = GetLastError();

    if (*pcSid)
      free (*pcSid);

    if (*pcDomainName)
      free (*pcDomainName);

    return(FALSE);
}


//**********************************************************************
// CSecureableObject::GetNameFromSID
// =================================
// Look up the username and domain for a SID
// The username and domain buffers are assumed to be at least 64 bytes
// long.
//**********************************************************************

BOOL CSecureableObject::GetNameFromSID(WCHAR* pDomainName, WCHAR* pAccountName, BYTE* pcSid)
{
  DWORD len_name, len_domain;
  SID_NAME_USE sidname;

  lstrcpy(pDomainName, L"Unknown");
  lstrcpy(pAccountName, L"Unknown");

  len_name = len_domain = 63;
  if (!LookupAccountSid(NULL, pcSid, pAccountName, &len_name, pDomainName, &len_domain, &sidname))
    return FALSE;

  return TRUE;
}


//**********************************************************************
// CSecureableObject::BuildSD
// ==========================
// Use the supplied SD to build a new absolute SD and save it as our
// objects SD.  This function is generally called from subclasses'
// implementation of the virtual function GetObjectSecurity.
//**********************************************************************

BOOL CSecureableObject::BuildSD(PSECURITY_DESCRIPTOR pSelfRelativeReturnSD)
{ DWORD iSDSize, iDACLSize, iSACLSize, iOwnerSize, iGroupSize;

   iSDSize=0;
   iDACLSize=0;
   iSACLSize=0;
   iOwnerSize=0;
   iGroupSize=0;

// Attempt to create an absolute SD using the self relative SD supplied.
// This will fail because the buffer sizes are all zero.  We can then allocate
// buffers of the size required and try again.

   if (!MakeAbsoluteSD(pSelfRelativeReturnSD, m_pSD, &iSDSize, m_pDACL, &iDACLSize, m_pSACL, &iSACLSize, m_pOwner, &iOwnerSize, m_pPrimaryGroup, &iGroupSize) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
   {

// Allocate all the data structures and try again...

     if (iSDSize)
     { m_pSD = malloc(iSDSize);
       if (!m_pSD)
         goto ErrorExit;
     }

     if (iDACLSize)
     { m_pDACL = (PACL)malloc(iDACLSize);
       if (!m_pDACL)
         goto ErrorExit;
     }

     if (iSACLSize)
     { m_pSACL = (PACL)malloc(iSACLSize);
       if (!m_pSACL)
         goto ErrorExit;
     }

     if (iOwnerSize)
     { m_pOwner = malloc(iOwnerSize);
       if (!m_pOwner)
         goto ErrorExit;
     }

     if (iGroupSize)
     { m_pPrimaryGroup = malloc(iGroupSize);
       if (!m_pPrimaryGroup)
         goto ErrorExit;
     }

// Try again to create the absolute SD

     if (!MakeAbsoluteSD(pSelfRelativeReturnSD, m_pSD, &iSDSize, m_pDACL, &iDACLSize, m_pSACL, &iSACLSize, m_pOwner, &iOwnerSize, m_pPrimaryGroup, &iGroupSize))
       goto ErrorExit;

// It worked, we have a new SD which we keep for later modification

     return TRUE;
   }

// We got here due to an error in the original MakeAbsoluteSD call

   FreeDataStructures();

   return FALSE;

// OK, gotos are horrid think of it as a try-except structure :-)

  ErrorExit:
    m_iSecErrorCode = GetLastError();
    FreeDataStructures();
    return(FALSE);
}


//**********************************************************************
// CSecureableObject::FreeDataStructures
// =====================================
// Free the structures we allocated to hold SDs, ACLs etc
//**********************************************************************

void CSecureableObject::FreeDataStructures(void)
{
  if (m_pSD)
    free (m_pSD);
  if (m_pDACL)
    free(m_pDACL);
  if (m_pSACL)
    free(m_pSACL);
  if (m_pOwner)
    free(m_pOwner);
  if (m_pPrimaryGroup)
    free(m_pPrimaryGroup);

  ZeroOut();
}


//**********************************************************************
// CSecureableObject::ZeroOut
// ==========================
// Initialise internal variables to default state
//**********************************************************************

void CSecureableObject::ZeroOut(void)
{
  m_pSD = NULL;
  m_pDACL = NULL;
  m_pSACL = NULL;
  m_pOwner = NULL;
  m_pPrimaryGroup = NULL;
}


//**********************************************************************
// CSecureableObject::GetLastErrorMessage
// ======================================
// Print the error message for the last error
//**********************************************************************

const WCHAR* CSecureableObject::GetLastErrorMessage(void)
{ static WCHAR errmsg[512];

  lstrcpy(errmsg, L"<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, m_iSecErrorCode, 0, errmsg, 511, NULL);
  return(errmsg);
}


//**********************************************************************
// SetSpecificPrivilegeInAccessToken
// =================================
// Enable a privilege
//**********************************************************************

BOOL SetSpecificPrivilegeInAccessToken(const WCHAR* lpPrivType, BOOL bEnabled)
{ HANDLE           hProcess;
  HANDLE           hAccessToken;
  LUID             luidPrivilegeLUID;
  TOKEN_PRIVILEGES tpTokenPrivilege;
  BOOL bReturn = FALSE;

// Get the current process details

  hProcess = GetCurrentProcess();
  if (!hProcess)
    return(FALSE);

  if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hAccessToken))
    return(FALSE);

// Look up the privilege LUID from its name

  if (!LookupPrivilegeValue(NULL, lpPrivType, &luidPrivilegeLUID))
  { CloseHandle(hAccessToken);
    return(FALSE);
  }

// Enable the privilege

  tpTokenPrivilege.PrivilegeCount = 1;
  tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
  tpTokenPrivilege.Privileges[0].Attributes = bEnabled?SE_PRIVILEGE_ENABLED:0;
  SetLastError(ERROR_SUCCESS);

  bReturn = AdjustTokenPrivileges(hAccessToken, FALSE, &tpTokenPrivilege, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
  if (GetLastError() != ERROR_SUCCESS)
    bReturn = FALSE;

  CloseHandle(hAccessToken);

// Return indicating whether successful

  return bReturn;
}


//**********************************************************************
// SetPrivilegeInAccessToken
// =========================
// Enable the SeSecurityPrivilege privilege
//**********************************************************************

BOOL SetPrivilegeInAccessToken(BOOL bEnabled)
{
  return SetSpecificPrivilegeInAccessToken(L"SeSecurityPrivilege", bEnabled);
}


