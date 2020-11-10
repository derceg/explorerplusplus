// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkDropper.h"
#include "../Helper/DropTarget.h"
#include <wil/com.h>
#include <optional>

class BookmarkDropTargetWindow : private DropTargetInternal
{
protected:
	struct DropLocation
	{
		BookmarkItem *parentFolder;
		size_t position;
		bool parentFolderSelected;
	};

	BookmarkDropTargetWindow(HWND hwnd, BookmarkTree *bookmarkTree);
	~BookmarkDropTargetWindow() = default;

	virtual DropLocation GetDropLocation(const POINT &pt) = 0;
	virtual void UpdateUiForDropLocation(const DropLocation &dropLocation) = 0;
	virtual void ResetDropUiState() = 0;

	void SetBlockDrop(bool blockDrop);
	bool IsWithinDrag() const;

private:
	// DropTargetInternal methods.
	DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;
	DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) override;
	void DragLeave() override;
	DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;

	void ResetDropState();

	BookmarkTree *m_bookmarkTree;

	wil::com_ptr_nothrow<DropTarget> m_dropTarget;
	std::unique_ptr<BookmarkDropper> m_bookmarkDropper;
	std::optional<POINT> m_previousDragOverPoint;
	std::optional<DropLocation> m_previousDropLocation;
	DWORD m_previousDropEffect;
	bool m_blockDrop;
};