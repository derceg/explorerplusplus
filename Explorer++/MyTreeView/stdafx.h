// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0601	// Change this to the appropriate value to target other versions of IE.
#endif

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

/* Disable 'unreferenced formal parameter' warning. */
#pragma warning( disable : 4100 )

/* Disable the '#pragma deprecated' warning. */
#pragma warning( disable : 4995 )

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
#include <gdiplus.h>
#include <Lm.h>
#include <intshcut.h>
#include <strsafe.h>
#include <dbt.h>
#include <Iphlpapi.h>
#include <psapi.h>
#include <userenv.h>