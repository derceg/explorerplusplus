// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DropTargetWindow.h"
#include "ShellHelper.h"
#include "WinRTBaseWrapper.h"
#include <wil/com.h>
#include <optional>

template <typename DropTargetItemIdentifierType>
class ShellDropTargetWindow : private DropTargetInternal
{
public:
	HWND GetHWND() const;

protected:
	ShellDropTargetWindow(HWND hwnd);
	~ShellDropTargetWindow() = default;

	wil::com_ptr_nothrow<IDropTarget> GetDropTargetForPidl(PCIDLIST_ABSOLUTE pidl);

	const HWND m_hwnd;

private:
	enum class DropType
	{
		LeftClick,
		RightClick
	};

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

	virtual DropTargetItemIdentifierType GetDropTargetItem(const POINT &pt) = 0;
	virtual unique_pidl_absolute GetPidlForTargetItem(DropTargetItemIdentifierType targetItem) = 0;
	virtual IUnknown *GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl) = 0;

	// This will be used to detect whether the target directory is the source directory (to ensure
	// that an item dragged and dropped within a single directory doesn't get moved or copied).
	virtual bool IsTargetSourceOfDrop(DropTargetItemIdentifierType targetItem) = 0;

	virtual void UpdateUiForDrop(DropTargetItemIdentifierType targetItem, const POINT &pt) = 0;
	virtual void ResetDropUiState() = 0;

	DWORD OnDragInWindow(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect);
	DWORD GetDropEffect(DropTargetItemIdentifierType targetItem, IDataObject *dataObject,
		DWORD keyState, POINT pt, DWORD allowedEffects);
	DropTargetInfo GetDropTargetInfoForItem(DropTargetItemIdentifierType targetItem);
	DWORD PerformDrop(DropTargetItemIdentifierType targetItem, IDataObject *dataObject,
		DWORD previousKeyState, DWORD keyState, POINT pt, DWORD allowedEffects);
	void ResetDropState();

	winrt::com_ptr<DropTargetWindow> m_dropTargetWindow;
	IDataObject *m_currentDropObject;
	DropType m_dropType;
	DWORD m_previousKeyState;
	std::optional<DropTargetInfo> m_previousTargetInfo;
};
