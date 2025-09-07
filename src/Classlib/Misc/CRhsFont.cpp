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
// john.rennie@ratsauce.co.uk
// 11th March 1998
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <commdlg.h>
#include "CRhsRegistry.h"
#include "CRhsFont.h"


//**********************************************************************
// CRhsFont::CRhsFont
// ------------------
// Create the font using the default of 10pt MS Sans Serif, a LOGFONT or
// individually supplied details.
//**********************************************************************

CRhsFont::CRhsFont(void)
{
  SetFont(-13,
          0,
          0,
          0,
          FW_NORMAL,
          FALSE,
          FALSE,
          FALSE,
          ANSI_CHARSET,
          OUT_DEFAULT_PRECIS,
          CLIP_DEFAULT_PRECIS,
          DEFAULT_QUALITY,
          VARIABLE_PITCH | FF_MODERN,
          L"MS Sans Serif"
         );
}


CRhsFont::CRhsFont(LOGFONT* LogFont)
{
  SetFont(LogFont->lfHeight,
          LogFont->lfWidth,
          LogFont->lfEscapement,
          LogFont->lfOrientation,
          LogFont->lfWeight,
          LogFont->lfItalic,
          LogFont->lfUnderline,
          LogFont->lfStrikeOut,
          LogFont->lfCharSet,
          LogFont->lfOutPrecision,
          LogFont->lfClipPrecision,
          LogFont->lfQuality,
          LogFont->lfPitchAndFamily,
          LogFont->lfFaceName
         );
}


CRhsFont::CRhsFont(int  lfHeight,
                   int  lfWidth,
                   int  lfEscapement,
                   int  lfOrientation,
                   int  lfWeight,
                   BYTE lfItalic,
                   BYTE lfUnderline,
                   BYTE lfStrikeOut,
                   BYTE lfCharSet,
                   BYTE lfOutPrecision,
                   BYTE lfClipPrecision,
                   BYTE lfQuality,
                   BYTE lfPitchAndFamily,
                   WCHAR* lfFaceName
                  )
{
  SetFont(lfHeight,
          lfWidth,
          lfEscapement,
          lfOrientation,
          lfWeight,
          lfItalic,
          lfUnderline,
          lfStrikeOut,
          lfCharSet,
          lfOutPrecision,
          lfClipPrecision,
          lfQuality,
          lfPitchAndFamily,
          lfFaceName
         );
}


//**********************************************************************
// CRhsFont::~CRhsFont
// -------------------
// Delete the HFONT if it has been created
//**********************************************************************

CRhsFont::~CRhsFont(void)
{
  if (m_hfont)
    DeleteObject(m_hfont);
}


//**********************************************************************
// CRhsFont::SetFont
// -----------------
// Set the font details
//**********************************************************************

const HFONT CRhsFont::SetFont(LOGFONT* LogFont)
{
  return(SetFont(LogFont->lfHeight,
                 LogFont->lfWidth,
                 LogFont->lfEscapement,
                 LogFont->lfOrientation,
                 LogFont->lfWeight,
                 LogFont->lfItalic,
                 LogFont->lfUnderline,
                 LogFont->lfStrikeOut,
                 LogFont->lfCharSet,
                 LogFont->lfOutPrecision,
                 LogFont->lfClipPrecision,
                 LogFont->lfQuality,
                 LogFont->lfPitchAndFamily,
                 LogFont->lfFaceName
                )
        );
}


const HFONT CRhsFont::SetFont(int  lfHeight,
                              int  lfWidth,
                              int  lfEscapement,
                              int  lfOrientation,
                              int  lfWeight,
                              BYTE lfItalic,
                              BYTE lfUnderline,
                              BYTE lfStrikeOut,
                              BYTE lfCharSet,
                              BYTE lfOutPrecision,
                              BYTE lfClipPrecision,
                              BYTE lfQuality,
                              BYTE lfPitchAndFamily,
                              WCHAR* lfFaceName
                             )
{ HFONT hf;
  LOGFONT lf;

// Initialise the font structure

  lf.lfHeight         = lfHeight;
  lf.lfWidth          = lfWidth;
  lf.lfEscapement     = lfEscapement;
  lf.lfOrientation    = lfOrientation;
  lf.lfWeight         = lfWeight;
  lf.lfItalic         = lfItalic;
  lf.lfUnderline      = lfUnderline;
  lf.lfStrikeOut      = lfStrikeOut;
  lf.lfCharSet        = lfCharSet;
  lf.lfOutPrecision   = lfOutPrecision;
  lf.lfClipPrecision  = lfClipPrecision;
  lf.lfQuality        = lfQuality;
  lf.lfPitchAndFamily = lfPitchAndFamily;
  lstrcpy(lf.lfFaceName, lfFaceName);

// Create the font

  hf = CreateFontIndirect(&lf);

  if (hf)
  { if (m_hfont && m_hfont != (HFONT) GetStockObject(ANSI_VAR_FONT))
      DeleteObject(m_hfont);
    m_hfont = hf;
    m_lfont = lf;
    return(m_hfont);
  }
  else
  { return(NULL);
  }
}


//**********************************************************************
// CRhsFont::ReadFont
// ------------------
// Read the font details from the registry
//**********************************************************************

const HFONT CRhsFont::ReadFont(CRhsRegistry* Ini)
{
  return(ReadFont(*Ini));
}

const HFONT CRhsFont::ReadFont(CRhsRegistry& Ini)
{ HFONT hf;
  LOGFONT lf;

// Read the font description

  lf.lfHeight         =        Ini.GetInt(L"lfHeight",         m_lfont.lfHeight);
  lf.lfWidth          =        Ini.GetInt(L"lfWidth",          m_lfont.lfWidth);
  lf.lfEscapement     =        Ini.GetInt(L"lfEscapement",     m_lfont.lfEscapement);
  lf.lfOrientation    =        Ini.GetInt(L"lfOrientation",    m_lfont.lfOrientation);
  lf.lfWeight         =        Ini.GetInt(L"lfWeight",         m_lfont.lfWeight);
  lf.lfItalic         = (BYTE) Ini.GetInt(L"lfItalic",         m_lfont.lfItalic);
  lf.lfUnderline      = (BYTE) Ini.GetInt(L"lfUnderline",      m_lfont.lfUnderline);
  lf.lfStrikeOut      = (BYTE) Ini.GetInt(L"lfStrikeOut",      m_lfont.lfStrikeOut);
  lf.lfCharSet        = (BYTE) Ini.GetInt(L"lfCharSet",        m_lfont.lfCharSet);
  lf.lfPitchAndFamily = (BYTE) Ini.GetInt(L"lfPitchAndFamily", m_lfont.lfPitchAndFamily);

  lstrcpy(lf.lfFaceName, Ini.GetString(L"lfFaceName", m_lfont.lfFaceName, LF_FACESIZE-1));

  lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
  lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
  lf.lfQuality        = DEFAULT_QUALITY;

// Create the font

  hf = CreateFontIndirect(&lf);

  if (hf)
  { if (m_hfont && m_hfont != (HFONT) GetStockObject(ANSI_VAR_FONT))
      DeleteObject(m_hfont);
    m_hfont = hf;
    m_lfont = lf;
    return(m_hfont);
  }
  else
  { return(NULL);
  }
}


//**********************************************************************
// CRhsFont::SaveFont
// ------------------
// Save the font details to the registry
//**********************************************************************

void CRhsFont::SaveFont(CRhsRegistry* Ini)
{
  SaveFont(*Ini);
}

void CRhsFont::SaveFont(CRhsRegistry& Ini)
{
  Ini.WriteInt(L"lfHeight",         m_lfont.lfHeight);
  Ini.WriteInt(L"lfWidth",          m_lfont.lfWidth);
  Ini.WriteInt(L"lfEscapement",     m_lfont.lfEscapement);
  Ini.WriteInt(L"lfOrientation",    m_lfont.lfOrientation);
  Ini.WriteInt(L"lfWeight",         m_lfont.lfWeight);
  Ini.WriteInt(L"lfItalic",         m_lfont.lfItalic);
  Ini.WriteInt(L"lfUnderline",      m_lfont.lfUnderline);
  Ini.WriteInt(L"lfStrikeOut",      m_lfont.lfStrikeOut);
  Ini.WriteInt(L"lfCharSet",        m_lfont.lfCharSet);
  Ini.WriteInt(L"lfPitchAndFamily", m_lfont.lfPitchAndFamily);
  Ini.WriteString(L"lfFaceName", m_lfont.lfFaceName);
}


//**********************************************************************
// CRhsFont::ChooseFont
// --------------------
// Ask the font to set itself using the Choose Font common dialog box
//**********************************************************************

const HFONT CRhsFont::ChooseFont(HWND hWnd, const WCHAR* Title)
{ BOOL b;
  HFONT hf;
  LOGFONT lf;
  CHOOSEFONT chf;

// Initialise the ChooseFont structure

  chf.lStructSize    = sizeof(CHOOSEFONT);
  chf.hwndOwner      = hWnd;
  chf.hDC            = NULL;
  chf.lpLogFont      = &lf;
  chf.Flags          = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
  chf.rgbColors      = RGB(0, 0, 0);
  chf.lCustData      = 0L;
  chf.lpfnHook       = NULL;
  chf.lpTemplateName = NULL;
  chf.hInstance      = NULL;
  chf.lpszStyle      = NULL;
  chf.nFontType      = SCREEN_FONTTYPE;

/*** Open the dialog box */

  lf = m_lfont;

  b = ::ChooseFont(&chf);

  if (!b)
    return(NULL);

// Create the font

  hf = CreateFontIndirect(&lf);

  if (hf)
  { m_hfont = hf;
    m_lfont = lf;
    return(m_hfont);
  }
  else
  { return(NULL);
  }
}


