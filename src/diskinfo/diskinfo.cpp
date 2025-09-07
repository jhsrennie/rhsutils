// *********************************************************************
// diskinfo
// ========
// Listing of disks, partitions, etc
//
// John Rennie
// 01/01/2013
// *********************************************************************

#include <windows.h>
#include <stdio.h>
#include <CRhsIO/CRhsIO.h>
#include <Misc/CRhsFindFile.h>
#include <Misc/CRhsDate.h>


//**********************************************************************
// To avoid having to link the DDK headers and libraries we manually
// specify the structures and functions that we need.
//**********************************************************************

#define SYMBOLIC_LINK_QUERY 1
#define DIRECTORY_QUERY 1

#define OBJ_CASE_INSENSITIVE    0x00000040L

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

typedef struct _LSA_UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING, UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
  ULONG           Length;
  HANDLE          RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG           Attributes;
  PVOID           SecurityDescriptor;
  PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

typedef struct _OBJECT_DIRECTORY_INFORMATION
{
  UNICODE_STRING ObjectName;
  UNICODE_STRING ObjectTypeName; // e.g. Directory, Device ...
  char Data[1]; // variable length array
} OBJECT_DIRECTORY_INFORMATION;

typedef VOID (WINAPI *_RtlInitUnicodeString)(
  PUNICODE_STRING DestinationString,
  PCWSTR SourceString
);
_RtlInitUnicodeString RtlInitUnicodeString = NULL;

typedef NTSTATUS (WINAPI *_NtOpenSymbolicLinkObject)(
  PHANDLE LinkHandle,
  ACCESS_MASK DesiredAccess,
  POBJECT_ATTRIBUTES ObjectAttributes
);
_NtOpenSymbolicLinkObject NtOpenSymbolicLinkObject = NULL;

typedef NTSTATUS (WINAPI *_NtQuerySymbolicLinkObject)(
  HANDLE LinkHandle,
  PUNICODE_STRING LinkTarget,
  PULONG ReturnedLength
);
_NtQuerySymbolicLinkObject NtQuerySymbolicLinkObject = NULL;

typedef NTSTATUS (WINAPI *_NtOpenDirectoryObject)(
  PHANDLE DirectoryHandle,
  ACCESS_MASK DesiredAccess,
  POBJECT_ATTRIBUTES ObjectAttributes
);
_NtOpenDirectoryObject NtOpenDirectoryObject = NULL;

typedef NTSTATUS (WINAPI *_NtQueryDirectoryObject)(
  HANDLE DirectoryHandle,
  PVOID Buffer,
  ULONG Length,
  BOOLEAN ReturnSingleEntry,
  BOOLEAN RestartScan,
  PULONG Context,
  PULONG ReturnLength
);
_NtQueryDirectoryObject NtQueryDirectoryObject = NULL;


//**********************************************************************
// Prototypes
// ----------
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused);

BOOL AnalyseDisks(void);
BOOL AnalysePartitions(WCHAR* DiskDeviceName);

BOOL AnalyseVolumes(void);
BOOL EnumerateVolumeMountPoints(WCHAR* Volume);

BOOL GetNTLink(WCHAR* LinkName, WCHAR* LinkTarget);
BOOL LoadNTFunctions(void);
const WCHAR* GetLastErrorMessage(void);


//**********************************************************************
// Global variables
// ----------------
//**********************************************************************

#define LEN_DISKNAME       1024
#define LEN_PARTITIONNAME  1024
#define LEN_VOLUMEGUID      256
#define LEN_MOUNTPOINTNAME 1024
#define LEN_LINK           1024

CRhsIO RhsIO;

#define SYNTAX \
L"diskinfo v1.0\r\n"


//**********************************************************************
// WinMain
//**********************************************************************

int wmain(int argc, WCHAR* argv[])
{ int retcode;
  WCHAR lasterror[256];

  if (!RhsIO.Init(GetModuleHandle(NULL), L"diskinfo"))
  { wprintf(L"Error initialising: %s\n", RhsIO.LastError(lasterror, 256));
    return 2;
  }

  retcode = RhsIO.Run(rhsmain);

  return retcode;
}


//**********************************************************************
// Here we go
//**********************************************************************

DWORD WINAPI rhsmain(LPVOID unused)
{

// Load DDK functions from NT system libraries

  if (!LoadNTFunctions())
    return 2;

// Analyse the physical disks

  RhsIO.printf(L"Physical disks\n");
  RhsIO.printf(L"--------------\n\n");

  AnalyseDisks();

// Analyse the logical volumes

  RhsIO.printf(L"Logical volumes\n");
  RhsIO.printf(L"---------------\n\n");

  AnalyseVolumes();

// All done

  return 0;
}


//**********************************************************************
// AnalyseDisks
// -------------
// Do a directory listing of all devices and look for HarddiskN
//**********************************************************************

BOOL AnalyseDisks(void)
{
  DWORD index, objlen;
  UNICODE_STRING device;
  OBJECT_ATTRIBUTES attr;
  NTSTATUS status;
  HANDLE h_dev;
  OBJECT_DIRECTORY_INFORMATION *objinfo;
  WCHAR diskname[LEN_DISKNAME];
  BYTE objdata[0x1000];

// Start the directory listing

  RtlInitUnicodeString(&device, L"\\Device");
  InitializeObjectAttributes(&attr, &device, OBJ_CASE_INSENSITIVE, NULL, NULL);
  status = NtOpenDirectoryObject(&h_dev, STANDARD_RIGHTS_READ | DIRECTORY_QUERY, &attr);

  if (!NT_SUCCESS(status))
  {
    RhsIO.errprintf(L"NtOpenDirectoryObject failed: %d - %s", GetLastError(), GetLastErrorMessage());
    return FALSE;
  }

// Keep looping while there are devices available

  index = 0;

  while (NT_SUCCESS(status))
  {
    ZeroMemory(objdata, 0x1000);
    objinfo = (OBJECT_DIRECTORY_INFORMATION*) objdata;
    status = NtQueryDirectoryObject(h_dev, objinfo, 0x1000, TRUE, FALSE, &index, &objlen);

    if (NT_SUCCESS(status))
    {
      objinfo->ObjectName.Buffer[objinfo->ObjectName.Length] = 0;

      lstrcpy(diskname, objinfo->ObjectName.Buffer);
      diskname[8] = 0;

      if (lstrcmp(diskname, L"Harddisk") == 0)
      {
        if (objinfo->ObjectName.Buffer[8] >= '0' && objinfo->ObjectName.Buffer[8] <= '9')
        {
          lstrcpy(diskname, L"\\Device\\");
          lstrcat(diskname, objinfo->ObjectName.Buffer);
          RhsIO.printf(L"Disk: %s\n\n", diskname);
          RhsIO.printf(L"Partitions:\n");

          AnalysePartitions(diskname);
        }
      }
    }
  }

  CloseHandle(h_dev);

  return TRUE;
}


//**********************************************************************
// AnalysePartitions
// ------------------
// The disk device name is \Device\HarddiskN where N is an integer
//**********************************************************************

BOOL AnalysePartitions(WCHAR* DiskDeviceName)
{
  DWORD index, objlen;
  UNICODE_STRING device;
  OBJECT_ATTRIBUTES attr;
  NTSTATUS status;
  HANDLE h_dev;
  OBJECT_DIRECTORY_INFORMATION *objinfo;
  WCHAR partition_name[LEN_PARTITIONNAME], partition_link[LEN_LINK];
  BYTE objdata[0x1000];

// Start the directory listing

  RtlInitUnicodeString(&device, DiskDeviceName);
  InitializeObjectAttributes(&attr, &device, OBJ_CASE_INSENSITIVE, NULL, NULL);
  status = NtOpenDirectoryObject(&h_dev, STANDARD_RIGHTS_READ | DIRECTORY_QUERY, &attr);

  if (!NT_SUCCESS(status))
  {
    if (GetLastError() != ERROR_NO_MORE_FILES)
      RhsIO.errprintf(L"NtOpenDirectoryObject(%s) failed: %s", DiskDeviceName, GetLastErrorMessage());
    return FALSE;
  }

// Keep looping while there are devices available

  index = 0;

  while (NT_SUCCESS(status))
  {
    ZeroMemory(objdata, 0x1000);
    objinfo = (OBJECT_DIRECTORY_INFORMATION*) objdata;
    status = NtQueryDirectoryObject(h_dev, objinfo, 0x1000, TRUE, FALSE, &index, &objlen);

    if (NT_SUCCESS(status))
    {
      objinfo->ObjectName.Buffer[objinfo->ObjectName.Length] = 0;
      objinfo->ObjectTypeName.Buffer[objinfo->ObjectTypeName.Length] = 0;

      lstrcpy(partition_name, DiskDeviceName);
      lstrcat(partition_name, L"\\");
      lstrcat(partition_name, objinfo->ObjectName.Buffer);
      RhsIO.printf(L"  %s\n", partition_name);

      if (GetNTLink(partition_name, partition_link))
        RhsIO.printf(L"  Link to: %s\n", partition_link);

      RhsIO.printf(L"\n");
    }
  }

  CloseHandle(h_dev);

  return TRUE;
}


//**********************************************************************
// AnalyseVolumes
// --------------
//**********************************************************************

BOOL AnalyseVolumes(void)
{
  HANDLE hvol;
  WCHAR volume_name[LEN_VOLUMEGUID+1];
  WCHAR link_target[LEN_LINK+1];

// Find the first volume

  hvol = FindFirstVolume(volume_name, LEN_VOLUMEGUID);

  if (hvol == INVALID_HANDLE_VALUE)
  {
    RhsIO.errprintf(L"FindFirstVolume failed: %s", GetLastErrorMessage());
    return FALSE;
  }

  do
  {

// Volume GUID

    RhsIO.printf(L"Volume: %s\n", volume_name);

// Find any links

    // For some reason volume links start "\??\" rather than "\\?\"
    volume_name[1] = '?';

    if (GetNTLink(volume_name, link_target))
      RhsIO.printf(L"Link to: %s\n", link_target);

    volume_name[1] = '\\';

// Print the mount point(s)

    EnumerateVolumeMountPoints(volume_name);

// Finished this volume

    RhsIO.printf(L"\n");

  } while (FindNextVolume(hvol, volume_name, LEN_VOLUMEGUID));

  FindVolumeClose(hvol);

// Return indicating success

  return TRUE;
}


//**********************************************************************
// EnumerateVolumeMountPoints
// --------------------------
//**********************************************************************

BOOL EnumerateVolumeMountPoints(WCHAR* Volume)
{
  DWORD d;
  WCHAR mountpoint_name[LEN_MOUNTPOINTNAME+1];

  if (!GetVolumePathNamesForVolumeName(Volume, mountpoint_name, LEN_MOUNTPOINTNAME, &d))
  {
    RhsIO.errprintf(L"GetVolumePathNamesForVolumeName(%s) failed: %s", Volume, GetLastErrorMessage());
    return FALSE;
  }

  RhsIO.printf(L"Mounted as: ");

  if (!mountpoint_name[0])
  {
    RhsIO.printf(L"not mounted");
  }
  else
  {
    for (int i = 0; mountpoint_name[i]; i += (lstrlen(mountpoint_name + i) + 1))
    {
      RhsIO.printf(L"%s ", mountpoint_name + i);
    }
  }

  RhsIO.printf(L"\n");

  return TRUE;
}


//**********************************************************************
// GetNTLink
// ---------
// Resolve an NT symbolic link
//**********************************************************************

BOOL GetNTLink(WCHAR* LinkName, WCHAR* LinkTarget)
{
  int i;
  DWORD d;
  HANDLE hlink;
  NTSTATUS status;
  UNICODE_STRING link;
  OBJECT_ATTRIBUTES attr;
  WCHAR linkname[LEN_LINK+1];

// Trim off any trailing slash

  lstrcpy(linkname, LinkName);
  i = lstrlen(linkname);
  if (i > 1)
    if (linkname[i-1] == '\\')
      linkname[i-1] = 0;

// Attempt to open the link

  RtlInitUnicodeString(&link, linkname);
  InitializeObjectAttributes(&attr, &link, OBJ_CASE_INSENSITIVE, NULL, NULL);
  status = NtOpenSymbolicLinkObject(&hlink, SYMBOLIC_LINK_QUERY, &attr);

  if (!NT_SUCCESS(status))
  {
    return FALSE;
  }

// Found the link, so retrieve the link target

  link.Length        = 0;
  link.MaximumLength = LEN_LINK;
  link.Buffer        = LinkTarget;
  status = NtQuerySymbolicLinkObject(hlink, &link, &d);

  if (!NT_SUCCESS(status))
  {
    CloseHandle(hlink);
    return FALSE;
  }

  CloseHandle(hlink);

// The returned string isn't NULL terminated

  LinkTarget[d] = 0;

// Return indicating success

  return TRUE;
}


//**********************************************************************
// LoadNTFunctions
// ---------------
// Load NT/DDK functions
//**********************************************************************

BOOL LoadNTFunctions(void)
{
  HMODULE h_ntdll;

  h_ntdll = LoadLibrary(L"ntdll.dll");

  RtlInitUnicodeString = (_RtlInitUnicodeString) GetProcAddress(h_ntdll, "RtlInitUnicodeString");;
  if (!RtlInitUnicodeString)
  {
    RhsIO.errprintf(L"Failed to load RtlInitUnicodeString\n");
    return FALSE;
  }

  NtOpenSymbolicLinkObject = (_NtOpenSymbolicLinkObject) GetProcAddress(h_ntdll, "NtOpenSymbolicLinkObject");
  if (!NtOpenSymbolicLinkObject)
  {
    RhsIO.errprintf(L"Failed to load NtOpenSymbolicLinkObject\n");
    return FALSE;
  }

  NtQuerySymbolicLinkObject = (_NtQuerySymbolicLinkObject) GetProcAddress(h_ntdll, "NtQuerySymbolicLinkObject");
  if (!NtQuerySymbolicLinkObject)
  {
    RhsIO.errprintf(L"Failed to load NtQuerySymbolicLinkObject\n");
    return FALSE;
  }

  NtOpenDirectoryObject = (_NtOpenDirectoryObject) GetProcAddress(h_ntdll, "NtOpenDirectoryObject");
  if (!NtOpenDirectoryObject)
  {
    RhsIO.errprintf(L"Failed to load NtOpenDirectoryObject\n");
    return FALSE;
  }

  NtQueryDirectoryObject = (_NtQueryDirectoryObject) GetProcAddress(h_ntdll, "NtQueryDirectoryObject");
  if (!NtQueryDirectoryObject)
  {
    RhsIO.errprintf(L"Failed to load NtQueryDirectoryObject\n");
    return FALSE;
  }

  return TRUE;
}


//**********************************************************************
// GetLastErrorMessage
// -------------------
//**********************************************************************

const WCHAR* GetLastErrorMessage(void)
{ DWORD d;
  static WCHAR errmsg[1024];

  d = GetLastError();

  lstrcpy(errmsg, L"<unknown error>");
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, 0, errmsg, 1023, NULL);
  return(errmsg);
}


