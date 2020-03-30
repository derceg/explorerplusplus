// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/AddBookmarkDialog.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
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

int CALLBACK BookmarkHelper::Sort(
	ColumnType columnType, const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	if (firstItem->IsFolder() && secondItem->IsBookmark())
	{
		return -1;
	}
	else if (firstItem->IsBookmark() && secondItem->IsFolder())
	{
		return 1;
	}
	else
	{
		int iRes = 0;

		switch (columnType)
		{
		case ColumnType::Default:
			iRes = SortByDefault(firstItem, secondItem);
			break;

		case ColumnType::Name:
			iRes = SortByName(firstItem, secondItem);
			break;

		case ColumnType::Location:
			iRes = SortByLocation(firstItem, secondItem);
			break;

		case ColumnType::DateCreated:
			iRes = SortByDateAdded(firstItem, secondItem);
			break;

		case ColumnType::DateModified:
			iRes = SortByDateModified(firstItem, secondItem);
			break;

		default:
			assert(false);
			break;
		}

		return iRes;
	}
}

int CALLBACK SortByDefault(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	size_t firstIndex = firstItem->GetParent()->GetChildIndex(firstItem);
	size_t secondIndex = secondItem->GetParent()->GetChildIndex(secondItem);
	return static_cast<int>(firstIndex) - static_cast<int>(secondIndex);
}

int CALLBACK SortByName(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	return StrCmpLogicalW(firstItem->GetName().c_str(), secondItem->GetName().c_str());
}

int CALLBACK SortByLocation(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	if (firstItem->IsFolder() && secondItem->IsFolder())
	{
		return SortByName(firstItem, secondItem);
	}
	else
	{
		return firstItem->GetLocation().compare(secondItem->GetLocation());
	}
}

int CALLBACK SortByDateAdded(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	FILETIME firstItemDateCreated = firstItem->GetDateCreated();
	FILETIME secondItemDateCreated = secondItem->GetDateCreated();
	return CompareFileTime(&firstItemDateCreated, &secondItemDateCreated);
}

int CALLBACK SortByDateModified(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	FILETIME firstItemDateModified = firstItem->GetDateModified();
	FILETIME secondItemDateModified = secondItem->GetDateModified();
	return CompareFileTime(&firstItemDateModified, &secondItemDateModified);
}

void BookmarkHelper::BookmarkAllTabs(BookmarkTree *bookmarkTree, HMODULE resoureceModule,
	HWND parentWindow, IExplorerplusplus *coreInterface)
{
	std::wstring bookmarkAllTabsText =
		ResourceHelper::LoadString(resoureceModule, IDS_ADD_BOOKMARK_TITLE_BOOKMARK_ALL_TABS);
	auto bookmarkFolder = AddBookmarkItem(bookmarkTree, BookmarkItem::Type::Folder, nullptr,
		std::nullopt, parentWindow, coreInterface, bookmarkAllTabsText);

	if (!bookmarkFolder)
	{
		return;
	}

	size_t index = 0;

	for (auto tabRef : coreInterface->GetTabContainer()->GetAllTabsInOrder())
	{
		auto &tab = tabRef.get();
		auto entry = tab.GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
		auto bookmark = std::make_unique<BookmarkItem>(
			std::nullopt, entry->GetDisplayName(), tab.GetShellBrowser()->GetDirectory());

		bookmarkTree->AddBookmarkItem(bookmarkFolder, std::move(bookmark), index);

		index++;
	}
}

BookmarkItem *BookmarkHelper::AddBookmarkItem(BookmarkTree *bookmarkTree, BookmarkItem::Type type,
	BookmarkItem *defaultParentSelection, std::optional<size_t> suggestedIndex, HWND parentWindow,
	IExplorerplusplus *coreInterface, std::optional<std::wstring> customDialogTitle)
{
	std::unique_ptr<BookmarkItem> bookmarkItem;

	if (type == BookmarkItem::Type::Bookmark)
	{
		const Tab &selectedTab = coreInterface->GetTabContainer()->GetSelectedTab();
		auto entry = selectedTab.GetShellBrowser()->GetNavigationController()->GetCurrentEntry();

		bookmarkItem = std::make_unique<BookmarkItem>(
			std::nullopt, entry->GetDisplayName(), selectedTab.GetShellBrowser()->GetDirectory());
	}
	else
	{
		bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt,
			ResourceHelper::LoadString(
				coreInterface->GetLanguageModule(), IDS_BOOKMARKS_NEWBOOKMARKFOLDER),
			std::nullopt);
	}

	BookmarkItem *rawBookmarkItem = bookmarkItem.get();
	BookmarkItem *selectedParentFolder = nullptr;

	AddBookmarkDialog addBookmarkDialog(coreInterface->GetLanguageModule(), parentWindow,
		coreInterface, bookmarkTree, bookmarkItem.get(), defaultParentSelection,
		&selectedParentFolder, customDialogTitle);
	auto res = addBookmarkDialog.ShowModalDialog();

	if (res == BaseDialog::RETURN_OK)
	{
		assert(selectedParentFolder != nullptr);

		size_t targetIndex;

		if (selectedParentFolder == defaultParentSelection && suggestedIndex)
		{
			targetIndex = *suggestedIndex;
		}
		else
		{
			targetIndex = selectedParentFolder->GetChildren().size();
		}

		bookmarkTree->AddBookmarkItem(selectedParentFolder, std::move(bookmarkItem), targetIndex);

		return rawBookmarkItem;
	}

	return nullptr;
}

void BookmarkHelper::EditBookmarkItem(BookmarkItem *bookmarkItem, BookmarkTree *bookmarkTree,
	HMODULE resoureceModule, HWND parentWindow, IExplorerplusplus *coreInterface)
{
	BookmarkItem *selectedParentFolder = nullptr;
	AddBookmarkDialog addBookmarkDialog(resoureceModule, parentWindow, coreInterface, bookmarkTree,
		bookmarkItem, nullptr, &selectedParentFolder);
	auto res = addBookmarkDialog.ShowModalDialog();

	if (res == BaseDialog::RETURN_OK)
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
void BookmarkHelper::OpenBookmarkItemInNewTab(
	const BookmarkItem *bookmarkItem, IExplorerplusplus *expp)
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

// Cuts/copies the selected bookmark items. Each bookmark item needs to be part
// of the specified bookmark tree.
bool BookmarkHelper::CopyBookmarkItems(
	BookmarkTree *bookmarkTree, const RawBookmarkItems &bookmarkItems, bool cut)
{
	OwnedRefBookmarkItems ownedBookmarkItems;

	for (auto bookmarkItem : bookmarkItems)
	{
		auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);
		ownedBookmarkItems.push_back(ownedPtr);
	}

	BookmarkClipboard bookmarkClipboard;
	bool res = bookmarkClipboard.WriteBookmarks(ownedBookmarkItems);

	if (cut && res)
	{
		for (auto bookmarkItem : bookmarkItems)
		{
			if (!bookmarkTree->IsPermanentNode(bookmarkItem))
			{
				bookmarkTree->RemoveBookmarkItem(bookmarkItem);
			}
		}
	}

	return res;
}

// Note that the parent folder must be a part of the specified bookmark tree.
void BookmarkHelper::PasteBookmarkItems(
	BookmarkTree *bookmarkTree, BookmarkItem *parentFolder, size_t index)
{
	assert(parentFolder->IsFolder());

	BookmarkClipboard bookmarkClipboard;
	auto bookmarkItems = bookmarkClipboard.ReadBookmarks();
	int i = 0;

	for (auto &bookmarkItem : bookmarkItems)
	{
		bookmarkTree->AddBookmarkItem(parentFolder, std::move(bookmarkItem), index + i);

		i++;
	}
}

BookmarkItem *BookmarkHelper::GetBookmarkItemById(
	BookmarkTree *bookmarkTree, std::wstring_view guid)
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

bool BookmarkHelper::IsAncestor(BookmarkItem *bookmarkItem, BookmarkItem *possibleAncestor)
{
	if (bookmarkItem == possibleAncestor)
	{
		return true;
	}

	BookmarkItem *parent = bookmarkItem->GetParent();

	if (!parent)
	{
		return false;
	}

	return IsAncestor(parent, possibleAncestor);
}