//**********************************************************************
// Utils.h
// =======
// Collection of useful functions, mostly to do with manipulating files.
//
// John Rennie
// john.rennie@ratsauce.co.uk
// 12/05/09
//**********************************************************************

#ifndef _INC_CRHSUTILS
#define _INC_CRHSUTILS


// *********************************************************************
// Prototypes
// ----------
// *********************************************************************

class CRhsFindFile;

BOOL UtilsStripTrailingSlash(WCHAR* Filename);
BOOL UtilsStripTrailingCRLF(char* String);
BOOL UtilsAddCurPath(WCHAR* Filename, int BufLen);
BOOL UtilsIsValidPath(const WCHAR* Filename);
BOOL UtilsGetPathFromFilename(const WCHAR* Filename, WCHAR* Path, int BufLen);
BOOL UtilsGetNameFromFilename(const WCHAR* Filename, WCHAR* Name, int BufLen);
BOOL UtilsFileInfoFormat(CRhsFindFile* FFInfo, BOOL Full, WCHAR* Result, int BufLen);

const WCHAR* GetLastErrorMessage(void);


//**********************************************************************
// Global variables
// ----------------
//**********************************************************************

#define LEN_FILENAME 4096


//**********************************************************************
// End of Utils.cpp
// ----------------
//**********************************************************************

#endif // _INC_CRHSUTILS
