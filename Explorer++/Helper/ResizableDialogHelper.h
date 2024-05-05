// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <vector>

enum class MovingType
{
	None,
	Horizontal,
	Vertical,
	Both
};

enum class SizingType
{
	None,
	Horizontal,
	Vertical,
	Both
};

struct ResizableDialogControl
{
	HWND hwnd;
	MovingType movingType;
	SizingType sizingType;

	ResizableDialogControl(HWND hwnd, MovingType movingType, SizingType sizingType) :
		hwnd(hwnd),
		movingType(movingType),
		sizingType(sizingType)
	{
	}
};

// Within a resizable dialog, controls either: move, resize, or hold the same size and position. For
// controls that do move/resize, they may be constrained along one axis. For example, a particular
// control may resize/move horizontally, but not vertically.
class ResizableDialogHelper : private boost::noncopyable
{
public:
	ResizableDialogHelper(HWND dialog, const std::vector<ResizableDialogControl> &controls);

	void UpdateControls(int updatedDialogWidth, int updatedDialogHeight);

private:
	struct ControlPosition
	{
		ResizableDialogControl control;
		RECT originalRect;

		ControlPosition(const ResizableDialogControl &control, const RECT &originalRect) :
			control(control),
			originalRect(originalRect)
		{
		}
	};

	bool GetControlRectInDialog(HWND control, RECT &outputRect);

	const HWND m_dialog;
	std::vector<ControlPosition> m_controlPositions;

	int m_originalDialogWidth;
	int m_originalDialogHeight;
};
