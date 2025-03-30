// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma warning(push)
#pragma warning(disable : 4464) // relative include path contains '..'
#include "../targetver.h"
#pragma warning(pop)

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

// While wstring_convert is deprecated in C++17, it won't be removed from the
// language until a replacement is added. Therefore, it should be safe to keep
// using it for now and the deprecation warning can be ignored.
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#define STRICT

#define STRICT_TYPED_ITEMIDS

#define WIL_SUPPRESS_EXCEPTIONS

#define PATHCCH_NO_DEPRECATE

#include "DisableUnaligned.h"

// Windows Header Files:
// clang-format off
#include <winsock2.h>
#include <windows.h>
// clang-format on
#include <Iphlpapi.h>
#include <Lm.h>
#include <WinUser.h>
#include <accctrl.h>
#include <aclapi.h>
#include <assert.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <dbt.h>
#include <gdiplus.h>
#include <intrin.h>
#include <intsafe.h>
#include <intshcut.h>
#include <malloc.h>
#include <math.h>
#include <objidl.h>
#include <propvarutil.h>
#include <psapi.h>
#include <richedit.h>
#include <sddl.h>
#include <shellapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <stdint.h>
#include <strsafe.h>
#include <tchar.h>
#include <time.h>
#include <userenv.h>
#include <wincrypt.h>
#include <windowsx.h>
#include <winioctl.h>
#include <wmsdk.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GLOG_USE_GLOG_EXPORT
#include <glog/logging.h>

// WinRT
#include "WinRTBaseWrapper.h"

// C++ Header Files:
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
