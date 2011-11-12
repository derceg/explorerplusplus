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
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#ifdef UNICODE
#define _itot_s _itow_s
#else
#define _itot_s _itoa_s
#endif

#define STRICT

#define ISOLATION_AWARE_ENABLED	1

#define OEMRESOURCE

/* Disable 'unreferenced formal parameter' warning. */
#pragma warning( disable : 4100 )

/* Disable the '#pragma deprecated' warning. */
#pragma warning( disable : 4995 )

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <winbase.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shlwapi.h>
#include <dbt.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <tchar.h>
#include <Imagehlp.h>
#include <shellapi.h>
#include <prsht.h>
#include <math.h>
#include <richedit.h>
#include <objidl.h>
#include <shlguid.h>
#include <gdiplus.h>
#include <strsafe.h>
#include <psapi.h>
