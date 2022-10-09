// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkMenuDropTarget.h"
#include "Bookmarks/BookmarkDropper.h"

BookmarkMenuDropTarget::BookmarkMenuDropTarget(BookmarkItem *targetFolder, size_t targetIndex,
	BookmarkTree *bookmarkTree) :
	m_targetFolder(targetFolder),
	m_targetIndex(targetIndex),
	m_bookmarkTree(bookmarkTree)
{
}

BookmarkMenuDropTarget::~BookmarkMenuDropTarget() = default;

// IDropTarget
IFACEMETHODIMP BookmarkMenuDropTarget::DragEnter(IDataObject *dataObject, DWORD keyState,
	POINTL ptl, DWORD *effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(ptl);

	m_bookmarkDropper = std::make_unique<BookmarkDropper>(dataObject, *effect, m_bookmarkTree);

	*effect = m_bookmarkDropper->GetDropEffect(m_targetFolder, m_targetIndex);
	return S_OK;
}

IFACEMETHODIMP BookmarkMenuDropTarget::DragOver(DWORD keyState, POINTL ptl, DWORD *effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(ptl);

	*effect = m_bookmarkDropper->GetDropEffect(m_targetFolder, m_targetIndex);
	return S_OK;
}

IFACEMETHODIMP BookmarkMenuDropTarget::DragLeave()
{
	return S_OK;
}

IFACEMETHODIMP BookmarkMenuDropTarget::Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl,
	DWORD *effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(ptl);

	*effect = m_bookmarkDropper->PerformDrop(m_targetFolder, m_targetIndex);

	return S_OK;
}
