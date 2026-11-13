// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/DropTargetDelegate.h"
#include <optional>

class BookmarkDropper;
class BookmarkItem;
class BookmarkTree;

class BookmarkDropTargetDelegate : public DropTargetDelegate
{
protected:
	struct DropLocation
	{
		BookmarkItem *parentFolder;
		size_t position;
		bool parentFolderSelected;
	};

	BookmarkDropTargetDelegate(BookmarkTree *bookmarkTree);
	~BookmarkDropTargetDelegate();

	// DropTargetDelegate
	DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;
	DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) override;
	void DragLeave() override;
	DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;

	virtual DropLocation GetDropLocation(const POINT &pt) = 0;
	virtual void UpdateUiForDropLocation(const DropLocation &dropLocation) = 0;
	virtual void ResetDropUiState() = 0;

	void SetBlockDrop(bool blockDrop);

private:
	void ResetDropState();

	BookmarkTree *const m_bookmarkTree;

	std::unique_ptr<BookmarkDropper> m_bookmarkDropper;
	std::optional<POINT> m_previousDragOverPoint;
	std::optional<DropLocation> m_previousDropLocation;
	DWORD m_previousDropEffect;
	bool m_blockDrop;
};
