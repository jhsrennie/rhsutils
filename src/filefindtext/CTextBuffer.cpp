// *********************************************************************
// CTextBuffer
// ===========
// Class to buffer output text before printing it.
// This class uses a temporary file to buffer the text.
// NB this is a quick and dirty implementation with minimal error checks
// *********************************************************************

#include <windows.h>
#include <stdio.h>
#include "CTextBuffer.h"


//**********************************************************************
// CTextBuffer
// -----------
//**********************************************************************

CTextBuffer::CTextBuffer()
{
  m_File = NULL;
  m_IOState = 0; // 0 = not open, 1 = write, 2 = read

// Build the temporary file at this stage so we know what to delete later

  WCHAR temppath[MAX_PATH+1];

  GetTempPath(MAX_PATH, temppath);
  GetTempFileName(temppath, L"fft", 0, m_TempFileName);
}


CTextBuffer::~CTextBuffer()
{

// If the file is open close it

  if (m_File)
    fclose(m_File);

// Delete the file. If the file doesn't exist this will fail harmlessly

  DeleteFile(m_TempFileName);
}


//**********************************************************************
// CTextBuffer
// -----------
//**********************************************************************

BOOL CTextBuffer::Initialise(void)
{

// If a file is currently open close it

  if (m_IOState != 0)
  {
    fclose(m_File);
    m_File = NULL;
    m_IOState = 0;
  }

// Now open a temporary file for writing

  if (_wfopen_s(&m_File, m_TempFileName, L"wb") != 0)
  {
    return FALSE;
  }

  m_IOState = 1;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// CTextBuffer
// -----------
//**********************************************************************

BOOL CTextBuffer::Readback(void)
{

// We must be currently writing to a file

  if (m_IOState != 1)
  {
    return FALSE;
  }

// Close the file and reopen it

  fclose(m_File);
  m_IOState = 0;

  if (_wfopen_s(&m_File, m_TempFileName, L"rb") != 0)
  {
    return FALSE;
  }

  m_IOState = 2;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// printf
// ------
// Write a line to the temporary file
// Note that this function uses ANSI not UNICODE
//**********************************************************************

int CTextBuffer::printf(const char* Format, ...)
{
  int numtowrite, numwritten;
  va_list ap;

// The file must be open for writing

  if (m_IOState != 1)
    return EOF;

// Build the output string

  va_start(ap, Format);
  numtowrite = vsprintf_s(m_Buf, LEN_CTEXTBUFFERBUF-1, Format, ap);
  va_end(ap);

// Write to the buffer

  numwritten = fputs(m_Buf, m_File);

// For GUI apps append the text to the display window

  return numwritten;
}


//**********************************************************************
// gets
// ----
// Read the next line from the temporary file
// Note that this includes the terminating \r\n
//**********************************************************************

const char* CTextBuffer::gets(void)
{

// The file must be open for reading

  if (m_IOState != 2)
    return NULL;

// Read the next line from the temporary file

  if (!fgets(m_Buf, LEN_CTEXTBUFFERBUF-1, m_File))
    return NULL;

// Return a pointer to the buffer containing the string

  return m_Buf;
}


