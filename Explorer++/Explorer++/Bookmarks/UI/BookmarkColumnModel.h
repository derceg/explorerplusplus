// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkColumn.h"
#include "ListViewColumnModel.h"
#include <vector>

class BookmarkColumnModel : public ListViewColumnModel
{
public:
	BookmarkColumnModel();

	static ListViewColumnId BookmarkColumnToColumnId(BookmarkColumn column);
	static BookmarkColumn ColumnIdToBookmarkColumn(ListViewColumnId columnId);

private:
	static constexpr int DEFAULT_COLUMN_WIDTH = 180;

	static std::vector<ListViewColumn> BuildColumnSet();
};
