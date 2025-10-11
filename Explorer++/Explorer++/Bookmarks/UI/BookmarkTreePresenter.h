// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/UI/BookmarkDropTargetWindow.h"
#include "Bookmarks/UI/BookmarkTreeViewContextMenu.h"
#include "Bookmarks/UI/OrganizeBookmarksContextMenuDelegate.h"
#include "ResourceHelper.h"
#include "TreeViewDelegate.h"
#include "../Helper/SignalWrapper.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

class AcceleratorManager;
class BookmarkItem;
class BookmarkTree;
class BookmarkTreeViewAdapter;
class ClipboardStore;
class ResourceLoader;
class TreeView;

// Manages a TreeView instance that contains a hierarchical set of bookmark folders.
class BookmarkTreePresenter :
	public OrganizeBookmarksContextMenuDelegate,
	private TreeViewDelegate,
	private BookmarkDropTargetWindow,
	private BookmarkTreeViewContextMenuDelegate
{
public:
	BookmarkTreePresenter(std::unique_ptr<TreeView> view,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
		BookmarkTree *bookmarkTree, ClipboardStore *clipboardStore,
		const std::unordered_set<std::wstring> &initiallyExpandedBookmarkIds,
		const std::optional<std::wstring> &initiallySelectedBookmarkId = std::nullopt);
	~BookmarkTreePresenter();

	TreeView *GetView();
	const TreeView *GetView() const;

	// OrganizeBookmarksContextMenuDelegate
	bool CanSelectAllItems() const override;
	void SelectAllItems() override;
	void CreateFolder(size_t index) override;
	RawBookmarkItems GetSelectedItems() const override;
	RawBookmarkItems GetSelectedChildItems(const BookmarkItem *targetFolder) const override;
	void SelectOnly(const BookmarkItem *bookmarkItem) override;

	BookmarkItem *GetSelectedFolder() const;
	RawBookmarkItems GetExpandedBookmarks() const;

	TreeViewDelegate *GetDelegateForTesting();
	BookmarkTreeViewAdapter *GetAdaptorForTesting();
	const BookmarkTreeViewAdapter *GetAdaptorForTesting() const;

	// Signals
	SignalWrapper<BookmarkTreePresenter, void(BookmarkItem *bookmarkFolder)> selectionChangedSignal;

private:
	static inline const double FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE = 0.2;

	void SetUpTreeView(const std::unordered_set<std::wstring> &initiallyExpandedBookmarkIds,
		const std::optional<std::wstring> &initiallySelectedBookmarkId);

	// TreeViewDelegate
	bool OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name) override;
	void OnNodeRemoved(TreeViewNode *targetNode, RemoveMode removeMode) override;
	void OnNodeCopied(TreeViewNode *targetNode) override;
	void OnNodeCut(TreeViewNode *targetNode) override;
	void OnPaste(TreeViewNode *targetNode) override;
	void OnSelectionChanged(TreeViewNode *selectedNode) override;
	void OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen) override;
	void OnBeginDrag(TreeViewNode *targetNode) override;

	// BookmarkTreeViewContextMenuDelegate
	void StartRenamingFolder(BookmarkItem *folder) override;
	void CreateFolder(BookmarkItem *parentFolder, size_t index) override;

	// BookmarkDropTargetWindow
	DropLocation GetDropLocation(const POINT &pt) override;
	void UpdateUiForDropLocation(const DropLocation &dropLocation) override;
	void ResetDropUiState() override;
	void RemoveDropHighlight();

	const std::unique_ptr<TreeView> m_view;
	const AcceleratorManager *const m_acceleratorManager;
	const ResourceLoader *const m_resourceLoader;
	IconImageListMapping m_imageListMappings;
	BookmarkTree *const m_bookmarkTree;
	ClipboardStore *const m_clipboardStore;
	std::unique_ptr<BookmarkTreeViewAdapter> m_adapter;

	// This will only be set when an item is being dragged over a folder in the view.
	std::optional<std::wstring> m_highlightedItemGuid;
};
