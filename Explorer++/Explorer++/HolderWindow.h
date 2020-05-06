// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#define WM_USER_HOLDERRESIZED WM_APP + 300

HWND CreateHolderWindow(HWND hParent, TCHAR *szWindowName, UINT uStyle);