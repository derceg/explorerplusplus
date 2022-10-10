// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BookmarkItem;
class CoreInterface;
class Navigator;

class BookmarkMenuController
{
public:
	BookmarkMenuController(CoreInterface *coreInterface, Navigator *navigator);

	void OnBookmarkMenuItemSelected(const BookmarkItem *bookmarkItem);

private:
	CoreInterface *m_coreInterface;
	Navigator *m_navigator;
};
