// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class MenuDelegate
{
public:
	virtual ~MenuDelegate() = default;

	virtual void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) = 0;

	virtual void OnItemMiddleClicked(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
	{
		UNREFERENCED_PARAMETER(id);
		UNREFERENCED_PARAMETER(isCtrlKeyDown);
		UNREFERENCED_PARAMETER(isShiftKeyDown);
	}
};
