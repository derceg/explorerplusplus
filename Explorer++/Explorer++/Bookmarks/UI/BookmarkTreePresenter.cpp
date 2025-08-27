// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkTreePresenter.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkTreeViewAdapter.h"
#include "Bookmarks/UI/BookmarkTreeViewNode.h"
#include "MainResource.h"
#include "NoOpMenuHelpTextHost.h"
#include "PopupMenuView.h"
#include "ResourceLoader.h"
#include "TestHelper.h"
#include "TreeView.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/WindowHelper.h"
#include <glog/logging.h>
#include <wil/com.h>
#include <wil/resource.h>

BookmarkTreePresenter::BookmarkTreePresenter(std::unique_ptr<TreeView> view,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	BookmarkTree *bookmarkTree, ClipboardStore *clipboardStore,
	const std::unordered_set<std::wstring> &initiallyExpandedBookmarkIds,
	const std::optional<std::wstring> &initiallySelectedBookmarkId) :
	BookmarkDropTargetWindow(view->GetHWND(), bookmarkTree),
	m_view(std::move(view)),
	m_acceleratorManager(acceleratorManager),
	m_resourceLoader(resourceLoader),
	m_bookmarkTree(bookmarkTree),
	m_clipboardStore(clipboardStore)
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_view->GetHWND());
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	wil::unique_himagelist imageList;
	std::tie(imageList, m_imageListMappings) = ResourceHelper::CreateIconImageList(m_resourceLoader,
		iconWidth, iconHeight, { Icon::Folder });
	m_view->SetImageList(std::move(imageList));

	m_adapter = std::make_unique<BookmarkTreeViewAdapter>(m_bookmarkTree,
		m_imageListMappings[Icon::Folder]);
	m_view->SetAdapter(m_adapter.get());
	m_view->SetDelegate(this);

	SetUpTreeView(initiallyExpandedBookmarkIds, initiallySelectedBookmarkId);
}

BookmarkTreePresenter::~BookmarkTreePresenter() = default;

void BookmarkTreePresenter::SetUpTreeView(
	const std::unordered_set<std::wstring> &initiallyExpandedBookmarkIds,
	const std::optional<std::wstring> &initiallySelectedBookmarkId)
{
	for (const auto &id : initiallyExpandedBookmarkIds)
	{
		if (auto *bookmarkFolder = m_bookmarkTree->MaybeGetBookmarkItemById(id))
		{
			m_view->ExpandNode(m_adapter->GetNodeForBookmark(bookmarkFolder));
		}
	}

	if (initiallySelectedBookmarkId)
	{
		if (auto *bookmarkFolder =
				m_bookmarkTree->MaybeGetBookmarkItemById(*initiallySelectedBookmarkId))
		{
			m_view->SelectNode(m_adapter->GetNodeForBookmark(bookmarkFolder));
		}
	}
}

TreeView *BookmarkTreePresenter::GetView()
{
	return m_view.get();
}

const TreeView *BookmarkTreePresenter::GetView() const
{
	return m_view.get();
}

bool BookmarkTreePresenter::OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name)
{
	auto *bookmarkFolder = m_adapter->GetBookmarkForNode(targetNode);

	if (m_bookmarkTree->IsPermanentNode(bookmarkFolder))
	{
		return false;
	}

	bookmarkFolder->SetName(name);
	return true;
}

void BookmarkTreePresenter::OnNodeRemoved(TreeViewNode *targetNode)
{
	m_bookmarkTree->RemoveBookmarkItem(m_adapter->GetBookmarkForNode(targetNode));
}

void BookmarkTreePresenter::OnNodeCopied(TreeViewNode *targetNode)
{
	BookmarkHelper::CopyBookmarkItems(m_clipboardStore, m_bookmarkTree,
		{ m_adapter->GetBookmarkForNode(targetNode) }, ClipboardAction::Copy);
}

void BookmarkTreePresenter::OnNodeCut(TreeViewNode *targetNode)
{
	auto *bookmark = m_adapter->GetBookmarkForNode(targetNode);

	if (m_bookmarkTree->IsPermanentNode(bookmark))
	{
		return;
	}

	BookmarkHelper::CopyBookmarkItems(m_clipboardStore, m_bookmarkTree, { bookmark },
		ClipboardAction::Cut);
}

void BookmarkTreePresenter::OnPaste(TreeViewNode *targetNode)
{
	auto *bookmark = m_adapter->GetBookmarkForNode(targetNode);
	BookmarkHelper::PasteBookmarkItems(m_clipboardStore, m_bookmarkTree, bookmark,
		bookmark->GetChildren().size());
}

void BookmarkTreePresenter::OnSelectionChanged(TreeViewNode *selectedNode)
{
	selectionChangedSignal.m_signal(m_adapter->GetBookmarkForNode(selectedNode));
}

void BookmarkTreePresenter::OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen)
{
	m_view->SelectNode(targetNode);

	PopupMenuView popupMenu(NoOpMenuHelpTextHost::GetInstance());
	BookmarkTreeViewContextMenu contextMenu(&popupMenu, m_acceleratorManager, this, m_bookmarkTree,
		m_adapter->GetBookmarkForNode(targetNode), m_resourceLoader);
	popupMenu.Show(m_view->GetHWND(), ptScreen);
}

void BookmarkTreePresenter::OnBeginDrag(TreeViewNode *targetNode)
{
	auto *bookmarkFolder = m_adapter->GetBookmarkForNode(targetNode);

	if (m_bookmarkTree->IsPermanentNode(bookmarkFolder))
	{
		return;
	}

	auto dropSource = winrt::make_self<DropSourceImpl>();

	auto &ownedPtr = bookmarkFolder->GetParent()->GetChildOwnedPtr(bookmarkFolder);
	auto dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

	wil::com_ptr_nothrow<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_ALL,
		IID_PPV_ARGS(&dragSourceHelper));

	if (SUCCEEDED(hr))
	{
		dragSourceHelper->InitializeFromWindow(m_view->GetHWND(), nullptr, dataObject.get());
	}

	DWORD effect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);
}

void BookmarkTreePresenter::StartRenamingFolder(BookmarkItem *folder)
{
	m_view->StartRenamingNode(m_adapter->GetNodeForBookmark(folder));
}

void BookmarkTreePresenter::CreateFolder(size_t index)
{
	CreateFolder(GetSelectedFolder(), index);
}

void BookmarkTreePresenter::CreateFolder(BookmarkItem *parentFolder, size_t index)
{
	std::wstring newBookmarkFolderName =
		m_resourceLoader->LoadString(IDS_BOOKMARKS_NEWBOOKMARKFOLDER);
	auto *addedFolder = m_bookmarkTree->AddBookmarkItem(parentFolder,
		std::make_unique<BookmarkItem>(std::nullopt, newBookmarkFolderName, std::nullopt), index);

	auto *node = m_adapter->GetNodeForBookmark(addedFolder);
	m_view->SelectNode(node);
	m_view->StartRenamingNode(node);
}

bool BookmarkTreePresenter::CanSelectAllItems() const
{
	return false;
}

void BookmarkTreePresenter::SelectAllItems()
{
}

RawBookmarkItems BookmarkTreePresenter::GetSelectedItems() const
{
	return { GetSelectedFolder() };
}

RawBookmarkItems BookmarkTreePresenter::GetSelectedChildItems(
	const BookmarkItem *targetFolder) const
{
	UNREFERENCED_PARAMETER(targetFolder);

	return {};
}

void BookmarkTreePresenter::SelectItem(const BookmarkItem *bookmarkItem)
{
	if (!bookmarkItem->IsFolder())
	{
		return;
	}

	m_view->SelectNode(m_adapter->GetNodeForBookmark(bookmarkItem));
}

BookmarkItem *BookmarkTreePresenter::GetSelectedFolder() const
{
	return m_adapter->GetBookmarkForNode(m_view->GetSelectedNode());
}

RawBookmarkItems BookmarkTreePresenter::GetExpandedBookmarks() const
{
	RawBookmarkItems expandedBookmarks;
	auto expandedViewItems = m_view->GetExpandedNodes();

	for (auto *expandedViewItem : expandedViewItems)
	{
		auto *bookmarkFolder = m_adapter->GetBookmarkForNode(expandedViewItem);
		expandedBookmarks.push_back(bookmarkFolder);
	}

	return expandedBookmarks;
}

BookmarkTreePresenter::DropLocation BookmarkTreePresenter::GetDropLocation(const POINT &pt)
{
	POINT ptClient = pt;
	ScreenToClient(m_view->GetHWND(), &ptClient);

	auto *node = m_view->MaybeGetNodeAtPoint(ptClient);

	BookmarkItem *parentFolder = nullptr;
	size_t position;
	bool parentFolderSelected = false;

	if (node)
	{
		auto *bookmarkFolder = m_adapter->GetBookmarkForNode(node);

		auto nodeRect = m_view->GetNodeRect(node);

		RECT folderCentralRect = nodeRect;
		int indent =
			static_cast<int>(FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE * GetRectHeight(&nodeRect));
		InflateRect(&folderCentralRect, 0, -indent);

		if (ptClient.y < folderCentralRect.top)
		{
			parentFolder = bookmarkFolder->GetParent();
			position = bookmarkFolder->GetParent()->GetChildIndex(bookmarkFolder);
		}
		else if (ptClient.y > folderCentralRect.bottom)
		{
			parentFolder = bookmarkFolder->GetParent();
			position = bookmarkFolder->GetParent()->GetChildIndex(bookmarkFolder) + 1;
		}
		else
		{
			parentFolder = bookmarkFolder;
			position = bookmarkFolder->GetChildren().size();
			parentFolderSelected = true;
		}
	}
	else
	{
		auto *nextNode = m_view->MaybeGetNextVisibleNode(ptClient);

		if (nextNode)
		{
			auto *nextBookmarkFolder = m_adapter->GetBookmarkForNode(nextNode);
			parentFolder = nextBookmarkFolder->GetParent();
			position = nextBookmarkFolder->GetParent()->GetChildIndex(nextBookmarkFolder);
		}
		else
		{
			parentFolder = m_bookmarkTree->GetRoot();
			position = m_bookmarkTree->GetRoot()->GetChildren().size();
		}
	}

	return { parentFolder, position, parentFolderSelected };
}

void BookmarkTreePresenter::UpdateUiForDropLocation(const DropLocation &dropLocation)
{
	RemoveDropHighlight();

	if (dropLocation.parentFolderSelected)
	{
		m_view->RemoveInsertMark();

		m_view->HighlightNode(m_adapter->GetNodeForBookmark(dropLocation.parentFolder));
		m_highlightedItemGuid = dropLocation.parentFolder->GetGUID();
	}
	else
	{
		TreeViewNode *node;
		TreeView::InsertMarkPosition position;

		auto &children = dropLocation.parentFolder->GetChildren();
		auto insertItr = children.begin() + dropLocation.position;

		auto nextFolderItr =
			std::find_if(insertItr, children.end(), [](auto &child) { return child->IsFolder(); });

		// If the cursor is not over a folder, it means that it's within a particular parent folder.
		// That parent must have at least one child folder, since if it didn't, that would mean that
		// the folder wouldn't be expanded and the only possible positions would be within the
		// grandparent folder, either before or after the parent (but not within it).
		if (nextFolderItr == children.end())
		{
			auto reverseInsertItr = children.rbegin() + (children.size() - dropLocation.position);

			auto previousFolderItr = std::find_if(reverseInsertItr, children.rend(),
				[](auto &child) { return child->IsFolder(); });

			node = m_adapter->GetNodeForBookmark(previousFolderItr->get());
			position = TreeView::InsertMarkPosition::After;
		}
		else
		{
			node = m_adapter->GetNodeForBookmark(nextFolderItr->get());
			position = TreeView::InsertMarkPosition::Before;
		}

		m_view->ShowInsertMark(node, position);
	}
}

void BookmarkTreePresenter::ResetDropUiState()
{
	m_view->RemoveInsertMark();
	RemoveDropHighlight();
}

void BookmarkTreePresenter::RemoveDropHighlight()
{
	if (!m_highlightedItemGuid)
	{
		return;
	}

	auto *bookmarkFolder = m_bookmarkTree->MaybeGetBookmarkItemById(*m_highlightedItemGuid);

	if (bookmarkFolder)
	{
		m_view->UnhighlightNode(m_adapter->GetNodeForBookmark(bookmarkFolder));
	}

	m_highlightedItemGuid.reset();
}

TreeViewDelegate *BookmarkTreePresenter::GetDelegateForTesting()
{
	CHECK(IsInTest());
	return this;
}

BookmarkTreeViewAdapter *BookmarkTreePresenter::GetAdaptorForTesting()
{
	return const_cast<BookmarkTreeViewAdapter *>(std::as_const(*this).GetAdaptorForTesting());
}

const BookmarkTreeViewAdapter *BookmarkTreePresenter::GetAdaptorForTesting() const
{
	CHECK(IsInTest());
	return m_adapter.get();
}
