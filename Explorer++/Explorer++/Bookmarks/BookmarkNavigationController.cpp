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

void BookmarkNavigationController::Navigate(const BookmarkHistoryEntry *entry)
{
	auto bookmarkFolder = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree, entry->getGuid());

	if (!bookmarkFolder)
	{
		// This is valid. The folder might simply have been deleted since the original navigation
		// occurred.
		return;
	}

	Navigate(bookmarkFolder, entry);
}

void BookmarkNavigationController::Navigate(BookmarkItem *bookmarkFolder,
	const BookmarkHistoryEntry *entry)
{
	m_navigator->NavigateToBookmarkFolder(bookmarkFolder, entry);
}

void BookmarkNavigationController::OnNavigationCompleted(BookmarkItem *bookmarkFolder,
	const BookmarkHistoryEntry *entry)
{
	if (entry)
	{
		// Navigation is synchronous, so when navigating to a history entry, the entry should always
		// exist at this point.
		auto index = GetIndexOfEntry(entry);
		CHECK(index);
		SetCurrentIndex(*index);
	}
	else
	{
		AddEntry(std::make_unique<BookmarkHistoryEntry>(bookmarkFolder->GetGUID()));
	}
}

BookmarkHistoryEntry::BookmarkHistoryEntry(const std::wstring &guid) : m_guid(guid)
{
}

std::wstring BookmarkHistoryEntry::getGuid() const
{
	return m_guid;
}
