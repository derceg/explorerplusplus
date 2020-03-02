// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "NavigationController.h"
#include "../Helper/Macros.h"

class BookmarkItem;
class BookmarkNavigatorInterface;
class BookmarkTree;

class BookmarkHistoryEntry
{
public:
	BookmarkHistoryEntry(const std::wstring &guid);

	std::wstring getGuid() const;

private:
	std::wstring m_guid;
};

class BookmarkNavigationController : public NavigationController<BookmarkHistoryEntry, bool>
{
public:
	BookmarkNavigationController(BookmarkTree *bookmarkTree, BookmarkNavigatorInterface *navigator);

	bool BrowseFolder(BookmarkItem *bookmarkFolder, bool addHistoryEntry = true);

private:
	DISALLOW_COPY_AND_ASSIGN(BookmarkNavigationController);

	bool BrowseFolder(const BookmarkHistoryEntry *entry, bool addHistoryEntry = true) override;
	bool GetFailureValue() override;

	void OnNavigationCompleted(BookmarkItem *bookmarkFolder, bool addHistoryEntry);

	BookmarkTree *m_bookmarkTree;
	BookmarkNavigatorInterface *m_navigator;

	std::vector<boost::signals2::scoped_connection> m_connections;
};