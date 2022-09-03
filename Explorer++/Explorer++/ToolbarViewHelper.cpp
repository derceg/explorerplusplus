// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarViewHelper.h"

// TODO: If BookmarksToolbar is updated to use ToolbarView, this function should be moved into
// ToolbarView.
// Returns the index of the button that comes after the specified point. If the point
// is past the last button on the toolbar, this index will be one past the last button (or 0 if
// there are no buttons).
int FindNextButtonIndex(HWND hwnd, const POINT &ptClient)
{
	int numButtons = static_cast<int>(SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0));
	int nextIndex = 0;

	for (int i = 0; i < numButtons; i++)
	{
		RECT rc;
		SendMessage(hwnd, TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&rc));

		if (ptClient.x < rc.right)
		{
			break;
		}

		nextIndex = i + 1;
	}

	return nextIndex;
}
