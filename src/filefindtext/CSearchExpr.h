// *********************************************************************
// CSearchExpr.h
// =============
// *********************************************************************

#ifndef _INC_CSEARCHEXPR
#define _INC_CSEARCHEXPR


//**********************************************************************
// CSearchExpr
// -----------
// Class to hold a search expression
//**********************************************************************

#define MAX_SEARCHTEXT 8
#define LEN_SEARCHTEXT 256

class CSearchExpr
{
  public:
    CSearchExpr();

    BOOL ParseExpr(const char* SearchExpr);

    BOOL FindText(const char* Input, BOOL IgnoreCase);

  public:
    int  num_expr;
    char text[MAX_SEARCHTEXT][LEN_SEARCHTEXT];

};


//**********************************************************************
// End of CSearchExpr
// ------------------
//**********************************************************************

#endif // _INC_CSEARCHEXPR

