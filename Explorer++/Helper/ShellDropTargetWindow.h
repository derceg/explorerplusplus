// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DropTargetWindow.h"
#include "ShellHelper.h"
#include <wil/com.h>
#include <optional>

template <typename DropTargetItemIdentifierType>
class ShellDropTargetWindow : private DropTargetInternal
{
public:
	HWND GetHWND() const;
	bool IsWithinDrag() const;

protected:
	ShellDropTargetWindow(HWND hwnd);
	~ShellDropTargetWindow() = default;

	virtual DropTargetItemIdentifierType GetDropTargetItem(const POINT &pt) = 0;
	virtual unique_pidl_absolute GetPidlForTargetItem(DropTargetItemIdentifierType targetItem) = 0;
	virtual IUnknown *GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl) = 0;
	virtual bool IsTargetSourceOfDrop(
		DropTargetItemIdentifierType targetItem, IDataObject *dataObject) = 0;
	virtual void UpdateUiForDrop(DropTargetItemIdentifierType targetItem, const POINT &pt) = 0;
	virtual void ResetDropUiState() = 0;

	wil::com_ptr_nothrow<IDropTarget> GetDropTargetForPidl(PCIDLIST_ABSOLUTE pidl);

private:
	struct DropTargetInfo
	{
		DropTargetItemIdentifierType item;
		wil::com_ptr_nothrow<IDropTarget> dropTarget;
		bool dropTargetInitialised;
	};

	// DropTargetInternal methods.
	DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;
	DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) override;
	void DragLeave() override;
	DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;

	DWORD OnDragInWindow(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect);
	DWORD GetDropEffect(DropTargetItemIdentifierType targetItem, IDataObject *dataObject,
		DWORD keyState, POINT pt, DWORD allowedEffects);
	DropTargetInfo GetDropTargetInfoForItem(DropTargetItemIdentifierType targetItem);
	DWORD PerformDrop(DropTargetItemIdentifierType targetItem, IDataObject *dataObject,
		DWORD previousKeyState, DWORD keyState, POINT pt, DWORD allowedEffects);
	void ResetDropState();

	wil::com_ptr_nothrow<DropTargetWindow> m_dropTargetWindow;
	IDataObject *m_currentDropObject;
	DWORD m_previousKeyState;
	std::optional<DropTargetInfo> m_previousTargetInfo;

	HWND m_hwnd;
};