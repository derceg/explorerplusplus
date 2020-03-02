// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkNavigationController.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkNavigatorInterface.h"

BookmarkNavigationController::BookmarkNavigationController(
	BookmarkTree *bookmarkTree, BookmarkNavigatorInterface *navigator) :
	m_bookmarkTree(bookmarkTree),
	m_navigator(navigator)
{
	m_connections.push_back(m_navigator->AddNavigationCompletedObserver(
		std::bind(&BookmarkNavigationController::OnNavigationCompleted, this, std::placeholders::_1,
			std::placeholders::_2),
		boost::signals2::at_front));
}

bool BookmarkNavigationController::BrowseFolder(
	const BookmarkHistoryEntry *entry, bool addHistoryEntry)
{
	auto bookmarkFolder = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree, entry->getGuid());

	if (!bookmarkFolder)
	{
		// This is valid. The folder might simply have been deleted since it was
		// last navigated to.
		return false;
	}

	BrowseFolder(bookmarkFolder, addHistoryEntry);
	return true;
}

bool BookmarkNavigationController::BrowseFolder(BookmarkItem *bookmarkFolder, bool addHistoryEntry)
{
	m_navigator->NavigateToBookmarkFolder(bookmarkFolder, addHistoryEntry);
	return true;
}

bool BookmarkNavigationController::GetFailureValue()
{
	return false;
}

void BookmarkNavigationController::OnNavigationCompleted(
	BookmarkItem *bookmarkFolder, bool addHistoryEntry)
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