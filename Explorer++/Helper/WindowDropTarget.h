// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"
#include <ShObjIdl.h>

class DropTargetDelegate;

class WindowDropTarget : public winrt::implements<WindowDropTarget, IDropTarget, winrt::non_agile>
{
public:
	WindowDropTarget(HWND hwnd, DropTargetDelegate *delegate);

	// IDropTarget
	IFACEMETHODIMP DragEnter(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragOver(DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragLeave();
	IFACEMETHODIMP Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);

private:
	static IDropTargetHelper *GetDropTargetHelper();

	static inline IDropTargetHelper *m_dropTargetHelper = nullptr;

	const HWND m_hwnd;
	DropTargetDelegate *const m_delegate;
};
