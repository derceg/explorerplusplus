// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <string>

std::wstring GetCurrentProcessPath();
BOOL GetProcessOwner(DWORD dwProcessId, TCHAR *szOwner, size_t cchMax);
bool IsProcessElevated();
