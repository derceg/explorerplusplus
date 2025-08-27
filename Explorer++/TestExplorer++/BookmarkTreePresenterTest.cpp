// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarkTreePresenter.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI//BookmarkTreeViewNode.h"
#include "Bookmarks/UI/BookmarkTreeViewAdapter.h"
#include "CopiedBookmark.h"
#include "SimulatedClipboardStore.h"
#include "TreeView.h"
#include "Win32ResourceLoader.h"
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/iterator_range.hpp>
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

using namespace testing;

namespace
{

MATCHER_P(CreateFolderMatcher, expectedParentFolder, "")
{
	return arg.IsFolder() && arg.GetParent() == expectedParentFolder;
}

}

class BookmarkTreePresenterTest : public Test
{
protected:
	BookmarkTreePresenterTest() :
		m_resourceLoader(GetModuleHandle(nullptr), IconSet::Color, nullptr, nullptr)
	{
	}

	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		m_treeViewWindow.reset(CreateWindow(WC_TREEVIEW, L"", WS_POPUP | TVS_EDITLABELS, 0, 0, 0, 0,
			m_parentWindow.get(), nullptr, GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_treeViewWindow, nullptr);
	}

	std::unique_ptr<BookmarkTreePresenter> BuildPresenter(
		const std::unordered_set<std::wstring> &initiallyExpandedBookmarkIds = {},
		const std::optional<std::wstring> &initiallySelectedBookmarkId = std::nullopt)
	{
		return std::make_unique<BookmarkTreePresenter>(
			std::make_unique<TreeView>(m_treeViewWindow.get()), &m_acceleratorManager,
			&m_resourceLoader, &m_bookmarkTree, &m_clipboardStore, initiallyExpandedBookmarkIds,
			initiallySelectedBookmarkId);
	}

	void VerifyViewItems(const BookmarkTreePresenter *presenter)
	{
		const auto treeViewItems = presenter->GetView()->GetAllNodesDepthFirstForTesting();
		const auto bookmarkItems = GetBookmarkFoldersDepthFirst();
		ASSERT_EQ(treeViewItems.size(), bookmarkItems.size());

		const auto *adaptor = presenter->GetAdaptorForTesting();

		for (const auto &[treeViewItem, bookmarkItem] :
			boost::combine(treeViewItems, bookmarkItems))
		{
			EXPECT_EQ(adaptor->GetBookmarkForNode(treeViewItem), bookmarkItem);
			EXPECT_EQ(treeViewItem->GetText(), bookmarkItem->GetName());
		}
	}

	AcceleratorManager m_acceleratorManager;
	Win32ResourceLoader m_resourceLoader;
	SimulatedClipboardStore m_clipboardStore;
	BookmarkTree m_bookmarkTree;

	wil::unique_hwnd m_parentWindow;
	wil::unique_hwnd m_treeViewWindow;

private:
	std::vector<const BookmarkItem *> GetBookmarkFoldersDepthFirst()
	{
		std::vector<const BookmarkItem *> bookmarkItems;
		m_bookmarkTree.GetRoot()->VisitRecursively(
			[this, &bookmarkItems](BookmarkItem *currentItem)
			{
				if (currentItem == m_bookmarkTree.GetRoot() || !currentItem->IsFolder())
				{
					return;
				}

				bookmarkItems.push_back(currentItem);
			});
		return bookmarkItems;
	}
};

TEST_F(BookmarkTreePresenterTest, InitialItems)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(folder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(folder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(folder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));

	auto *folder4 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 4", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(folder4,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	auto presenter = BuildPresenter();
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, InitialExpansionState)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(folder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));

	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));

	std::vector<BookmarkItem *> bookmarksToExpand = { folder1,
		m_bookmarkTree.GetBookmarksToolbarFolder() };

	auto guidsToExpand = boost::copy_range<std::unordered_set<std::wstring>>(
		bookmarksToExpand | boost::adaptors::transformed(std::mem_fn(&BookmarkItem::GetGUID)));
	auto presenter = BuildPresenter(guidsToExpand);

	const auto *view = presenter->GetView();
	const auto *adaptor = presenter->GetAdaptorForTesting();

	for (const auto *bookmark : bookmarksToExpand)
	{
		EXPECT_TRUE(view->IsNodeExpanded(adaptor->GetNodeForBookmark(bookmark)));
	}

	EXPECT_THAT(presenter->GetExpandedBookmarks(), UnorderedElementsAreArray(bookmarksToExpand));
}

TEST_F(BookmarkTreePresenterTest, InitialSelectedItem)
{
	auto *initiallySelectedBookmark = m_bookmarkTree.GetOtherBookmarksFolder();
	auto presenter = BuildPresenter({}, initiallySelectedBookmark->GetGUID());
	EXPECT_EQ(presenter->GetSelectedFolder(), initiallySelectedBookmark);
}

TEST_F(BookmarkTreePresenterTest, SelectedFolder)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto presenter = BuildPresenter();

	presenter->SelectItem(m_bookmarkTree.GetBookmarksMenuFolder());
	EXPECT_EQ(presenter->GetSelectedFolder(), m_bookmarkTree.GetBookmarksMenuFolder());
	EXPECT_THAT(presenter->GetSelectedItems(),
		ElementsAre(m_bookmarkTree.GetBookmarksMenuFolder()));

	presenter->SelectItem(folder1);
	EXPECT_EQ(presenter->GetSelectedFolder(), folder1);
	EXPECT_THAT(presenter->GetSelectedItems(), ElementsAre(folder1));
}

TEST_F(BookmarkTreePresenterTest, SelectBookmark)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	auto *bookmark1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));

	auto presenter = BuildPresenter();
	presenter->SelectItem(folder1);

	// Attempting to select a bookmark should have no effect (since the view only displays folders).
	presenter->SelectItem(bookmark1);
	EXPECT_EQ(presenter->GetSelectedFolder(), folder1);
}

TEST_F(BookmarkTreePresenterTest, CreateFolder)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto *folder2 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(folder2,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));

	auto presenter = BuildPresenter();

	MockFunction<void(BookmarkItem & bookmarkItem, size_t index)> callback;
	m_bookmarkTree.bookmarkItemAddedSignal.AddObserver(callback.AsStdFunction());

	InSequence seq;

	presenter->SelectItem(folder1);
	EXPECT_CALL(callback, Call(CreateFolderMatcher(folder1), 0));
	presenter->CreateFolder(0);

	presenter->SelectItem(folder2);
	EXPECT_CALL(callback, Call(CreateFolderMatcher(folder2), 1));
	presenter->CreateFolder(1);
}

TEST_F(BookmarkTreePresenterTest, AddFolder)
{
	auto presenter = BuildPresenter();

	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"New folder", std::nullopt), 0);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, UpdateFolder)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto presenter = BuildPresenter();

	folder1->SetName(L"Updated name");
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, MoveFolder)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto presenter = BuildPresenter();

	m_bookmarkTree.MoveBookmarkItem(folder1, m_bookmarkTree.GetOtherBookmarksFolder(), 0);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, MoveFolderToSameParent)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));

	auto presenter = BuildPresenter();

	m_bookmarkTree.MoveBookmarkItem(folder1, folder1->GetParent(), 2);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, MoveFolderToSameParentNoIndexChange)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));

	auto presenter = BuildPresenter();

	// In this case, the bookmark folder is being moved, but the index in the view shouldn't change.
	m_bookmarkTree.MoveBookmarkItem(folder1, folder1->GetParent(), 3);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, RemoveFolder)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto presenter = BuildPresenter();

	m_bookmarkTree.RemoveBookmarkItem(folder1);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, PasteWithNestedFolder)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	auto *folder2 = m_bookmarkTree.AddBookmarkItem(folder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(folder2,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));

	auto presenter = BuildPresenter();

	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { folder1 },
		ClipboardAction::Copy);
	BookmarkHelper::PasteBookmarkItems(&m_clipboardStore, &m_bookmarkTree,
		m_bookmarkTree.GetOtherBookmarksFolder(), 0);
	VerifyViewItems(presenter.get());
}

TEST_F(BookmarkTreePresenterTest, OnNodeRenamed)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto presenter = BuildPresenter();

	auto *delegate = presenter->GetDelegateForTesting();
	auto *adaptor = presenter->GetAdaptorForTesting();
	std::wstring updatedName = L"Updated name";
	delegate->OnNodeRenamed(adaptor->GetNodeForBookmark(folder1), updatedName);
	EXPECT_EQ(folder1->GetName(), updatedName);
}

TEST_F(BookmarkTreePresenterTest, OnNodeRemoved)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto presenter = BuildPresenter();

	auto *delegate = presenter->GetDelegateForTesting();
	auto *adaptor = presenter->GetAdaptorForTesting();
	auto guid = folder1->GetGUID();
	delegate->OnNodeRemoved(adaptor->GetNodeForBookmark(folder1));

	EXPECT_EQ(m_bookmarkTree.MaybeGetBookmarkItemById(guid), nullptr);
}

TEST_F(BookmarkTreePresenterTest, OnNodeCopied)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	CopiedBookmark copiedBookmark(*folder1);

	auto presenter = BuildPresenter();

	auto *delegate = presenter->GetDelegateForTesting();
	auto *adaptor = presenter->GetAdaptorForTesting();
	delegate->OnNodeCopied(adaptor->GetNodeForBookmark(folder1));

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(BookmarkTreePresenterTest, OnNodeCut)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	CopiedBookmark copiedBookmark(*folder1);

	auto presenter = BuildPresenter();

	auto *delegate = presenter->GetDelegateForTesting();
	auto *adaptor = presenter->GetAdaptorForTesting();
	delegate->OnNodeCut(adaptor->GetNodeForBookmark(folder1));
	EXPECT_TRUE(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().empty());

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(BookmarkTreePresenterTest, OnPaste)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	CopiedBookmark copiedBookmark(*folder1);
	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { folder1 },
		ClipboardAction::Copy);

	auto presenter = BuildPresenter();

	auto *delegate = presenter->GetDelegateForTesting();
	auto *adaptor = presenter->GetAdaptorForTesting();
	delegate->OnPaste(adaptor->GetNodeForBookmark(m_bookmarkTree.GetOtherBookmarksFolder()));
	EXPECT_THAT(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren(),
		ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(BookmarkTreePresenterTest, SelectionChangedSignal)
{
	auto *folder1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));

	auto presenter = BuildPresenter();

	MockFunction<void(BookmarkItem * bookmarkFolder)> callback;
	presenter->selectionChangedSignal.AddObserver(callback.AsStdFunction());

	auto *adaptor = presenter->GetAdaptorForTesting();
	EXPECT_CALL(callback, Call(folder1));
	presenter->GetView()->SelectNode(adaptor->GetNodeForBookmark(folder1));
}
