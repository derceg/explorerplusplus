// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkHelper.h"
#include "AddBookmarkDialog.h"
#include "MainResource.h"
#include "../Helper/Macros.h"
#include <algorithm>

int CALLBACK SortByName(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByLocation(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByDateAdded(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByDateModified(const BookmarkItem *firstItem, const BookmarkItem *secondItem);

bool NBookmarkHelper::IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsFolder();
}

bool NBookmarkHelper::IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsBookmark();
}

int CALLBACK NBookmarkHelper::Sort(SortMode_t SortMode, const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	if(firstItem->GetType() == BookmarkItem::Type::Folder &&
		secondItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		return -1;
	}
	else if(firstItem->GetType() == BookmarkItem::Type::Bookmark &&
		secondItem->GetType() == BookmarkItem::Type::Folder)
	{
		return 1;
	}
	else
	{
		int iRes = 0;

		switch(SortMode)
		{
		case SM_NAME:
			iRes = SortByName(firstItem,secondItem);
			break;

		case SM_LOCATION:
			iRes = SortByLocation(firstItem,secondItem);
			break;

		case SM_DATE_ADDED:
			iRes = SortByDateAdded(firstItem,secondItem);
			break;

		case SM_DATE_MODIFIED:
			iRes = SortByDateModified(firstItem,secondItem);
			break;

		default:
			assert(false);
			break;
		}

		return iRes;
	}
}

int CALLBACK SortByName(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	return firstItem->GetName().compare(secondItem->GetName());
}

int CALLBACK SortByLocation(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	if(firstItem->GetType() == BookmarkItem::Type::Folder &&
		secondItem->GetType() == BookmarkItem::Type::Folder)
	{
		return 0;
	}
	else if (firstItem->GetType() == BookmarkItem::Type::Folder &&
		secondItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		return -1;
	}
	else if (firstItem->GetType() == BookmarkItem::Type::Bookmark &&
		secondItem->GetType() == BookmarkItem::Type::Folder)
	{
		return 1;
	}
	else
	{
		return firstItem->GetLocation().compare(secondItem->GetLocation());
	}
}

int CALLBACK SortByDateAdded(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	FILETIME firstItemDateCreated = firstItem->GetDateCreated();
	FILETIME secondItemDateCreated = secondItem->GetDateCreated();
	return CompareFileTime(&firstItemDateCreated, &secondItemDateCreated);
}

int CALLBACK SortByDateModified(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	FILETIME firstItemDateModified = firstItem->GetDateModified();
	FILETIME secondItemDateModified = secondItem->GetDateModified();
	return CompareFileTime(&firstItemDateModified, &secondItemDateModified);
}

void NBookmarkHelper::AddBookmark(BookmarkTree *bookmarkTree, HMODULE resoureceModule,
	HWND parentWindow, TabContainer *tabContainer, IExplorerplusplus *coreInterface)
{
	const Tab &selectedTab = tabContainer->GetSelectedTab();
	auto entry = selectedTab.GetShellBrowser()->GetNavigationController()->GetCurrentEntry();

	auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, entry->GetDisplayName(),
		selectedTab.GetShellBrowser()->GetDirectory());
	BookmarkItem *selectedParentFolder = nullptr;

	CAddBookmarkDialog AddBookmarkDialog(resoureceModule, parentWindow, coreInterface,
		bookmarkTree, bookmarkItem.get(), &selectedParentFolder);
	auto res = AddBookmarkDialog.ShowModalDialog();

	if (res == CBaseDialog::RETURN_OK)
	{
		assert(selectedParentFolder != nullptr);
		bookmarkTree->AddBookmarkItem(selectedParentFolder, std::move(bookmarkItem),
			selectedParentFolder->GetChildren().size());
	}
}

void NBookmarkHelper::EditBookmark(BookmarkItem *bookmarkItem, BookmarkTree *bookmarkTree,
	HMODULE resoureceModule, HWND parentWindow, IExplorerplusplus *coreInterface)
{
	BookmarkItem *selectedParentFolder = nullptr;
	CAddBookmarkDialog AddBookmarkDialog(resoureceModule, parentWindow, coreInterface,
		bookmarkTree, bookmarkItem, &selectedParentFolder);
	auto res = AddBookmarkDialog.ShowModalDialog();

	if (res == CBaseDialog::RETURN_OK)
	{
		assert(selectedParentFolder != nullptr);

		size_t newIndex;

		// The bookmark properties will have already been updated, so the only
		// thing that needs to be done is to move the bookmark to the correct
		// folder.
		if (selectedParentFolder == bookmarkItem->GetParent())
		{
			newIndex = *bookmarkItem->GetParent()->GetChildIndex(bookmarkItem);
		}
		else
		{
			newIndex = selectedParentFolder->GetChildren().size();
		}

		bookmarkTree->MoveBookmarkItem(bookmarkItem, selectedParentFolder, newIndex);
	}
}