// *********************************************************************
// DiskThrasher
// ============
// Somewhat experimental app to test disk speed
// The app creates 4 threads, and each thread:
// - creates a file with 256 1MB blocks (i.e. 256MB)
// - quicksorts the blocks in the file in ascending order of the first
//   byte in the block
// The idea is to test both streaming speed and random access
// *********************************************************************

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


//**********************************************************************
// Prototypes
// ----------
//**********************************************************************

DWORD SortThread(DWORD ThreadId);

BOOL SortFile(HANDLE hFile, int Left, int Right, char* Buff, DWORD ThreadId);
BOOL SortFileSwap(HANDLE hFile, int i, int j, char* Buff, DWORD ThreadId);

BOOL ReadBlock(HANDLE hFile, int BlockNum, char* Buff, DWORD ThreadId);
BOOL WriteBlock(HANDLE hFile, int BlockNum, char* Buff, DWORD ThreadId);

double GetCounter(void);

const char* GetGUID(char* Guid);

const char* GetLastErrorMessage(void);


// *********************************************************************
// Global variables
// ----------------
// *********************************************************************

#define SYNTAX "DiskThrasher: Simple disk test utility"

#define TEST_NUMBLKS 0x100
#define TEST_BLKSIZE 0x100000

typedef struct tagThreadInfo
{ HANDLE hThread;
  HANDLE hInitDone;
  DWORD BlocksRead, BlocksWritten;
  double ReadTimer, WriteTimer;
} ThreadInfo;

#define TEST_NUMTHREADS 4
ThreadInfo g_Threads[TEST_NUMTHREADS];

HANDLE g_StartTest;


// *********************************************************************
// main
// ----
// *********************************************************************

int main(int argc, char* argv[])
{ int i;
  DWORD tick_start, tick_finished;
  DWORD blocksread, blockswritten, d;
  double readspeed, writespeed, readtimer, writetimer, timerfreq;
  LARGE_INTEGER li;
  SYSTEMTIME st;

// Check the arguments

  if (argc == 2)
  { if (strcmp(argv[1], "/?") == 0 || strcmp(argv[1], "-?") == 0)
    { printf("%s\n", SYNTAX);
      return 0;
    }
  }

  if (argc != 1)
  { printf("%s\n", SYNTAX);
    return 2;
  }

// Print banner

  printf("DiskThrasher\n============\n");
  printf("No. threads: %i\n", TEST_NUMTHREADS);
  printf("No. blocks:  %i\n", TEST_NUMBLKS);
  printf("Block size:  %i\n", TEST_BLKSIZE);
  printf("\n");

// Start the test

  GetLocalTime(&st);
  printf("Starting test at %02i:%02i:%02i\n", st.wHour, st.wMinute, st.wSecond);
  printf("\n");

// Create the event used to signal the test start

  g_StartTest = CreateEvent(NULL, TRUE, FALSE, NULL);

// Create the threads

  for (i = 0; i < TEST_NUMTHREADS; i++)
  { printf("Creating thread %i\n", i);

// For each thread create an event: the thread signals this event to
// indicate it has initialised and is ready to start

    g_Threads[i].hInitDone = CreateEvent(NULL, TRUE, FALSE, NULL);

// Create the thread

    g_Threads[i].hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) SortThread, (LPVOID) i, 0, &d);
  }

// Wait for all the threads to signal they are intialised

  printf("Waiting for threads to initialise ...\n");

  for (i = 0; i < TEST_NUMTHREADS; i++)
    WaitForSingleObject(g_Threads[i].hInitDone, INFINITE);

  printf("Done\n\n");

// Start the test

  printf("Starting the test\n");

  tick_start = GetTickCount();
  SetEvent(g_StartTest);

// Wait for all the threads to complete

  for (i = 0; i < TEST_NUMTHREADS; i++)
    WaitForSingleObject(g_Threads[i].hThread, INFINITE);

  tick_finished = GetTickCount();
  printf("Test finished\n\n");

// Compute the read and write speeds using the high frequency timer

  QueryPerformanceFrequency(&li);
  timerfreq = li.HighPart;
  timerfreq *= 4294967296.0;
  timerfreq += li.LowPart;

  blocksread = blockswritten = 0;
  readtimer = writetimer = 0.0;

  for (i = 0; i < TEST_NUMTHREADS; i++)
  { blocksread += g_Threads[i].BlocksRead;
    blockswritten += g_Threads[i].BlocksWritten;

    readtimer += g_Threads[i].ReadTimer;
    writetimer += g_Threads[i].WriteTimer;
  }

  readtimer /= timerfreq;
  writetimer /= timerfreq;

// Print the stats

  readspeed = (double) blocksread;
  readspeed *= TEST_BLKSIZE/1048576.0;
  readspeed *= TEST_NUMTHREADS;
  readspeed /= readtimer;
  writespeed = (double) blockswritten;
  writespeed *= TEST_BLKSIZE/1048576.0;
  writespeed *= TEST_NUMTHREADS;
  writespeed /= writetimer;

  printf("Test time:       %i seconds\n", (tick_finished - tick_start)/1000);
  printf("Blocks read:     %8i, total read time:  %.3f seconds\n", blocksread, readtimer);
  printf("Blocks written:  %8i, total write time: %.3f seconds\n", blockswritten, writetimer);
  printf("Av. read speed:  %.3f MB/sec\n", readspeed);
  printf("Av. write speed: %.3f MB/sec\n", writespeed);

// All finished

  printf("Finished\n");

  return 0;
}


// *********************************************************************
// SortThread
// ----------
// *********************************************************************

DWORD SortThread(DWORD ThreadId)
{ int i;
  DWORD d;
  char* buf;
  HANDLE hfile;
  char filename[MAX_PATH+1];

// Allocate and fill a buffer with random data
// The buffer is two blocks in size so it can be used by the sort 
// routines

  buf = (char*) GlobalAlloc(GMEM_FIXED, 3*TEST_BLKSIZE);

  for (i = 0; i < 2*TEST_BLKSIZE; i++)
    buf[i] = (char) (rand() & 0xFF);

// Create the test file and populate it with the random data

  GetGUID(filename);
  hfile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING | FILE_FLAG_DELETE_ON_CLOSE, NULL);

  for (i = 0; i < TEST_NUMBLKS; i++)
  { CopyMemory(buf + 2*TEST_BLKSIZE, buf + (rand() % TEST_BLKSIZE), TEST_BLKSIZE);
    if (!WriteFile(hfile, buf + 2*TEST_BLKSIZE, TEST_BLKSIZE, &d, NULL))
    { printf("Panic! %s\n", GetLastErrorMessage());
      return 0;
    }
  }

  SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

// Signal that we have completed initialisation

  SetEvent(g_Threads[ThreadId].hInitDone);

// Wait for the main thread to start the test

  WaitForSingleObject(g_StartTest, INFINITE);

// Do the sort

  g_Threads[ThreadId].BlocksRead = 0;
  g_Threads[ThreadId].BlocksWritten = 0;

  g_Threads[ThreadId].ReadTimer = 0;
  g_Threads[ThreadId].WriteTimer = 0;

  SortFile(hfile, 0, TEST_NUMBLKS-1, buf, ThreadId);

// Close the file

  CloseHandle(hfile);

// Thread is finished

  return 0;
}


// *********************************************************************
// SortFile
// --------
// The file consists of an array of blocks. This function sorts the
// blocks in ascending order of the first byte in each block.
// The function is passed a buffer that is at least three blocks in size
// and it should use this for handling data.
// *********************************************************************

BOOL SortFile(HANDLE hFile, int Left, int Right, char* Buff, DWORD ThreadId)
{ int i, last;
  int left, right;

  if (Left >= Right) return TRUE;

  SortFileSwap(hFile, Left, (Left + Right)/2, Buff, ThreadId);
  last = Left;

  ReadBlock(hFile, Left, Buff, ThreadId);
  left = *((int*) Buff);

  for (i = Left + 1; i <= Right; i++)
  { ReadBlock(hFile, i, Buff, ThreadId);
    right = *((int*) Buff);
  
    if (left > right)
      SortFileSwap(hFile, ++last, i, Buff, ThreadId);
  }

  SortFileSwap(hFile, Left, last, Buff, ThreadId);

  SortFile(hFile, Left, last-1, Buff, ThreadId);
  SortFile(hFile, last+1, Right, Buff, ThreadId);

// Return indicating success

  return TRUE;
}


BOOL SortFileSwap(HANDLE hFile, int i, int j, char* Buff, DWORD ThreadId)
{

// Read block i and store it in the buffer

  ReadBlock(hFile, i, Buff, ThreadId);

// Read block j and store it in the top half of the buffer

  ReadBlock(hFile, j, Buff+TEST_BLKSIZE, ThreadId);

// Write block j back to position i

  WriteBlock(hFile, i, Buff+TEST_BLKSIZE, ThreadId);

// Finally write block i back to position j

  WriteBlock(hFile, j, Buff, ThreadId);

// Return indicating success

  return TRUE;
}


// *********************************************************************
// ReadBlock
// ---------
// *********************************************************************

BOOL ReadBlock(HANDLE hFile, int BlockNum, char* Buff, DWORD ThreadId)
{ DWORD d;
  double timer;

  timer = GetCounter();

// Set the file pointer to the block

  if (SetFilePointer(hFile, BlockNum*TEST_BLKSIZE, NULL, FILE_BEGIN) == 0xFFFFFFFF)
  { printf("SetFilePointer failed: %s\n", GetLastErrorMessage());
    return FALSE;
  }

// Read the block

  if (!ReadFile(hFile, Buff, TEST_BLKSIZE, &d, NULL))
  { printf("ReadFile failed: %s\n", GetLastErrorMessage());
    return FALSE;
  }

// Return indicating success

  g_Threads[ThreadId].BlocksRead++;

  timer = GetCounter() - timer;
  g_Threads[ThreadId].ReadTimer += timer;

  return TRUE;
}


// *********************************************************************
// WriteBlock
// ----------
// *********************************************************************

BOOL WriteBlock(HANDLE hFile, int BlockNum, char* Buff, DWORD ThreadId)
{ DWORD d;
  double timer;

  timer = GetCounter();

// Set the file pointer to the block

  if (SetFilePointer(hFile, BlockNum*TEST_BLKSIZE, NULL, FILE_BEGIN) == 0xFFFFFFFF)
  { printf("SetFilePointer failed: %s\n", GetLastErrorMessage());
    return FALSE;
  }

// Read the block

  if (!WriteFile(hFile, Buff, TEST_BLKSIZE, &d, NULL))
  { printf("WriteFile failed: %s\n", GetLastErrorMessage());
    return FALSE;
  }

// Return indicating success

  g_Threads[ThreadId].BlocksWritten++;

  timer = GetCounter() - timer;
  g_Threads[ThreadId].WriteTimer += timer;

  return TRUE;
}


//**********************************************************************
// GetCounter
// ----------
// Get the high frequency counter.
// Convert it to a double to save messing about with 64 bit integers.
//**********************************************************************

double GetCounter(void)
{ double d;
  LARGE_INTEGER li;

  QueryPerformanceCounter(&li);
  d = li.HighPart;
  d *= 4294967296.0;
  d += li.LowPart;

  return d;
}


//**********************************************************************
// GetGUID
// -------
// Create a GUID and strip the non-alpha characters
// The buffer Guid is assumed to be at least 33 characters long
//**********************************************************************

#define LEN_GUIDSTR 38 // length without the terminating '\0'
#define LEN_STRIPPED_GUID 32 // length without the terminating '\0'

const char* GetGUID(char* Guid)
{ int i, j;
  GUID new_guid;
  WCHAR w_new_guid[LEN_GUIDSTR+1];
  char  s_new_guid[LEN_GUIDSTR+1];

// Get the GUID

  CoCreateGuid(&new_guid);
  StringFromGUID2(new_guid, w_new_guid, LEN_GUIDSTR + 1);
  WideCharToMultiByte(CP_ACP, 0, w_new_guid, -1, s_new_guid, LEN_GUIDSTR, NULL, NULL);

// Strip the non-alpha characters

  for (i = j = 0; i < LEN_GUIDSTR && s_new_guid[i] != '\0'; i++)
    if (s_new_guid[i] >= '0' && s_new_guid[i] <= '9' || s_new_guid[i] >= 'A' && s_new_guid[i] <= 'F' || s_new_guid[i] >= 'a' && s_new_guid[i] <= 'f')
      s_new_guid[j++] = s_new_guid[i];
  s_new_guid[j] = '\0';

// Return the guid

  lstrcpy(Guid, s_new_guid);

  return Guid;
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
//**********************************************************************

const char* GetLastErrorMessage(void)
{ DWORD d;
  static char errmsg[1024];

  d = GetLastError();

  strcpy(errmsg, "<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 1023, NULL);
  return(errmsg);
}


