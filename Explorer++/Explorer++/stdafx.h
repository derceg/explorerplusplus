// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

#define STRICT

#define ISOLATION_AWARE_ENABLED	1

#define OEMRESOURCE

// Windows Header Files:
#include <Winsock2.h>
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
#include <shellapi.h>
#include <prsht.h>
#include <math.h>
#include <richedit.h>
#include <objidl.h>
#include <shlguid.h>
#include <strsafe.h>
#include <psapi.h>
#include <uxtheme.h>
#include <wmsdk.h>
#include <vfw.h>
#include <dwmapi.h>
#include <WinInet.h>
#include <VersionHelpers.h>
#include <Imagehlp.h>

#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/log/common.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/parameter.hpp>
#include <boost/signals2.hpp>
#include <wil/resource.h>

#include "../ThirdParty/cereal/archives/binary.hpp"
#include "../ThirdParty/cereal/types/memory.hpp"
#include "../ThirdParty/cereal/types/string.hpp"
#include "../ThirdParty/cereal/types/vector.hpp"

#include "../ThirdParty/CTPL/cpl_stl.h"

#include "SolWrapper.h"