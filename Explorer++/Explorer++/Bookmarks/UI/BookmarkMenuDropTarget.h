// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <winrt/base.h>
#include <oleidl.h>

class BookmarkDropper;
class BookmarkItem;
class BookmarkTree;

// When dragging over a bookmark menu, an IDropTarget instance will be requested for the current
// position. This class represents a drop target at a fixed position in a parent folder.
class BookmarkMenuDropTarget : public winrt::implements<BookmarkMenuDropTarget, IDropTarget>
{
public:
	BookmarkMenuDropTarget(BookmarkItem *targetFolder, size_t targetIndex,
		BookmarkTree *bookmarkTree);
	~BookmarkMenuDropTarget();

	// IDropTarget
	IFACEMETHODIMP DragEnter(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragOver(DWORD keyState, POINTL ptl, DWORD *effect);
	IFACEMETHODIMP DragLeave();
	IFACEMETHODIMP Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect);

private:
	BookmarkItem *m_targetFolder;
	size_t m_targetIndex;
	BookmarkTree *m_bookmarkTree;
	std::unique_ptr<BookmarkDropper> m_bookmarkDropper;
};
