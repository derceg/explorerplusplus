// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"

class DropTargetDelegate;
class WindowDropTarget;

class ScopedDropTargetRegistration
{
public:
	ScopedDropTargetRegistration(HWND hwnd, DropTargetDelegate *delegate);
	~ScopedDropTargetRegistration();

private:
	const HWND m_hwnd;
	winrt::com_ptr<WindowDropTarget> m_dropTarget;
};
