// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkListViewItem.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkColumnModel.h"
#include "Config.h"
#include "../Helper/Helper.h"
#include "../Helper/ScopedStopSource.h"

BookmarkListViewItem::BookmarkListViewItem(BookmarkItem *bookmarkItem,
	const BookmarkTree *bookmarkTree, BookmarkIconManager *bookmarkIconManager,
	const Config *config) :
	m_bookmarkItem(bookmarkItem),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkIconManager(bookmarkIconManager),
	m_config(config),
	m_scopedStopSource(std::make_unique<ScopedStopSource>())
{
	m_updateConnection = m_bookmarkItem->updatedSignal.AddObserver(
		std::bind_front(&BookmarkListViewItem::OnBookmarkItemUpdated, this));
}

BookmarkListViewItem::~BookmarkListViewItem() = default;

void BookmarkListViewItem::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem,
	BookmarkItem::PropertyType propertyType)
{
	UNREFERENCED_PARAMETER(bookmarkItem);

	if (m_bookmarkItem->IsBookmark() && propertyType == BookmarkItem::PropertyType::Location)
	{
		// This will stop any ongoing icon request.
		m_scopedStopSource = std::make_unique<ScopedStopSource>();

		m_iconIndex.reset();
	}

	NotifyUpdated();
}

std::wstring BookmarkListViewItem::GetColumnText(ListViewColumnId columnId) const
{
	return GetTextForColumn(BookmarkColumnModel::ColumnIdToBookmarkColumn(columnId));
}

std::wstring BookmarkListViewItem::GetTextForColumn(BookmarkColumn column) const
{
	switch (column)
	{
	case BookmarkColumn::Name:
		return m_bookmarkItem->GetName();

	case BookmarkColumn::Location:
		return m_bookmarkItem->IsBookmark() ? m_bookmarkItem->GetLocation() : L"";

	case BookmarkColumn::DateCreated:
		return FormatDate(m_bookmarkItem->GetDateCreated());

	case BookmarkColumn::DateModified:
		return FormatDate(m_bookmarkItem->GetDateModified());
	}

	LOG(FATAL) << "Bookmark column type not found";
}

std::wstring BookmarkListViewItem::FormatDate(const FILETIME &date) const
{
	wchar_t formattedDate[256];
	auto res = CreateFileTimeString(&date, formattedDate, std::size(formattedDate),
		m_config->globalFolderSettings.showFriendlyDates);

	if (!res)
	{
		return L"";
	}

	return formattedDate;
}

std::optional<int> BookmarkListViewItem::GetIconIndex() const
{
	if (!m_iconIndex)
	{
		m_iconIndex = m_bookmarkIconManager->GetBookmarkItemIconIndex(m_bookmarkItem,
			std::bind_front(&BookmarkListViewItem::OnIconAvailable,
				m_weakPtrFactory.GetMutableWeakPtr()),
			m_scopedStopSource->GetToken());
	}

	return m_iconIndex;
}

void BookmarkListViewItem::OnIconAvailable(WeakPtr<BookmarkListViewItem> weakSelf, int iconIndex)
{
	if (!weakSelf)
	{
		return;
	}

	weakSelf->m_iconIndex = iconIndex;
	weakSelf->NotifyUpdated();
}

bool BookmarkListViewItem::CanRename() const
{
	return !m_bookmarkTree->IsPermanentNode(m_bookmarkItem);
}

bool BookmarkListViewItem::CanRemove() const
{
	return !m_bookmarkTree->IsPermanentNode(m_bookmarkItem);
}

bool BookmarkListViewItem::IsFile() const
{
	return false;
}

const BookmarkItem *BookmarkListViewItem::GetBookmarkItem() const
{
	return m_bookmarkItem;
}

BookmarkItem *BookmarkListViewItem::GetBookmarkItem()
{
	return m_bookmarkItem;
}
