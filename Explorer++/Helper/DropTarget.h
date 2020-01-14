// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"
#include "WindowSubclassWrapper.h"
#include <wil/com.h>

class DropTargetInternal
{
public:

	virtual DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) = 0;
	virtual DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) = 0;
	virtual void DragLeave() = 0;
	virtual DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) = 0;
};

class DropTarget : public IDropTarget
{
public:

	static wil::com_ptr<DropTarget> Create(HWND hwnd, DropTargetInternal *dropTargetInternal);

	// IUnknown methods.
	IFACEMETHODIMP QueryInterface(REFIID iid, void **object);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IDropTarget methods.
	IFACEMETHODIMP DragEnter(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragOver(DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragLeave();
	IFACEMETHODIMP Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);

	bool IsWithinDrag() const;

private:

	DISALLOW_COPY_AND_ASSIGN(DropTarget);

	static inline const UINT_PTR SUBCLASS_ID = 0;

	DropTarget(HWND hwnd, DropTargetInternal *dropTargetInternal);
	~DropTarget() = default;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);

	static IDropTargetHelper *GetDropTargetHelper();

	static inline IDropTargetHelper *m_dropTargetHelper = nullptr;

	HWND m_hwnd;
	DropTargetInternal *m_dropTargetInternal;
	long m_refCount;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;

	bool m_withinDrag;
};