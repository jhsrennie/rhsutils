//**********************************************************************
// RhsCalc
// =======
//
// Reverse Polish calculator
//**********************************************************************

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <Misc/CRhsRegistry.h>
#include "RhsCalcDlg.h"


//**********************************************************************
// Function prototypes
//**********************************************************************

INT_PTR CALLBACK CalculatorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int winprintf(const WCHAR *, ...);


//**********************************************************************
// Global data
//**********************************************************************

HINSTANCE ThisInstance;

#define STACK_MAX 256
#define MEMORY_MAX 16

class RhsCalculator
{

  public:
    RhsCalculator();
    ~RhsCalculator();

    void ProcessInput(void);
    void ExecuteCommand(WCHAR *Command);
    double PopStack(void);
    void PushStack(double x);
    BOOL TestStack(int n);

  public:
    int StackTop;
    double Stack[STACK_MAX], Memory[MEMORY_MAX];

    double log_DBL_MAX;

    HWND hDlg, hTextWnd, hStackWnd, hMemoryWnd;

    CRhsRegistry* reg;

};


//*********************************************************************
// WinMain
// -------
//*********************************************************************

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{ RhsCalculator* calc;

  ThisInstance = hInstance;

  calc = new RhsCalculator;
  DialogBoxParam(ThisInstance, L"CalcBox", NULL, CalculatorProc, (LPARAM) calc);
  delete calc;

  return 0;
}


//**********************************************************************
// CalculatorProc
// --------------
// Dialog box function
//**********************************************************************

INT_PTR CALLBACK CalculatorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{ RhsCalculator* calc;
  int x, y, i;
  RECT r;
  WCHAR s[256];

  switch (message)
  {

// Initialise the dialog

    case WM_INITDIALOG:
      calc = (RhsCalculator*) lParam;
      SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR) calc);

      SetForegroundWindow(hDlg);
      x = calc->reg->GetInt(L"Left", -1);
      y = calc->reg->GetInt(L"Top",  -1);
      if (x > 0 && y > 0)
        SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

      calc->hDlg       = hDlg;
      calc->hTextWnd   = GetDlgItem(hDlg, IDD_CALC_TEXT);
      calc->hStackWnd  = GetDlgItem(hDlg, IDD_CALC_STACK);
      calc->hMemoryWnd = GetDlgItem(hDlg, IDD_CALC_MEMORY);
      SendMessage(calc->hTextWnd, EM_LIMITTEXT, 255, 0);
      for (i = 0; i < MEMORY_MAX; i++)
      { swprintf_s(s, 255, L"%i:", i);
        SendMessage(calc->hMemoryWnd, LB_ADDSTRING, 0, (LPARAM) s);
      }

      return(TRUE);

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {

// RETURN: Process input

        case 1:
          calc = (RhsCalculator*) GetWindowLongPtr(hDlg, GWLP_USERDATA);
          calc->ProcessInput();
          return(TRUE);

// CANCEL: Close the dialog box

        case IDCANCEL:
          EndDialog(hDlg, FALSE);
          return(TRUE);
      }
      break;

// On MOVE save the position

    case WM_MOVE:
      calc = (RhsCalculator*) GetWindowLongPtr(hDlg, GWLP_USERDATA);
      GetWindowRect(hDlg, &r);
      calc->reg->WriteInt(L"Left", r.left);
      calc->reg->WriteInt(L"Top", r.top);
      return(FALSE);
  }

  return(FALSE);
}


//**********************************************************************
// winprintf
// ---------
// Like printf but using a MessageBox.
//**********************************************************************

static int winprintf(const WCHAR *Format, ...)
{ WCHAR s[256];
  va_list ap;

  va_start(ap, Format);
  vswprintf_s(s, 255, Format, ap);
  va_end(ap);

  return(MessageBox(NULL, s, L"RhsCalculator", MB_OK | MB_ICONEXCLAMATION));
}


//**********************************************************************
// Class RhsCalculator
// ====================
//
// Constructor/Destructor
//**********************************************************************

RhsCalculator::RhsCalculator()
{
  StackTop = 0;
  log_DBL_MAX = log(DBL_MAX);

  reg = new CRhsRegistry(L"SOFTWARE\\Rhs\\RhsCalc", FALSE);
}


RhsCalculator::~RhsCalculator()
{
  delete reg;
}


//**********************************************************************
// Routine to parse the input string into individual commands, and
// pass them to the executor.
//**********************************************************************

void RhsCalculator::ProcessInput(void)
{ WCHAR *com;
  WCHAR* context;
  WCHAR s[256];

// Get and process the string

  GetWindowText(hTextWnd, s, 255);
  if (lstrlen(s) > 0)
  { CharUpper(s);

    com = wcstok_s(s, L" ", &context);
    while (com != NULL)
    { ExecuteCommand(com);
      com = wcstok_s(NULL, L" ", &context);
    }
  }

// Refresh the text box with the stack top

  if (StackTop > 0)
    swprintf_s(s, 255, L"%.10g", Stack[StackTop-1]);
  else
    s[0] = '\0';
  SetWindowText(hTextWnd, s);
  SendMessage(hTextWnd, EM_SETSEL, 0, 0X7FFF);
}


//**********************************************************************
// Routine to execute a command
//
// This is a bit of a monster because it contains a massive if else
// chain to work out which command needs processing.
//**********************************************************************

void RhsCalculator::ExecuteCommand(WCHAR *com)
{ int i;
  double x, y, z;
  WCHAR s[256];

// Stack manipulation

  if (lstrcmp(com, L"POP") == 0)
  { if (TestStack(1)) PopStack();
    return;
  }

  if (lstrcmp(com, L"SWAP") == 0 || lstrcmp(com, L"EXCH") == 0)
  { if (TestStack(2))
    { x = PopStack();
      y = PopStack();
      PushStack(x);
      PushStack(y);
    }
    return;
  }

  if (lstrcmp(com, L"DUP") == 0)
  { if (TestStack(1))
    { x = PopStack();
      PushStack(x);
      PushStack(x);
    }
    return;
  }

  if (lstrcmp(com, L"CLEAR") == 0)
  { StackTop = 0;
    SendMessage(hStackWnd, LB_RESETCONTENT, 0, 0L);
    return;
  }

// Numeric constants

  if (swscanf_s(com, L"%lf", &x) == 1)
  { PushStack(x);
    return;
  }

  if (lstrcmp(com, L"PI") == 0)
  { PushStack((double) 3.1415926535);
    return;
  }

  if (lstrcmp(com, L"E") == 0)
  { PushStack((double) 2.7182818);
    return;
  }

// Simple arithmetic

  if (lstrcmp(com, L"+") == 0)
  { if (TestStack(2))
    { x = PopStack();
      y = PopStack();
      if (x > 0 && y > 0)
      { if (x > DBL_MAX - y)
        { winprintf(L"Overflow in function \"+\"");
          PushStack(y); PushStack(x);
          return;
        }
      }
      if (x < 0 && y < 0)
      { if (x < -DBL_MAX - y)
        { winprintf(L"Overflow in function \"+\"");
          PushStack(y); PushStack(x);
          return;
        }
      }
      PushStack(y + x);
    }
    return;
  }

  if (lstrcmp(com, L"-") == 0)
  { if (TestStack(2))
    { x = PopStack();
      y = PopStack();
      if (y > 0 && x < 0)
      { if (x < y - DBL_MAX)
        { winprintf(L"Overflow in function \"-\"");
          PushStack(y); PushStack(x);
          return;
        }
      }
      if (y < 0 && x > 0)
      { if (x > DBL_MAX + y)
        { winprintf(L"Overflow in function \"-\"");
          PushStack(y); PushStack(x);
          return;
        }
      }
      PushStack(y - x);
    }
    return;
  }

  if (lstrcmp(com, L"*") == 0)
  { if (TestStack(2))
    { x = PopStack();
      y = PopStack();
      if (x == 0 || y == 0)
        PushStack(0.0);
      else
      { if (log(fabs(x)) + log(fabs(y)) < log_DBL_MAX)
          PushStack(y*x);
        else
        { PushStack(y);
          PushStack(x);
          winprintf(L"Overflow in routine \"*\"");
        }
      }
    }
    return;
  }

  if (lstrcmp(com, L"/") == 0)
  { if (TestStack(2))
    { x = PopStack();
      if (x == 0)
      { PushStack(x);
        winprintf(L"Division by zero in routine \"/\"");
      }
      else
      { y = PopStack();
        if (y == 0)
          PushStack(0.0);
        else
        { if (log(fabs(y)) - log(fabs(x)) < log_DBL_MAX)
            PushStack(y/x);
          else
          { PushStack(y);
            PushStack(x);
            winprintf(L"Overflow in routine \"/\"");
          }
        }
      }
    }
    return;
  }

// Slightly more complex arithmatic

  if (lstrcmp(com, L"SQR") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (log(fabs(x))*2 > log_DBL_MAX)
      { PushStack(x);
        winprintf(L"Overflow in routine \"sqr\"");
      }
      else
        PushStack(x * x);
    }
    return;
  }

  if (lstrcmp(com, L"SQRT") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x >= 0) PushStack(sqrt(x));
      else
      { PushStack(x);
        winprintf(L"Attempt to take square root of negative number");
      }
    }
    return;
  }

  if (lstrcmp(com, L"RECIP") == 0 || lstrcmp(com, L"INV") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x == 0)
      { PushStack(x);
        winprintf(L"Attempt to take reciprocal of zero");
      }
      else
        PushStack(1.0/x);
    }
    return;
  }

  if (lstrcmp(com, L"**") == 0 || lstrcmp(com, L"^") == 0)
  { if (TestStack(2))
    { x = PopStack();
      y = PopStack();
      if (y < 0)
      { PushStack(y);
        PushStack(x);
        winprintf(L"Cannot raise power of negative number");
      }
      else
      { z = log(y);
        if (log(fabs(z)) + log(fabs(x)) > log_DBL_MAX)
        { PushStack(y);
          PushStack(x);
          winprintf(L"Overflow in routine \"**\"");
        }
        else if (x*z > log_DBL_MAX)
        { PushStack(y);
          PushStack(x);
          winprintf(L"Overflow in routine \"**\"");
          return;
        }
        else
          PushStack(exp(x*z));
      }
    }
    return;
  }

// Simple functions

  if (lstrcmp(com, L"CHS") == 0)
  { if (TestStack(1)) PushStack(-PopStack());
    return;
  }

  if (lstrcmp(com, L"INT") == 0)
  { if (TestStack(1))
    { x = PopStack();
      PushStack(x - fmod(x, 1.0));
    }
    return;
  }

// Log functions

  if (lstrcmp(com, L"LOG") == 0 || lstrcmp(com, L"LN") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x > 0) PushStack(log(x));
      else
      { PushStack(x);
        winprintf(L"Attempt to take logarithm of negative number or zero");
      }
    }
    return;
  }

  if (lstrcmp(com, L"EXP") == 0 || lstrcmp(com, L"ALN") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x < log_DBL_MAX) PushStack(exp(x));
      else
      { PushStack(x);
        winprintf(L"Overflow in function \"EXP\"");
      }
    }
    return;
  }

  if (lstrcmp(com, L"LOG10") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x > 0) PushStack(log10(x));
      else
      { PushStack(x);
        winprintf(L"Attempt to take logarithm of negative number or zero");
      }
    }
    return;
  }

  if (lstrcmp(com, L"ALOG10") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x < log_DBL_MAX/log(10.0)) PushStack(exp(x*log(10.0)));
      else
      { PushStack(x);
        winprintf(L"Overflow in function ALOG10");
      }
    }
    return;
  }

// Trigonometric functions

  if (lstrcmp(com, L"SIN") == 0)
  { if (TestStack(1))
    { PushStack(sin(PopStack()));
    }
    return;
  }

  if (lstrcmp(com, L"COS") == 0)
  { if (TestStack(1))
    { PushStack(cos(PopStack()));
    }
    return;
  }

  if (lstrcmp(com, L"TAN") == 0)
  { if (TestStack(1))
    { PushStack(tan(PopStack()));
      PushStack(x * x);
    }
    return;
  }

  if (lstrcmp(com, L"ASIN") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x >= -1 && x <= 1) PushStack(asin(x));
      else
      { PushStack(x);
        winprintf(L"Out of range error in function ASIN");
      }
    }
    return;
  }

  if (lstrcmp(com, L"ACOS") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (x >= -1 && x <= 1) PushStack(acos(x));
      else
      { PushStack(x);
        winprintf(L"Out of range error in function ACOS");
      }
    }
    return;
  }

  if (lstrcmp(com, L"ATAN") == 0)
  { if (TestStack(1)) PushStack(atan(PopStack()));
    return;
  }

  if (lstrcmp(com, L"SINH") == 0)
  { if (TestStack(1)) PushStack(sinh(PopStack()));
    return;
  }

  if (lstrcmp(com, L"COSH") == 0)
  { if (TestStack(1)) PushStack(cosh(PopStack()));
    return;
  }

  if (lstrcmp(com, L"TANH") == 0)
  { if (TestStack(1)) PushStack(tanh(PopStack()));
    return;
  }

// Memory functions

  if (lstrcmp(com, L"STORE") == 0 || lstrcmp(com, L"STO") == 0)
  { if (TestStack(2))
    { x = PopStack();
      if (fmod(x, 1.0) == 0)
      { i = (int) x;
        if (i > -1 && i < MEMORY_MAX)
        { Memory[i] = PopStack();
          PushStack(Memory[i]);
          swprintf_s(s, 255, L"%i:  %g", i, Memory[i]);
          SendMessage(hMemoryWnd, LB_DELETESTRING, i, 0L);
          SendMessage(hMemoryWnd, LB_INSERTSTRING, i, (LPARAM) s);
        }
        else
        { winprintf(L"Invalid memory cell \"%i\"", i);
        }
      }
      else
      { winprintf(L"Invalid memory cell \"%f\"", x);
      }
    }
    return;
  }

  if (lstrcmp(com, L"RCL") == 0 || lstrcmp(com, L"RECALL") == 0)
  { if (TestStack(1))
    { x = PopStack();
      if (fmod(x, 1.0) == 0)
      { i = (int) x;
        if (i > -1 && i < MEMORY_MAX)
          PushStack(Memory[i]);
        else
        { winprintf(L"Invalid memory cell \"%i\"", i);
        }
      }
      else
      { winprintf(L"Invalid memory cell \"%f\"", x);
      }
    }
    return;
  }

// Exit

  if (lstrcmp(com, L"EXIT") == 0 || lstrcmp(com, L"QUIT") == 0)
  { EndDialog(hDlg, TRUE);
    return;
  }

// Unrecognised functions end up here

  winprintf(L"Unrecognised command \"%s\"", com);
}


//**********************************************************************
// Routine to pop a number off the stack
//**********************************************************************

double RhsCalculator::PopStack(void)
{
  if (StackTop <= 0)
  { winprintf(L"Stack is empty");
    return(0);
  }

  SendMessage(hStackWnd, LB_DELETESTRING, 0, 0L);

  return(Stack[--StackTop]);
}


//**********************************************************************
// Routine to push a number onto the stack
//**********************************************************************

void RhsCalculator::PushStack(double x)
{ WCHAR s[256];

  if (StackTop == STACK_MAX)
  { winprintf(L"Stack is full");
    return;
  }

  swprintf_s(s, 255, L"%.10g", x);
  SendMessage(hStackWnd, LB_INSERTSTRING, 0, (LPARAM) s);

  Stack[StackTop++] = x;
}


//**********************************************************************
// Routine to check the stack.
// Passed numbor of items, N.
// Returns TRUE if N items exist on stack.
//**********************************************************************

BOOL RhsCalculator::TestStack(int n)
{
  if (StackTop >= n) return TRUE;
  else
  { winprintf(L"Not enough data on stack");
    return FALSE;
  }
}


