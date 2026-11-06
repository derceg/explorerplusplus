// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ControlTestHelper.h"

void SimulateClick(HWND hwnd, const POINT &ptClient)
{
	SimulateClick(hwnd, ptClient, ptClient);
}

void SimulateClick(HWND hwnd, const POINT &ptClientDown, const POINT &ptClientUp)
{
	SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(ptClientDown.x, ptClientDown.y));
	SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(ptClientUp.x, ptClientUp.y));
}
