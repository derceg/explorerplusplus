// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

#define ISOLATION_AWARE_ENABLED 1

#define OEMRESOURCE

#define STRICT

#define STRICT_TYPED_ITEMIDS

#define WIL_SUPPRESS_EXCEPTIONS

// Third-party Header Files:
#include "SolWrapper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include "../ThirdParty/cereal/archives/binary.hpp"
#include "../ThirdParty/cereal/types/memory.hpp"
#include "../ThirdParty/cereal/types/string.hpp"
#include "../ThirdParty/cereal/types/vector.hpp"

// Windows Header Files:
// clang-format off
#include <Winsock2.h>
#include <windows.h>
// clang-format on
#include <Imagehlp.h>
#include <VersionHelpers.h>
#include <WinInet.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <dbt.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <math.h>
#include <objidl.h>
#include <prsht.h>
#include <psapi.h>
#include <richedit.h>
#include <shellapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <strsafe.h>
#include <tchar.h>
#include <uxtheme.h>
#include <vfw.h>
#include <winbase.h>
#include <windowsx.h>
#include <winuser.h>
#include <wmsdk.h>

// It's important that Unknwn.h is included before any WinRT headers to ensure there is support for
// classic COM interfaces (i.e. IUnknown).
// clang-format off
#include <Unknwn.h>
#include <winrt/base.h>
// clang-format on

// The boost headers can include Windows header files. If the boost header files are included before
// the Windows header files above, that can cause issues, specifically with winsock.h and
// Winsock2.h.
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/functional/hash.hpp>
#include <boost/log/common.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/parameter.hpp>
#include <boost/signals2.hpp>

// wil/resource.h can use declarations from the Windows header files. For example,
// wil::unique_htheme depends on uxtheme.h being included first. So, this file is specifically
// included after the Windows headers.
#include <wil/resource.h>

// C++ Header Files:
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>
