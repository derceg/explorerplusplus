// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BookmarkItem;
class CoreInterface;

class BookmarkMenuController
{
public:
	BookmarkMenuController(CoreInterface *coreInterface);

	void OnBookmarkMenuItemSelected(const BookmarkItem *bookmarkItem);

private:
	CoreInterface *m_coreInterface;
};
