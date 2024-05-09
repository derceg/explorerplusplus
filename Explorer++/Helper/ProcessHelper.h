// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>

DWORD GetProcessImageName(DWORD dwProcessId, TCHAR *szImageName, DWORD nSize);
BOOL GetProcessOwner(DWORD dwProcessId, TCHAR *szOwner, size_t cchMax);
bool IsProcessElevated();
