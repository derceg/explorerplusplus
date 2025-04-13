// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

namespace MessageWindowHelper
{

inline constexpr wchar_t MESSAGE_CLASS_NAME[] = L"MessageWindowClass";

wil::unique_hwnd CreateMessageOnlyWindow(const std::wstring &windowName = L"");

}
