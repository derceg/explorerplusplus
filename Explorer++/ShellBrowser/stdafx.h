// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

//#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES	1
#define STRICT

#define ISOLATION_AWARE_ENABLED	1

// Windows Header Files:
#include <Winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objidl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <dbt.h>
#include <math.h>
#include <tchar.h>
#include <strsafe.h>
#include <shlobj.h>
#include <iphlpapi.h>
#include <unknwn.h>
#include <wmsdk.h>

#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)