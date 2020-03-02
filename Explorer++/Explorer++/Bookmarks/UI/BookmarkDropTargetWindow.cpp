// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkDropTargetWindow.h"

BookmarkDropTargetWindow::BookmarkDropTargetWindow(HWND hwnd, BookmarkTree *bookmarkTree) :
	m_bookmarkTree(bookmarkTree)
{
	m_dropTarget = DropTarget::Create(hwnd, this);
}

DWORD BookmarkDropTargetWindow::DragEnter(
	IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	m_bookmarkDropInfo = std::make_unique<BookmarkDropInfo>(dataObject, m_bookmarkTree);

	auto dropLocation = GetDropLocation(pt);

	return m_bookmarkDropInfo->GetDropEffect(dropLocation.parentFolder);
}

DWORD BookmarkDropTargetWindow::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	if (m_previousDragOverPoint && pt.x == m_previousDragOverPoint->x
		&& pt.y == m_previousDragOverPoint->y)
	{
		return m_previousDropEffect;
	}

	m_previousDragOverPoint = pt;

	auto dropLocation = GetDropLocation(pt);

	if (m_previousDropLocation && m_previousDropLocation->parentFolder == dropLocation.parentFolder
		&& m_previousDropLocation->position == dropLocation.position
		&& m_previousDropLocation->parentFolderSelected == dropLocation.parentFolderSelected)
	{
		return m_previousDropEffect;
	}

	m_previousDropLocation = dropLocation;

	DWORD targetEffect = m_bookmarkDropInfo->GetDropEffect(dropLocation.parentFolder);

	if (targetEffect != DROPEFFECT_NONE)
	{
		UpdateUiForDropLocation(dropLocation);
	}
	else
	{
		ResetDropUiState();
	}

	m_previousDropEffect = targetEffect;

	return targetEffect;
}

void BookmarkDropTargetWindow::DragLeave()
{
	ResetDropState();
}

DWORD BookmarkDropTargetWindow::Drop(
	IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	auto dropLocation = GetDropLocation(pt);
	DWORD finalEffect =
		m_bookmarkDropInfo->PerformDrop(dropLocation.parentFolder, dropLocation.position);

	ResetDropState();

	return finalEffect;
}

void BookmarkDropTargetWindow::ResetDropState()
{
	ResetDropUiState();

	m_bookmarkDropInfo.reset();
	m_previousDragOverPoint.reset();
	m_previousDropLocation.reset();
}

bool BookmarkDropTargetWindow::IsWithinDrag() const
{
	return m_dropTarget->IsWithinDrag();
}