// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkTreeFactory.h"
#include "Bookmarks/BookmarkTree.h"

BookmarkTreeFactory *BookmarkTreeFactory::m_staticInstance = nullptr;

BookmarkTreeFactory *BookmarkTreeFactory::GetInstance()
{
	if (!m_staticInstance)
	{
		// This instance is designed to live for the lifetime of the application and is deliberately
		// leaked.
		m_staticInstance = new BookmarkTreeFactory();
	}

	return m_staticInstance;
}

BookmarkTreeFactory::~BookmarkTreeFactory() = default;

BookmarkTree *BookmarkTreeFactory::GetBookmarkTree()
{
	if (!m_bookmarkTree)
	{
		m_bookmarkTree = std::make_unique<BookmarkTree>();
	}

	return m_bookmarkTree.get();
}
