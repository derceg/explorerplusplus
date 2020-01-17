// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkDropTargetWindow.h"

BookmarkDropTargetWindow::BookmarkDropTargetWindow(HWND hwnd, BookmarkTree *bookmarkTree) :
	m_bookmarkTree(bookmarkTree)
{
	m_dropTarget = DropTarget::Create(hwnd, this);
}

DWORD BookmarkDropTargetWindow::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	m_bookmarkDropInfo = std::make_unique<BookmarkDropInfo>(dataObject, m_bookmarkTree);

	auto dropTarget = GetDropLocation(pt);

	return m_bookmarkDropInfo->GetDropEffect(dropTarget.parentFolder);
}

DWORD BookmarkDropTargetWindow::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	if (m_previousDragOverPoint
		&& pt.x == m_previousDragOverPoint->x
		&& pt.y == m_previousDragOverPoint->y)
	{
		return m_previousDropEffect;
	}

	m_previousDragOverPoint = pt;

	auto dropTarget = GetDropLocation(pt);

	if (m_previousDropLocation
		&& m_previousDropLocation->parentFolder == dropTarget.parentFolder
		&& m_previousDropLocation->position == dropTarget.position
		&& m_previousDropLocation->parentFolderSelected == dropTarget.parentFolderSelected)
	{
		return m_previousDropEffect;
	}

	m_previousDropLocation = dropTarget;

	DWORD targetEffect = m_bookmarkDropInfo->GetDropEffect(dropTarget.parentFolder);

	if (targetEffect != DROPEFFECT_NONE)
	{
		UpdateUiForDropLocation(dropTarget);
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

DWORD BookmarkDropTargetWindow::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	auto dropTarget = GetDropLocation(pt);
	DWORD finalEffect = m_bookmarkDropInfo->PerformDrop(dropTarget.parentFolder, dropTarget.position);

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