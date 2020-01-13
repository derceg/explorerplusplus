// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkHelper.h"
#include "AddBookmarkDialog.h"
#include "MainResource.h"
#include "../Helper/Macros.h"
#include <boost/range/adaptor/filtered.hpp>
#include <algorithm>

int CALLBACK SortByDefault(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByName(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByLocation(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByDateAdded(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByDateModified(const BookmarkItem *firstItem, const BookmarkItem *secondItem);

BookmarkItem *GetBookmarkItemByIdResursive(BookmarkItem *bookmarkItem, std::wstring_view guid);

bool BookmarkHelper::IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsFolder();
}

bool BookmarkHelper::IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsBookmark();
}

int CALLBACK BookmarkHelper::Sort(SortMode sortMode, const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	if(firstItem->IsFolder() && secondItem->IsBookmark())
	{
		return -1;
	}
	else if(firstItem->IsBookmark() && secondItem->IsFolder())
	{
		return 1;
	}
	else
	{
		int iRes = 0;

		switch(sortMode)
		{
		case SortMode::Default:
			iRes = SortByDefault(firstItem, secondItem);
			break;

		case SortMode::Name:
			iRes = SortByName(firstItem,secondItem);
			break;

		case SortMode::Location:
			iRes = SortByLocation(firstItem,secondItem);
			break;

		case SortMode::DateCreated:
			iRes = SortByDateAdded(firstItem,secondItem);
			break;

		case SortMode::DateModified:
			iRes = SortByDateModified(firstItem,secondItem);
			break;

		default:
			assert(false);
			break;
		}

		return iRes;
	}
}

int CALLBACK SortByDefault(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	size_t firstIndex = firstItem->GetParent()->GetChildIndex(firstItem);
	size_t secondIndex = secondItem->GetParent()->GetChildIndex(secondItem);
	return static_cast<int>(firstIndex) - static_cast<int>(secondIndex);
}

int CALLBACK SortByName(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	return StrCmpLogicalW(firstItem->GetName().c_str(), secondItem->GetName().c_str());
}

int CALLBACK SortByLocation(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	if(firstItem->IsFolder() && secondItem->IsFolder())
	{
		return SortByName(firstItem, secondItem);
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

void BookmarkHelper::AddBookmarkItem(BookmarkTree *bookmarkTree, BookmarkItem::Type type,
	HMODULE resoureceModule, HWND parentWindow, TabContainer *tabContainer,
	IExplorerplusplus *coreInterface)
{
	std::unique_ptr<BookmarkItem> bookmarkItem;

	if (type == BookmarkItem::Type::Bookmark)
	{
		const Tab &selectedTab = tabContainer->GetSelectedTab();
		auto entry = selectedTab.GetShellBrowser()->GetNavigationController()->GetCurrentEntry();

		bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, entry->GetDisplayName(),
			selectedTab.GetShellBrowser()->GetDirectory());
	}
	else
	{
		bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt,
			ResourceHelper::LoadString(resoureceModule, IDS_BOOKMARKS_NEWBOOKMARKFOLDER), std::nullopt);
	}
	
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

void BookmarkHelper::EditBookmarkItem(BookmarkItem *bookmarkItem, BookmarkTree *bookmarkTree,
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
			newIndex = bookmarkItem->GetParent()->GetChildIndex(bookmarkItem);
		}
		else
		{
			newIndex = selectedParentFolder->GetChildren().size();
		}

		bookmarkTree->MoveBookmarkItem(bookmarkItem, selectedParentFolder, newIndex);
	}
}

// If the specified item is a bookmark, it will be opened in a new tab.
// If it's a bookmark folder, each of its children will be opened in new
// tabs.
void BookmarkHelper::OpenBookmarkItemInNewTab(const BookmarkItem *bookmarkItem, IExplorerplusplus *expp)
{
	if (bookmarkItem->IsFolder())
	{
		for (auto &childItem : bookmarkItem->GetChildren() | boost::adaptors::filtered(IsBookmark))
		{
			expp->GetTabContainer()->CreateNewTab(childItem->GetLocation().c_str());
		}
	}
	else
	{
		expp->GetTabContainer()->CreateNewTab(bookmarkItem->GetLocation().c_str());
	}
}

BookmarkItem *BookmarkHelper::GetBookmarkItemById(BookmarkTree *bookmarkTree, std::wstring_view guid)
{
	return GetBookmarkItemByIdResursive(bookmarkTree->GetRoot(), guid);
}

BookmarkItem *GetBookmarkItemByIdResursive(BookmarkItem *bookmarkItem, std::wstring_view guid)
{
	if (bookmarkItem->GetGUID() == guid)
	{
		return bookmarkItem;
	}

	if (!bookmarkItem->IsFolder())
	{
		return nullptr;
	}

	for (auto &child : bookmarkItem->GetChildren())
	{
		BookmarkItem *result = GetBookmarkItemByIdResursive(child.get(), guid);

		if (result)
		{
			return result;
		}
	}

	return nullptr;
}