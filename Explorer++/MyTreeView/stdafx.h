// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

#ifdef UNICODE
#define _ctoi _wtoi
#else
#define _ctoi atoi
#endif

#ifdef UNICODE
#define cstrtok_s wcstok_s
#else
#define cstrtok_s strtok_s
#endif

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES	1
#define STRICT

// Windows Header Files:
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <string>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objidl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <math.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlguid.h>
#include <wincrypt.h>
#include <malloc.h>
#include <time.h>
#include <math.h>
#include <winioctl.h>
#include <richedit.h>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <Lm.h>
#include <intshcut.h>
#include <strsafe.h>
#include <dbt.h>
#include <Iphlpapi.h>
#include <psapi.h>
#include <userenv.h>

#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4995)
#include <list>
#pragma warning(pop)

#include <vector>