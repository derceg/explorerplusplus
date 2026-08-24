// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkColumnModel.h"
#include "MainResource.h"

BookmarkColumnModel::BookmarkColumnModel() :
	ListViewColumnModel(BuildColumnSet(), BookmarkColumnToColumnId(BookmarkColumn::Name))
{
}

std::vector<ListViewColumn> BookmarkColumnModel::BuildColumnSet()
{
	std::vector<ListViewColumn> columns;

	auto addColumn = [&columns](BookmarkColumn bookmarkColumn, bool visible)
	{
		columns.emplace_back(BookmarkColumnToColumnId(bookmarkColumn),
			GetBookmarkColumnStringId(bookmarkColumn), DEFAULT_COLUMN_WIDTH, visible);
	};

	addColumn(BookmarkColumn::Name, true);
	addColumn(BookmarkColumn::Location, true);
	addColumn(BookmarkColumn::DateCreated, false);
	addColumn(BookmarkColumn::DateModified, false);

	return columns;
}

UINT BookmarkColumnModel::GetBookmarkColumnStringId(BookmarkColumn column)
{
	switch (column)
	{
	case BookmarkColumn::Name:
		return IDS_BOOKMARKS_COLUMN_NAME;

	case BookmarkColumn::Location:
		return IDS_BOOKMARKS_COLUMN_LOCATION;

	case BookmarkColumn::DateCreated:
		return IDS_BOOKMARKS_COLUMN_DATE_CREATED;

	case BookmarkColumn::DateModified:
		return IDS_BOOKMARKS_COLUMN_DATE_MODIFIED;
	}

	LOG(FATAL) << "Invalid BookmarkColumn value";
}

ListViewColumnId BookmarkColumnModel::BookmarkColumnToColumnId(BookmarkColumn column)
{
	return ListViewColumnId(static_cast<int>(column));
}

BookmarkColumn BookmarkColumnModel::ColumnIdToBookmarkColumn(ListViewColumnId columnId)
{
	auto column = BookmarkColumn::_from_integral_nothrow(columnId.value);
	CHECK(column);
	return *column;
}
