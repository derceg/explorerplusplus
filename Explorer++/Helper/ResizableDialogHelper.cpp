// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResizableDialogHelper.h"
#include "WindowHelper.h"
#include <wil/common.h>

ResizableDialogHelper::ResizableDialogHelper(HWND dialog,
	const std::vector<ResizableDialogControl> &controls) :
	m_dialog(dialog)
{
	RECT dialogRect;
	[[maybe_unused]] auto dialogRectResult = GetClientRect(m_dialog, &dialogRect);
	assert(dialogRectResult);

	m_originalDialogWidth = GetRectWidth(&dialogRect);
	m_originalDialogHeight = GetRectHeight(&dialogRect);

	for (const auto &control : controls)
	{
		RECT controlRect;
		[[maybe_unused]] bool controlRectResult = GetControlRectInDialog(control.hwnd, controlRect);
		assert(controlRectResult);

		m_controlPositions.emplace_back(control, controlRect);
	}
}

void ResizableDialogHelper::UpdateControls(int updatedDialogWidth, int updatedDialogHeight)
{
	auto deferInfo = BeginDeferWindowPos(static_cast<int>(m_controlPositions.size()));

	// BeginDeferWindowPos() can technically fail (due to a lack of system resources), however that
	// probably isn't going to be very likely in practice, given the small amount of memory that may
	// need to be allocated. Therefore, if the function does fail, no fallback will be used (which
	// means that the control positions simply won't be updated).
	// The same will be true if any of the DeferWindowPos() calls fail.
	if (!deferInfo)
	{
		return;
	}

	for (const auto &controlPosition : m_controlPositions)
	{
		RECT controlRect;
		[[maybe_unused]] bool controlRectResult =
			GetControlRectInDialog(controlPosition.control.hwnd, controlRect);
		assert(controlRectResult);

		int x = controlRect.left;
		int y = controlRect.top;
		int width = GetRectWidth(&controlRect);
		int height = GetRectHeight(&controlRect);
		UINT flags = SWP_NOZORDER | SWP_NOREDRAW;

		// Control positions will be updated based on how much the size of the parent dialog has
		// changed since it was opened. For example, if the parent dialog is 10px wider, then a
		// control that's resized will become 10px wider as well, while a control that moves will be
		// moved 10px to the right.
		switch (controlPosition.control.movingType)
		{
		case MovingType::Both:
			x = controlPosition.originalRect.left + (updatedDialogWidth - m_originalDialogWidth);
			y = controlPosition.originalRect.top + (updatedDialogHeight - m_originalDialogHeight);
			break;

		case MovingType::Horizontal:
			x = controlPosition.originalRect.left + (updatedDialogWidth - m_originalDialogWidth);
			break;

		case MovingType::Vertical:
			y = controlPosition.originalRect.top + (updatedDialogHeight - m_originalDialogHeight);
			break;

		case MovingType::None:
			WI_SetFlag(flags, SWP_NOMOVE);
			break;
		}

		switch (controlPosition.control.sizingType)
		{
		case SizingType::Both:
			width = GetRectWidth(&controlPosition.originalRect)
				+ (updatedDialogWidth - m_originalDialogWidth);
			height = GetRectHeight(&controlPosition.originalRect)
				+ (updatedDialogHeight - m_originalDialogHeight);
			break;

		case SizingType::Horizontal:
			width = GetRectWidth(&controlPosition.originalRect)
				+ (updatedDialogWidth - m_originalDialogWidth);
			break;

		case SizingType::Vertical:
			height = GetRectHeight(&controlPosition.originalRect)
				+ (updatedDialogHeight - m_originalDialogHeight);
			break;

		case SizingType::None:
			WI_SetFlag(flags, SWP_NOSIZE);
			break;
		}

		deferInfo = DeferWindowPos(deferInfo, controlPosition.control.hwnd, nullptr, x, y, width,
			height, flags);

		// As indicated by the documentation for DeferWindowPos(), EndDeferWindowPos() shouldn't be
		// called if the call to DeferWindowPos() fails.
		if (!deferInfo)
		{
			return;
		}
	}

	[[maybe_unused]] auto res = EndDeferWindowPos(deferInfo);
	assert(res);

	// Redrawing the entire window once the child controls have been repositioned appears to be a
	// better option than redrawing as part of the deferred window updates, since there seems to be
	// times when controls aren't redrawn properly. For example, multiline static controls don't get
	// drawn properly after a size change.
	RedrawWindow(m_dialog, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
}

bool ResizableDialogHelper::GetControlRectInDialog(HWND control, RECT &outputRect)
{
	RECT controlRect;
	BOOL windowRectResult = GetWindowRect(control, &controlRect);

	if (!windowRectResult)
	{
		return false;
	}

	int mapWindowPointsResult = MapWindowPoints(HWND_DESKTOP, m_dialog,
		reinterpret_cast<LPPOINT>(&controlRect), sizeof(RECT) / sizeof(POINT));

	if (mapWindowPointsResult == 0)
	{
		return false;
	}

	outputRect = controlRect;

	return true;
}
