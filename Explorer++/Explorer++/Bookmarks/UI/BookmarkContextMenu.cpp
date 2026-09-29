// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkTree.h"
#include "MainResource.h"
#include "MenuView.h"
#include "PlatformContext.h"
#include "ResourceLoader.h"
#include "../Helper/ClipboardStore.h"
#include <algorithm>
#include <ranges>

BookmarkContextMenu::BookmarkContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BookmarkTree *bookmarkTree,
	const RawBookmarkItems &bookmarkItems, const ResourceLoader *resourceLoader,
	BrowserWindow *browser, HWND parentWindow, PlatformContext *platformContext) :
	MenuBase(menuView, acceleratorManager),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkItems(bookmarkItems),
	m_resourceLoader(resourceLoader),
	m_browser(browser),
	m_parentWindow(parentWindow),
	m_platformContext(platformContext)
{
	BuildMenu();
}

void BookmarkContextMenu::BuildMenu()
{
	CHECK(AreBookmarkItemsValid());

	bool singleBookmarkSelected = (m_bookmarkItems.size() == 1) && m_bookmarkItems[0]->IsBookmark();

	if (singleBookmarkSelected)
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN))
				.SetKey(Item::Open)
				.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_HELP_TEXT))
				.SetSelectedCallback([this] { OnOpen(); }));

		m_rootMenuView->AppendItem(
			MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_IN_NEW_TAB))
				.SetKey(Item::OpenInNewTab)
				.SetHelpText(m_resourceLoader->LoadString(
					IDS_BOOKMARK_CONTEXT_MENU_OPEN_IN_NEW_TAB_HELP_TEXT))
				.SetSelectedCallback([this] { OnOpenInNewTab(); }));

		m_rootMenuView->AppendSeparator();
	}
	else
	{
		auto openAllText = std::format(L"{}\t{}",
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_ALL), GetTotalBookmarks());
		m_rootMenuView->AppendItem(MenuItemParams(openAllText)
				.SetKey(Item::OpenAll)
				.SetHelpText(
					m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_ALL_HELP_TEXT))
				.SetEnabledCallback([this] { return GetTotalBookmarks() > 0; })
				.SetSelectedCallback([this] { OnOpenAll(); }));

		m_rootMenuView->AppendSeparator();
	}

	if (m_bookmarkItems.size() == 1)
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_BOOKMARK))
				.SetKey(Item::NewBookmark)
				.SetHelpText(
					m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_BOOKMARK_HELP_TEXT))
				.SetSelectedCallback([this] { OnNewBookmarkItem(BookmarkItem::Type::Bookmark); }));

		m_rootMenuView->AppendItem(
			MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_FOLDER))
				.SetKey(Item::NewFolder)
				.SetHelpText(
					m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_FOLDER_HELP_TEXT))
				.SetSelectedCallback([this] { OnNewBookmarkItem(BookmarkItem::Type::Folder); }));

		m_rootMenuView->AppendSeparator();
	}

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_CUT))
			.SetKey(Item::Cut)
			.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_CUT_HELP_TEXT))
			.SetEnabledCallback([this] { return !HasPermanentNode(); })
			.SetSelectedCallback([this] { OnCopy(ClipboardAction::Cut); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_COPY))
			.SetKey(Item::Copy)
			.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_COPY_HELP_TEXT))
			.SetSelectedCallback([this] { OnCopy(ClipboardAction::Copy); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PASTE))
			.SetKey(Item::Paste)
			.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PASTE_HELP_TEXT))
			.SetEnabledCallback([this] { return CanPaste(); })
			.SetSelectedCallback([this] { OnPaste(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_DELETE))
			.SetKey(Item::Delete)
			.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_DELETE_HELP_TEXT))
			.SetEnabledCallback([this] { return !HasPermanentNode(); })
			.SetSelectedCallback([this] { OnDelete(); }));

	if (m_bookmarkItems.size() == 1)
	{
		m_rootMenuView->AppendSeparator();

		m_rootMenuView->AppendItem(
			MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PROPERTIES))
				.SetKey(Item::Properties)
				.SetHelpText(
					m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PROPERTIES_HELP_TEXT))
				.SetEnabledCallback([this] { return !HasPermanentNode(); })
				.SetSelectedCallback([this] { OnShowProperties(); }));
	}
}

bool BookmarkContextMenu::AreBookmarkItemsValid() const
{
	if (m_bookmarkItems.empty())
	{
		return false;
	}

	// It's expected that all items will be in the same bookmark folder (i.e. they'll all have the
	// same parent).
	const BookmarkItem *parentFolder = m_bookmarkItems[0]->GetParent();
	return std::ranges::all_of(m_bookmarkItems, [parentFolder](const auto *bookmarkItem)
		{ return bookmarkItem->GetParent() == parentFolder; });
}

size_t BookmarkContextMenu::GetTotalBookmarks() const
{
	size_t totalBookmarks = 0;

	for (const auto *bookmarkItem : m_bookmarkItems)
	{
		if (bookmarkItem->IsBookmark())
		{
			totalBookmarks++;
		}
		else
		{
			const auto &children = bookmarkItem->GetChildren();

			auto numChildBookmarks = std::count_if(children.begin(), children.end(),
				[](auto &child) { return child->IsBookmark(); });

			totalBookmarks += numChildBookmarks;
		}
	}

	return totalBookmarks;
}

bool BookmarkContextMenu::HasPermanentNode() const
{
	return std::ranges::any_of(m_bookmarkItems,
		[this](const auto *bookmarkItem) { return m_bookmarkTree->IsPermanentNode(bookmarkItem); });
}

bool BookmarkContextMenu::CanPaste() const
{
	return m_platformContext->GetClipboardStore()->IsDataAvailable(
		BookmarkClipboard::GetClipboardFormat());
}

void BookmarkContextMenu::OnOpen()
{
	DCHECK(m_bookmarkItems.size() == 1 && m_bookmarkItems[0]->IsBookmark());
	BookmarkHelper::OpenBookmarkItemWithDisposition(m_bookmarkItems[0],
		OpenFolderDisposition::CurrentTab, m_browser);
}

void BookmarkContextMenu::OnOpenInNewTab()
{
	DCHECK(m_bookmarkItems.size() == 1 && m_bookmarkItems[0]->IsBookmark());
	BookmarkHelper::OpenBookmarkItemWithDisposition(m_bookmarkItems[0],
		OpenFolderDisposition::NewTabDefault, m_browser);
}

void BookmarkContextMenu::OnOpenAll()
{
	OpenFolderDisposition disposition = OpenFolderDisposition::NewTabDefault;

	for (auto *bookmarkItem : m_bookmarkItems)
	{
		BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem, disposition, m_browser);

		disposition = OpenFolderDisposition::BackgroundTab;
	}
}

void BookmarkContextMenu::OnNewBookmarkItem(BookmarkItem::Type type)
{
	auto creationTarget = GetCreationTarget();
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, creationTarget.folder,
		creationTarget.index, m_parentWindow, m_browser, m_acceleratorManager, m_resourceLoader,
		m_platformContext);
}

void BookmarkContextMenu::OnCopy(ClipboardAction action)
{
	BookmarkHelper::CopyBookmarkItems(m_platformContext->GetClipboardStore(), m_bookmarkTree,
		m_bookmarkItems, action);
}

void BookmarkContextMenu::OnPaste()
{
	auto creationTarget = GetCreationTarget();
	BookmarkHelper::PasteBookmarkItems(m_platformContext->GetClipboardStore(), m_bookmarkTree,
		creationTarget.folder, creationTarget.index);
}

void BookmarkContextMenu::OnDelete()
{
	BookmarkHelper::RemoveBookmarks(m_bookmarkTree, m_bookmarkItems);
}

void BookmarkContextMenu::OnShowProperties()
{
	DCHECK_EQ(m_bookmarkItems.size(), 1U);
	BookmarkHelper::EditBookmarkItem(m_bookmarkItems[0], m_bookmarkTree, m_parentWindow,
		m_acceleratorManager, m_resourceLoader, m_platformContext);
}

BookmarkContextMenu::CreationTarget BookmarkContextMenu::GetCreationTarget() const
{
	BookmarkItem *targetFolder;
	size_t targetIndex;

	if (m_bookmarkItems.size() == 1 && m_bookmarkItems[0]->IsFolder())
	{
		targetFolder = m_bookmarkItems[0];
		targetIndex = targetFolder->GetChildren().size();
	}
	else
	{
		targetFolder = m_bookmarkItems[0]->GetParent();
		targetIndex = std::ranges::max(m_bookmarkItems
						  | std::views::transform([targetFolder](const auto *bookmarkItem)
							  { return targetFolder->GetChildIndex(bookmarkItem); }))
			+ 1;
	}

	return { targetFolder, targetIndex };
}
