// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES	1

// While wstring_convert is deprecated in C++17, it won't be removed from the
// language until a replacement is added. Therefore, it should be safe to keep
// using it for now and the deprecation warning can be ignored.
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#define STRICT

#define STRICT_TYPED_ITEMIDS

// Windows Header Files:
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <WinUser.h>
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

/* See https://stackoverflow.com/questions/39797242/where-do-i-get-the-correct-gdi-c-wrappers-from-for-vs21015.
This workaround is
required for VS 2015. */
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)

/* Temporarily disable the
"#pragma deprecated" warning. */
#pragma warning(push)
#pragma warning(disable:4995)
#include <intrin.h>
#pragma warning(pop)

#include <wmsdk.h>
#include <propvarutil.h>

/* See http://connect.microsoft.com/VisualStudio/feedback/details/621653/including-stdint-after-intsafe-generates-warnings.
This workaround is only
required for VS 2010. */
#pragma warning(push)
#pragma warning(disable:4005)
#include <stdint.h>
#include <intsafe.h>
#pragma warning(pop)

#include <assert.h>
#include <list>
#include <memory>
#include <sstream>
#include <vector>