// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkNavigationController.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkNavigatorInterface.h"

BookmarkNavigationController::BookmarkNavigationController(BookmarkTree *bookmarkTree,
	BookmarkNavigatorInterface *navigator) :
	m_bookmarkTree(bookmarkTree),
	m_navigator(navigator)
{
	m_connections.push_back(m_navigator->AddNavigationCompletedObserver(
		std::bind_front(&BookmarkNavigationController::OnNavigationCompleted, this),
		boost::signals2::at_front));
}

bool BookmarkNavigationController::Navigate(const BookmarkHistoryEntry *entry)
{
	auto bookmarkFolder = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree, entry->getGuid());

	if (!bookmarkFolder)
	{
		// This is valid. The folder might simply have been deleted since it was
		// last navigated to.
		return false;
	}

	Navigate(bookmarkFolder);
	return true;
}

bool BookmarkNavigationController::Navigate(BookmarkItem *bookmarkFolder, bool addHistoryEntry)
{
	m_navigator->NavigateToBookmarkFolder(bookmarkFolder, addHistoryEntry);
	return true;
}

bool BookmarkNavigationController::GetFailureValue()
{
	return false;
}

void BookmarkNavigationController::OnNavigationCompleted(BookmarkItem *bookmarkFolder,
	bool addHistoryEntry)
{
	if (addHistoryEntry)
	{
		auto entry = std::make_unique<BookmarkHistoryEntry>(bookmarkFolder->GetGUID());
		AddEntry(std::move(entry));
	}
}

BookmarkHistoryEntry::BookmarkHistoryEntry(const std::wstring &guid) : m_guid(guid)
{
}

std::wstring BookmarkHistoryEntry::getGuid() const
{
	return m_guid;
}
