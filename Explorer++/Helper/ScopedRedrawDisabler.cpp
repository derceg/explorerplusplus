// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ScopedRedrawDisabler.h"

ScopedRedrawDisabler::ScopedRedrawDisabler(HWND hwnd) : m_hwnd(hwnd)
{
	SendMessage(m_hwnd, WM_SETREDRAW, false, 0);
}

ScopedRedrawDisabler::~ScopedRedrawDisabler()
{
	SendMessage(m_hwnd, WM_SETREDRAW, true, 0);
}
