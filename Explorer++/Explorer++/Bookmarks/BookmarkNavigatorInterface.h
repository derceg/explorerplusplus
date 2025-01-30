// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

class BookmarkHistoryEntry;
class BookmarkItem;

using BookmarkNavigationCompletedSignal =
	boost::signals2::signal<void(BookmarkItem *bookmarkFolder, const BookmarkHistoryEntry *entry)>;

class BookmarkNavigatorInterface
{
public:
	virtual ~BookmarkNavigatorInterface() = default;

	virtual void NavigateToBookmarkFolder(BookmarkItem *bookmarkFolder,
		const BookmarkHistoryEntry *entry = nullptr) = 0;
	virtual boost::signals2::connection AddNavigationCompletedObserver(
		const BookmarkNavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;
};
