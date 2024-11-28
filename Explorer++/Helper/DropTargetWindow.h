// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"
#include "WindowSubclass.h"

class DropTargetInternal
{
public:
	virtual ~DropTargetInternal() = default;

	virtual DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) = 0;
	virtual DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) = 0;
	virtual void DragLeave() = 0;
	virtual DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) = 0;
};

class DropTargetWindow : public winrt::implements<DropTargetWindow, IDropTarget, winrt::non_agile>
{
public:
	DropTargetWindow(HWND hwnd, DropTargetInternal *dropTargetInternal);

	// IDropTarget methods.
	IFACEMETHODIMP DragEnter(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragOver(DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragLeave();
	IFACEMETHODIMP Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);

	bool IsWithinDrag() const;

private:
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static IDropTargetHelper *GetDropTargetHelper();

	static inline IDropTargetHelper *m_dropTargetHelper = nullptr;

	HWND m_hwnd;
	DropTargetInternal *m_dropTargetInternal;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	bool m_withinDrag;
};
