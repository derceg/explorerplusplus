// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "NavigationController.h"
#include <boost/core/noncopyable.hpp>

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

class BookmarkNavigationController :
	public NavigationController<BookmarkHistoryEntry, bool>,
	private boost::noncopyable
{
public:
	BookmarkNavigationController(BookmarkTree *bookmarkTree, BookmarkNavigatorInterface *navigator);

	bool Navigate(BookmarkItem *bookmarkFolder, bool addHistoryEntry = true);

private:
	bool Navigate(const BookmarkHistoryEntry *entry) override;
	bool GetFailureValue() override;

	void OnNavigationCompleted(BookmarkItem *bookmarkFolder, bool addHistoryEntry);

	BookmarkTree *m_bookmarkTree;
	BookmarkNavigatorInterface *m_navigator;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
