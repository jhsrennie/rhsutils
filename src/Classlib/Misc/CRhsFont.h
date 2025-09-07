//**********************************************************************
// CRhsFont
// ========
//
// Class to encapsulate a font
//
// The main point of the class is to simply creating fonts and in
// particular loading and saving font descriptions using the registry.
//
// The class requires the supporting class CRhsRegistry.
//
// J. Rennie
// jrennie@cix.compulink.co.uk
// 11th March 1998
//**********************************************************************

#ifndef _INC_CRHSFONT
#define _INC_CRHSFONT

class CRhsRegistry;

class CRhsFont
{

// Methods

public:
  CRhsFont(void);
  CRhsFont::CRhsFont(LOGFONT* LogFont);
  CRhsFont::CRhsFont(int lfHeight, int lfWidth, int lfEscapement, int lfOrientation, int lfWeight, BYTE lfItalic, BYTE lfUnderline, BYTE lfStrikeOut, BYTE lfCharSet, BYTE lfOutPrecision, BYTE lfClipPrecision, BYTE lfQuality, BYTE lfPitchAndFamily, WCHAR* lfFaceName);
  ~CRhsFont(void);

  const HFONT CRhsFont::SetFont(LOGFONT* LogFont);
  const HFONT CRhsFont::SetFont(int lfHeight, int lfWidth, int lfEscapement, int lfOrientation, int lfWeight, BYTE lfItalic, BYTE lfUnderline, BYTE lfStrikeOut, BYTE lfCharSet, BYTE lfOutPrecision, BYTE lfClipPrecision, BYTE lfQuality, BYTE lfPitchAndFamily, WCHAR* lfFaceName);

  const HFONT ReadFont(CRhsRegistry& Ini);
  void        SaveFont(CRhsRegistry& Ini);
  const HFONT ReadFont(CRhsRegistry* Ini);
  void        SaveFont(CRhsRegistry* Ini);

  const HFONT ChooseFont(HWND hWnd, const WCHAR* Title = NULL);

  inline const HFONT HFont(void) { return m_hfont; }
  inline const HFONT hFont(void) { return m_hfont; }
  void LogFont(LOGFONT* LogFont) { *LogFont = m_lfont; }

// Variables

private:
  LOGFONT m_lfont;
  HFONT   m_hfont;
};

#endif

