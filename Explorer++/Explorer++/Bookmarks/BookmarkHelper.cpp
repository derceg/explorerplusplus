// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/AddBookmarkDialog.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "../Helper/ShellHelper.h"
#include <boost/range/adaptor/filtered.hpp>
#include <glog/logging.h>
#include <algorithm>

namespace
{

void OpenBookmarkWithDisposition(const BookmarkItem *bookmarkItem,
	OpenFolderDisposition disposition, const std::wstring &currentDirectory, BrowserWindow *browser)
{
	DCHECK(bookmarkItem->IsBookmark());

	auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(
		bookmarkItem->GetLocation(), currentDirectory, EnvVarsExpansion::Expand);

	if (!absolutePath)
	{
		return;
	}

	browser->OpenItem(*absolutePath, disposition);
}

}

namespace BookmarkHelper
{

bool IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsFolder();
}

bool IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsBookmark();
}

void BookmarkAllTabs(BookmarkTree *bookmarkTree, BrowserWindow *browser,
	PlatformContext *platformContext, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader)
{
	std::wstring bookmarkAllTabsText =
		resourceLoader->LoadString(IDS_ADD_BOOKMARK_TITLE_BOOKMARK_ALL_TABS);
	auto bookmarkFolder = AddBookmarkItem(bookmarkTree, BookmarkItem::Type::Folder, nullptr,
		std::nullopt, browser->GetHWND(), browser, acceleratorManager, resourceLoader,
		platformContext, bookmarkAllTabsText);

	if (!bookmarkFolder)
	{
		return;
	}

	AddBrowserTabsToBookmarkFolder(bookmarkTree, browser, bookmarkFolder);
}

void AddBrowserTabsToBookmarkFolder(BookmarkTree *bookmarkTree, BrowserWindow *browser,
	BookmarkItem *bookmarkFolder)
{
	DCHECK(bookmarkFolder->IsFolder());

	for (const auto *tab : browser->GetActiveTabContainer()->GetAllTabsInOrder())
	{
		const auto &pidl = tab->GetShellBrowser()->GetDirectory();
		auto bookmark = std::make_unique<BookmarkItem>(std::nullopt,
			GetDisplayNameWithFallback(pidl.Raw(), SHGDN_INFOLDER),
			GetDisplayNameWithFallback(pidl.Raw(), SHGDN_FORPARSING));

		bookmarkTree->AddBookmarkItem(bookmarkFolder, std::move(bookmark));
	}
}

BookmarkItem *AddBookmarkItem(BookmarkTree *bookmarkTree, BookmarkItem::Type type,
	BookmarkItem *defaultParentSelection, std::optional<size_t> suggestedIndex, HWND parentWindow,
	BrowserWindow *browser, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader, PlatformContext *platformContext,
	std::optional<std::wstring> customDialogTitle)
{
	std::unique_ptr<BookmarkItem> bookmarkItem;

	if (type == BookmarkItem::Type::Bookmark)
	{
		std::wstring name;
		std::wstring location;

		if (browser)
		{
			const auto *shellBrowser = browser->GetActiveShellBrowser();
			const auto &directory = shellBrowser->GetDirectory();

			name = GetDisplayNameWithFallback(directory.Raw(), SHGDN_INFOLDER);
			location = GetDisplayNameWithFallback(directory.Raw(), SHGDN_FORPARSING);
		}

		bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, name, location);
	}
	else
	{
		bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt,
			resourceLoader->LoadString(IDS_BOOKMARKS_NEWBOOKMARKFOLDER), std::nullopt);
	}

	BookmarkItem *rawBookmarkItem = bookmarkItem.get();
	BookmarkItem *selectedParentFolder = nullptr;

	auto *addBookmarkDialog = AddBookmarkDialog::Create(resourceLoader, parentWindow, bookmarkTree,
		bookmarkItem.get(), defaultParentSelection, &selectedParentFolder, acceleratorManager,
		platformContext, customDialogTitle);
	auto res = addBookmarkDialog->ShowModalDialog();

	if (res == BaseDialog::RETURN_OK)
	{
		DCHECK(selectedParentFolder);

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

void EditBookmarkItem(BookmarkItem *bookmarkItem, BookmarkTree *bookmarkTree, HWND parentWindow,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	PlatformContext *platformContext)
{
	if (bookmarkTree->IsPermanentNode(bookmarkItem))
	{
		DCHECK(false);
		return;
	}

	BookmarkItem *selectedParentFolder = nullptr;
	auto *addBookmarkDialog = AddBookmarkDialog::Create(resourceLoader, parentWindow, bookmarkTree,
		bookmarkItem, nullptr, &selectedParentFolder, acceleratorManager, platformContext);
	auto res = addBookmarkDialog->ShowModalDialog();

	if (res == BaseDialog::RETURN_OK)
	{
		DCHECK(selectedParentFolder);

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

void RemoveBookmarks(BookmarkTree *bookmarkTree, const RawBookmarkItems &bookmarkItems)
{
	for (auto *bookmarkItem : bookmarkItems)
	{
		bookmarkTree->RemoveBookmarkItem(bookmarkItem);
	}
}

// If the specified item is a bookmark, it will be opened directly. If the item is a bookmark
// folder, each child bookmark will be opened.
void OpenBookmarkItemWithDisposition(const BookmarkItem *bookmarkItem,
	OpenFolderDisposition disposition, BrowserWindow *browser)
{
	// It doesn't make any sense to open a folder in the current tab.
	if (bookmarkItem->IsFolder() && disposition == OpenFolderDisposition::CurrentTab)
	{
		DCHECK(false);
		return;
	}

	// This isn't currently supported.
	if (bookmarkItem->IsFolder() && disposition == OpenFolderDisposition::NewWindow)
	{
		return;
	}

	const auto *shellBrowser = browser->GetActiveShellBrowser();
	std::wstring currentDirectory = shellBrowser->GetDirectoryPath();

	if (bookmarkItem->IsFolder())
	{
		for (auto &childItem : bookmarkItem->GetChildren() | boost::adaptors::filtered(IsBookmark))
		{
			OpenBookmarkWithDisposition(childItem.get(), disposition, currentDirectory, browser);

			// When opening a set of bookmarks within a folder, only the first item should be
			// switched to.
			if (disposition == OpenFolderDisposition::ForegroundTab
				|| disposition == OpenFolderDisposition::NewTabDefault
				|| disposition == OpenFolderDisposition::NewTabAlternate)
			{
				disposition = OpenFolderDisposition::BackgroundTab;
			}
		}
	}
	else
	{
		OpenBookmarkWithDisposition(bookmarkItem, disposition, currentDirectory, browser);
	}
}

// Cuts/copies the selected bookmark items. Each bookmark item needs to be part
// of the specified bookmark tree.
bool CopyBookmarkItems(ClipboardStore *clipboardStore, BookmarkTree *bookmarkTree,
	const RawBookmarkItems &bookmarkItems, ClipboardAction action)
{
	OwnedRefBookmarkItems ownedBookmarkItems;

	for (auto bookmarkItem : bookmarkItems)
	{
		auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);
		ownedBookmarkItems.push_back(ownedPtr);
	}

	BookmarkClipboard bookmarkClipboard(clipboardStore);
	bool res = bookmarkClipboard.WriteBookmarks(ownedBookmarkItems);

	if (action == ClipboardAction::Cut && res)
	{
		RemoveBookmarks(bookmarkTree, bookmarkItems);
	}

	return res;
}

// Note that the parent folder must be a part of the specified bookmark tree.
void PasteBookmarkItems(ClipboardStore *clipboardStore, BookmarkTree *bookmarkTree,
	BookmarkItem *parentFolder, size_t index)
{
	DCHECK(parentFolder->IsFolder());

	BookmarkClipboard bookmarkClipboard(clipboardStore);
	auto bookmarkItems = bookmarkClipboard.ReadBookmarks();
	int i = 0;

	for (auto &bookmarkItem : bookmarkItems)
	{
		bookmarkTree->AddBookmarkItem(parentFolder, std::move(bookmarkItem), index + i);

		i++;
	}
}

bool IsAncestor(const BookmarkItem *bookmarkItem, const BookmarkItem *possibleAncestor)
{
	if (bookmarkItem == possibleAncestor)
	{
		return true;
	}

	const BookmarkItem *parent = bookmarkItem->GetParent();

	if (!parent)
	{
		return false;
	}

	return IsAncestor(parent, possibleAncestor);
}

}
