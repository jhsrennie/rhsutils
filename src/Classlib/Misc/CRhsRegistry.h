//**********************************************************************
// CRhsRegistry
// ============
//
// A class to simplify registry access
//
// John Rennie
// jrennie@cix.compulink.co.uk
// 4th January 1998
//**********************************************************************

#ifndef _INC_CRHSREGISTRY
#define _INC_CRHSREGISTRY

#define LEN_REGISTRYPATH 255


//**********************************************************************
// CRhsRegistry
// ------------
//**********************************************************************

class CRhsRegistry
{

// Methods

public:
  CRhsRegistry(void);
  CRhsRegistry(const WCHAR* Key, BOOL HKeyLocalMachine = FALSE);
  ~CRhsRegistry(void);

  BOOL SetKey(const WCHAR* Key, BOOL HKeyLocalMachine = FALSE);
  BOOL GetKey(WCHAR* Key, int Len, BOOL* HKeyLocalMachine = NULL);

  BOOL  WriteString(const WCHAR* Key, const WCHAR* String);
  BOOL  WriteInt(const WCHAR* Key, int Value);
  const WCHAR* GetString(const WCHAR* Key, const WCHAR* Default, int Len);
  int   GetInt(const WCHAR* Key, int Default);

// Variables

protected:
  BOOL m_HKeyLocalMachine;
  WCHAR m_Key[LEN_REGISTRYPATH+1];

};


//**********************************************************************
// End of CRhsRegistry
//**********************************************************************

#endif // _INC_CRHSREGISTRY

