// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkDropTargetDelegate.h"
#include "Bookmarks/BookmarkDropper.h"

BookmarkDropTargetDelegate::BookmarkDropTargetDelegate(BookmarkTree *bookmarkTree) :
	m_bookmarkTree(bookmarkTree),
	m_blockDrop(false)
{
}

BookmarkDropTargetDelegate::~BookmarkDropTargetDelegate() = default;

DWORD BookmarkDropTargetDelegate::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt,
	DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);

	m_bookmarkDropper = std::make_unique<BookmarkDropper>(dataObject, effect, m_bookmarkTree);
	m_bookmarkDropper->SetBlockDrop(m_blockDrop);

	auto dropLocation = GetDropLocation(pt);

	return m_bookmarkDropper->GetDropEffect(dropLocation.parentFolder, dropLocation.position);
}

DWORD BookmarkDropTargetDelegate::DragOver(DWORD keyState, POINT pt, DWORD effect)
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

	DWORD targetEffect =
		m_bookmarkDropper->GetDropEffect(dropLocation.parentFolder, dropLocation.position);

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

void BookmarkDropTargetDelegate::DragLeave()
{
	ResetDropState();
}

DWORD BookmarkDropTargetDelegate::Drop(IDataObject *dataObject, DWORD keyState, POINT pt,
	DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	auto dropLocation = GetDropLocation(pt);
	DWORD finalEffect =
		m_bookmarkDropper->PerformDrop(dropLocation.parentFolder, dropLocation.position);

	ResetDropState();

	return finalEffect;
}

void BookmarkDropTargetDelegate::ResetDropState()
{
	ResetDropUiState();

	m_bookmarkDropper.reset();
	m_previousDragOverPoint.reset();
	m_previousDropLocation.reset();
}

void BookmarkDropTargetDelegate::SetBlockDrop(bool blockDrop)
{
	if (m_bookmarkDropper)
	{
		m_bookmarkDropper->SetBlockDrop(blockDrop);
	}

	m_blockDrop = blockDrop;
}
