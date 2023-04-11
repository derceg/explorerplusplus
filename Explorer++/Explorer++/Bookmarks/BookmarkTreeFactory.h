// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>

class BookmarkTree;

// This class doesn't do much at the moment. But it could be updated to return different
// BookmarkTree instances if necessary (e.g. for different profiles).
class BookmarkTreeFactory
{
public:
	static BookmarkTreeFactory *GetInstance();

	BookmarkTree *GetBookmarkTree();

private:
	BookmarkTreeFactory() = default;
	~BookmarkTreeFactory();

	static inline BookmarkTreeFactory *m_staticInstance = nullptr;

	std::unique_ptr<BookmarkTree> m_bookmarkTree;
};
