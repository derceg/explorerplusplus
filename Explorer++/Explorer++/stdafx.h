// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "../targetver.h"

#define ISOLATION_AWARE_ENABLED 1

#define OEMRESOURCE

#define STRICT

#define STRICT_TYPED_ITEMIDS

#define WIL_SUPPRESS_EXCEPTIONS

#define PATHCCH_NO_DEPRECATE

#include "../Helper/DisableUnaligned.h"

// Third-party Header Files:
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <cereal/archives/binary.hpp>
#include <sol/sol.hpp>

// Windows Header Files:
// clang-format off
#include <Winsock2.h>
#include <windows.h>
// clang-format on
#include <commctrl.h>
#include <commoncontrols.h>
#include <gdiplus.h>
#include <ocidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <strsafe.h>
#include <tchar.h>
#include <uxtheme.h>
#include <windowsx.h>

// The wrapper here will include Unknwn.h, which is why this appears after the rest of the Windows
// headers.
#include "../Helper/WinRTBaseWrapper.h"

// The boost headers can include Windows header files. If the boost header files are included before
// the Windows header files above, that can cause issues, specifically with winsock.h and
// Winsock2.h.
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/bimap.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/signals2.hpp>

// glog/logging.h includes windows.h, which specifically needs to be included after Winsock2.h
// (which is what's done above). Therefore, this is included after the Windows headers.
// GLOG_NO_ABBREVIATED_SEVERITIES is defined because windows.h defines ERROR, which is also normally
// defined by glog.
#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GLOG_USE_GLOG_EXPORT
#include <glog/logging.h>

// wil/resource.h can use declarations from the Windows header files. For example,
// wil::unique_htheme depends on uxtheme.h being included first. So, this file is specifically
// included after the Windows headers.
#include <wil/com.h>
#include <wil/resource.h>

// C++ Header Files:
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
