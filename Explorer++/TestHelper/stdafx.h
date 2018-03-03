// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#include <ShlGuid.h>

#pragma warning(push)
#pragma warning(disable:4458)
#include <GdiPlus.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4995)
#include "gtest\gtest.h"
#pragma warning(pop)