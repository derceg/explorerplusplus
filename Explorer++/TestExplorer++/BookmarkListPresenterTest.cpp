// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarkListPresenter.h"
#include "BookmarkTestHelper.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/UI/BookmarkColumnModel.h"
#include "Bookmarks/UI/BookmarkListViewItem.h"
#include "Bookmarks/UI/BookmarkListViewModel.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "CopiedBookmark.h"
#include "IconFetcherFake.h"
#include "LabelEditHandler.h"
#include "ListView.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>
#include <wil/resource.h>

using namespace testing;

class BookmarkListPresenterTest : public BrowserTestBase
{
protected:
	BookmarkListPresenterTest() :
		m_resourceInstance(GetModuleHandle(nullptr)),
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab(L"c:\\initial\\folder"))
	{
	}

	auto BuildPresenter(const BookmarkColumnModel &columnModel = BookmarkColumnModel())
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		CHECK(m_parentWindow);

		HWND listViewWindow = CreateWindow(WC_LISTVIEW, L"",
			WS_POPUP | LVS_REPORT | LVS_EDITLABELS | LVS_SHAREIMAGELISTS, 0, 0, 0, 0,
			m_parentWindow.get(), nullptr, GetModuleHandle(nullptr), nullptr);
		CHECK(listViewWindow);

		return std::make_unique<BookmarkListPresenter>(
			std::make_unique<ListView>(listViewWindow, m_platformContext.GetKeyboardState(),
				LabelEditHandler::CreateForTest, &m_resourceLoader),
			m_resourceInstance, &m_bookmarkTree, columnModel, std::nullopt,
			SortDirection::Ascending, &m_browserList, &m_config, &m_acceleratorManager,
			&m_resourceLoader, &m_iconFetcher, &m_platformContext);
	}

	void VerifyViewItems(const BookmarkListPresenter *presenter)
	{
		const auto *view = presenter->GetView();
		const auto *model = presenter->GetModelForTesting();
		const auto *columnModel = presenter->GetColumnModel();

		int numChildItems = static_cast<int>(presenter->GetCurrentFolder()->GetChildren().size());
		ASSERT_EQ(view->GetItemCountForTesting(), numChildItems);

		for (int i = 0; i < numChildItems; i++)
		{
			const auto *item = model->GetItemAtIndex(i);
			EXPECT_EQ(view->GetItemAtIndexForTesting(i), item);

			for (int j = 0; j < columnModel->GetNumVisibleColumns(); j++)
			{
				EXPECT_EQ(view->GetItemTextForTesting(i, j),
					item->GetColumnText(columnModel->GetColumnIdAtVisibleIndex(j)));
			}
		}
	}

	HINSTANCE m_resourceInstance;
	IconFetcherFake m_iconFetcher;

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;

	wil::unique_hwnd m_parentWindow;
};

TEST_F(BookmarkListPresenterTest, Items)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, MultipleNavigations)
{
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();

	presenter->NavigateToBookmarkFolder(m_bookmarkTree.GetBookmarksMenuFolder());
	EXPECT_EQ(presenter->GetCurrentFolder(), m_bookmarkTree.GetBookmarksMenuFolder());
	VerifyViewItems(presenter.get());

	presenter->NavigateToBookmarkFolder(m_bookmarkTree.GetBookmarksToolbarFolder());
	EXPECT_EQ(presenter->GetCurrentFolder(), m_bookmarkTree.GetBookmarksToolbarFolder());
	VerifyViewItems(presenter.get());

	presenter->NavigateToBookmarkFolder(m_bookmarkTree.GetOtherBookmarksFolder());
	EXPECT_EQ(presenter->GetCurrentFolder(), m_bookmarkTree.GetOtherBookmarksFolder());
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, SelectAllItems)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	auto *bookmark1 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	auto *folder = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	auto *bookmark2 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	presenter->SelectAllItems();
	EXPECT_THAT(presenter->GetSelectedItems(), ElementsAre(bookmark1, folder, bookmark2));
}

TEST_F(BookmarkListPresenterTest, SelectOnly)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	auto *bookmark1 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	auto *bookmark2 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);
	EXPECT_THAT(presenter->GetSelectedItems(), IsEmpty());

	presenter->SelectOnly(bookmark1);
	EXPECT_THAT(presenter->GetSelectedItems(), ElementsAre(bookmark1));

	presenter->SelectOnly(bookmark2);
	EXPECT_THAT(presenter->GetSelectedItems(), ElementsAre(bookmark2));
}

TEST_F(BookmarkListPresenterTest, CreateFolder)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	MockFunction<void(BookmarkItem & bookmarkItem, size_t index)> callback;
	m_bookmarkTree.bookmarkItemAddedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call(CreateFolderMatcher(targetFolder), 0));
	presenter->CreateFolder(0);

	EXPECT_CALL(callback, Call(CreateFolderMatcher(targetFolder), 3));
	presenter->CreateFolder(3);
}

TEST_F(BookmarkListPresenterTest, ToggleColumn)
{
	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(m_bookmarkTree.GetBookmarksMenuFolder());

	const auto *columnModel = presenter->GetColumnModel();
	BookmarkColumn column = BookmarkColumn::DateCreated;
	auto columnId = columnModel->BookmarkColumnToColumnId(column);

	bool originalVisibility = columnModel->IsColumnVisible(columnId);
	presenter->ToggleColumn(column);
	EXPECT_EQ(columnModel->IsColumnVisible(columnId), !originalVisibility);

	presenter->ToggleColumn(column);
	EXPECT_EQ(columnModel->IsColumnVisible(columnId), originalVisibility);
}

TEST_F(BookmarkListPresenterTest, TogglePrimaryColumn)
{
	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(m_bookmarkTree.GetBookmarksMenuFolder());

	const auto *columnModel = presenter->GetColumnModel();

	// Attempting to toggle the name column off should have no effect, since it's the primary
	// column.
	presenter->ToggleColumn(BookmarkColumn::Name);
	EXPECT_TRUE(
		columnModel->IsColumnVisible(columnModel->BookmarkColumnToColumnId(BookmarkColumn::Name)));
}

TEST_F(BookmarkListPresenterTest, SortColumn)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksMenuFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder A", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark X", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder B", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark Y", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	for (auto column : BookmarkColumn::_values())
	{
		presenter->SetSortDetails(column, SortDirection::Ascending);
		EXPECT_EQ(presenter->GetSortColumn(), column);
		VerifyViewItems(presenter.get());
	}
}

TEST_F(BookmarkListPresenterTest, SortDirection)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksMenuFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder A", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark X", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder B", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark Y", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	presenter->SetSortDetails(BookmarkColumn::Name, SortDirection::Descending);
	EXPECT_EQ(presenter->GetSortDirection(), +SortDirection::Descending);
	VerifyViewItems(presenter.get());

	presenter->SetSortDetails(BookmarkColumn::Name, SortDirection::Ascending);
	EXPECT_EQ(presenter->GetSortDirection(), +SortDirection::Ascending);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, AddItem)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 3", L"e:\\"), 2);

	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, UpdateItem)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	bookmark->SetName(L"Updated name");
	VerifyViewItems(presenter.get());

	bookmark->SetLocation(L"d:\\");
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, UpdateItemWithPositionChange)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	auto *bookmark2 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->SetSortDetails(BookmarkColumn::Location, SortDirection::Ascending);
	presenter->NavigateToBookmarkFolder(targetFolder);

	presenter->SelectOnly(bookmark2);

	bookmark2->SetLocation(L"a:\\");
	VerifyViewItems(presenter.get());

	// The item was selected before its position changed, so it should still be selected after.
	EXPECT_THAT(presenter->GetSelectedItems(), ElementsAre(bookmark2));
}

TEST_F(BookmarkListPresenterTest, MoveItemOutOfFolder)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksMenuFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	m_bookmarkTree.MoveBookmarkItem(bookmark, m_bookmarkTree.GetOtherBookmarksFolder(), 0);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, MoveItemInToFolder)
{
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	auto *targetFolder = m_bookmarkTree.GetBookmarksMenuFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	m_bookmarkTree.MoveBookmarkItem(bookmark, targetFolder, 1);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, MoveItemWithinFolder)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksMenuFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *view = presenter->GetView();
	auto *model = presenter->GetModelForTesting();
	view->SelectItem(model->GetItemForBookmark(bookmark));

	m_bookmarkTree.MoveBookmarkItem(bookmark, targetFolder, 2);
	VerifyViewItems(presenter.get());

	// The bookmark was selected before being moved, so it should still be selected after.
	EXPECT_THAT(view->GetSelectedItems(), ElementsAre(model->GetItemForBookmark(bookmark)));
}

TEST_F(BookmarkListPresenterTest, RemoveItem)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	m_bookmarkTree.RemoveBookmarkItem(bookmark);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkListPresenterTest, OnFolderActivated)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksMenuFolder();
	auto *folder = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	// Activating a folder should result in a navigation to that folder.
	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	delegate->OnItemsActivated({ model->GetItemForBookmark(folder) });
	EXPECT_EQ(presenter->GetCurrentFolder(), folder);
}

TEST_F(BookmarkListPresenterTest, OnBookmarkActivated)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\bookmark\\location"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	delegate->OnItemsActivated({ model->GetItemForBookmark(bookmark) });
	ASSERT_EQ(m_browser->GetActiveTabContainer()->GetNumTabs(), 2);

	// Activating a bookmark should result in the bookmark being opened in a new tab.
	const auto &tab = m_browser->GetActiveTabContainer()->GetTabByIndex(1);
	EXPECT_EQ(tab.GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(bookmark->GetLocation()));
}

TEST_F(BookmarkListPresenterTest, OnMultipleItemsActivated)
{
	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	auto *folder = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	auto *bookmark1 = m_bookmarkTree.AddBookmarkItem(folder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	auto *bookmark2 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	delegate->OnItemsActivated(
		{ model->GetItemForBookmark(folder), model->GetItemForBookmark(bookmark2) });
	ASSERT_EQ(m_browser->GetActiveTabContainer()->GetNumTabs(), 3);

	// Each of the activated items should be opened. That is, the bookmark contained within the
	// activated folder should be opened in one tab and the second bookmark in another tab.
	const auto &tab1 = m_browser->GetActiveTabContainer()->GetTabByIndex(1);
	EXPECT_EQ(tab1.GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(bookmark1->GetLocation()));

	const auto &tab2 = m_browser->GetActiveTabContainer()->GetTabByIndex(2);
	EXPECT_EQ(tab2.GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(bookmark2->GetLocation()));
}

TEST_F(BookmarkListPresenterTest, OnItemRenamed)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	std::wstring updatedName = L"Updated name";
	delegate->OnItemRenamed(model->GetItemForBookmark(bookmark), updatedName);
	EXPECT_EQ(bookmark->GetName(), updatedName);
}

TEST_F(BookmarkListPresenterTest, OnItemsRemoved)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));
	auto *folder = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	delegate->OnItemsRemoved(
		{ model->GetItemForBookmark(bookmark), model->GetItemForBookmark(folder) },
		RemoveMode::Standard);
	EXPECT_EQ(targetFolder->GetChildren().size(), 0u);
}

TEST_F(BookmarkListPresenterTest, OnItemsCopied)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	CopiedBookmark copiedBookmark(*bookmark);
	delegate->OnItemsCopied({ model->GetItemForBookmark(bookmark) });

	BookmarkClipboard bookmarkClipboard(m_platformContext.GetClipboardStore());
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(BookmarkListPresenterTest, OnItemsCut)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	CopiedBookmark copiedBookmark(*bookmark);
	delegate->OnItemsCut({ model->GetItemForBookmark(bookmark) });
	EXPECT_TRUE(targetFolder->GetChildren().empty());

	BookmarkClipboard bookmarkClipboard(m_platformContext.GetClipboardStore());
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(BookmarkListPresenterTest, OnPaste)
{
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	CopiedBookmark copiedBookmark(*bookmark);
	BookmarkHelper::CopyBookmarkItems(m_platformContext.GetClipboardStore(), &m_bookmarkTree,
		{ bookmark }, ClipboardAction::Copy);

	auto *targetFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	auto *folder2 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));

	auto presenter = BuildPresenter();
	presenter->NavigateToBookmarkFolder(targetFolder);

	auto *delegate = presenter->GetDelegateForTesting();
	auto *model = presenter->GetModelForTesting();
	delegate->OnPaste(model->GetItemForBookmark(folder2));
	EXPECT_THAT(targetFolder->GetChildren(), ElementsAre(_, _, Pointee(copiedBookmark), _));
}

TEST_F(BookmarkListPresenterTest, NavigationCompletedSignal)
{
	auto presenter = BuildPresenter();

	MockFunction<void(BookmarkItem * bookmarkFolder, const BookmarkHistoryEntry *entry)> callback;
	presenter->AddNavigationCompletedObserver(callback.AsStdFunction());

	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	EXPECT_CALL(callback, Call(targetFolder, nullptr));
	presenter->NavigateToBookmarkFolder(targetFolder);
}
