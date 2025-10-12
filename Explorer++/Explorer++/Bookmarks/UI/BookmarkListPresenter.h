// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkNavigatorInterface.h"
#include "Bookmarks/UI/BookmarkColumn.h"
#include "Bookmarks/UI/BookmarkDropTargetWindow.h"
#include "Bookmarks/UI/OrganizeBookmarksContextMenuDelegate.h"
#include "ListViewDelegate.h"
#include "../Helper/SortDirection.h"
#include <wil/resource.h>
#include <memory>
#include <optional>

class AcceleratorManager;
class BookmarkColumnModel;
class BookmarkIconManager;
class BookmarkListViewModel;
class BookmarkTree;
class BrowserList;
struct Config;
class IconFetcher;
class ListView;
class PlatformContext;
class ResourceLoader;

// Displays a set of bookmarks and bookmark folders within a ListView instance.
class BookmarkListPresenter :
	public BookmarkNavigatorInterface,
	public OrganizeBookmarksContextMenuDelegate,
	private BookmarkDropTargetWindow,
	private ListViewDelegate
{
public:
	BookmarkListPresenter(std::unique_ptr<ListView> view, HINSTANCE resourceInstance,
		BookmarkTree *bookmarkTree, const BookmarkColumnModel &columnModel,
		std::optional<BookmarkColumn> sortColumn, SortDirection sortDirection,
		const BrowserList *browserList, const Config *config,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
		IconFetcher *iconFetcher, PlatformContext *platformContext);
	~BookmarkListPresenter();

	ListView *GetView();
	const ListView *GetView() const;

	BookmarkItem *GetCurrentFolder();
	const BookmarkItem *GetCurrentFolder() const;

	// BookmarkNavigatorInterface
	void NavigateToBookmarkFolder(BookmarkItem *bookmarkFolder,
		const BookmarkHistoryEntry *entry = nullptr) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const BookmarkNavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

	// OrganizeBookmarksContextMenuDelegate
	bool CanSelectAllItems() const override;
	void SelectAllItems() override;
	void SelectOnly(const BookmarkItem *bookmarkItem) override;
	RawBookmarkItems GetSelectedItems() const override;
	RawBookmarkItems GetSelectedChildItems(const BookmarkItem *targetFolder) const override;
	void CreateFolder(size_t index) override;

	const BookmarkColumnModel *GetColumnModel() const;
	void ToggleColumn(BookmarkColumn column);
	wil::unique_hmenu BuildColumnsMenu();
	std::optional<BookmarkColumn> GetSortColumn() const;
	SortDirection GetSortDirection() const;
	void SetSortDetails(std::optional<BookmarkColumn> sortColumn, SortDirection direction);

	ListViewDelegate *GetDelegateForTesting();
	BookmarkListViewModel *GetModelForTesting();
	const BookmarkListViewModel *GetModelForTesting() const;

private:
	static inline const double FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE = 0.2;

	// ListViewDelegate
	void OnItemsActivated(const std::vector<ListViewItem *> &items) override;
	bool OnItemRenamed(ListViewItem *item, const std::wstring &name) override;
	void OnItemsRemoved(const std::vector<ListViewItem *> &items, RemoveMode removeMode) override;
	void OnItemsCopied(const std::vector<ListViewItem *> &items) override;
	void OnItemsCut(const std::vector<ListViewItem *> &items) override;
	void OnPaste(ListViewItem *lastSelectedItemOpt) override;
	void OnShowBackgroundContextMenu(const POINT &ptScreen) override;
	void OnShowItemContextMenu(const std::vector<ListViewItem *> &items,
		const POINT &ptScreen) override;
	void OnShowHeaderContextMenu(const POINT &ptScreen) override;
	void OnBeginDrag(const std::vector<ListViewItem *> &items) override;

	void OnBackgroundContextMenuItemSelected(int menuItemId);
	void OnNewBookmark();

	void OnHeaderContextMenuItemSelected(int menuItemId);

	// BookmarkDropTargetWindow
	DropLocation GetDropLocation(const POINT &pt) override;
	void UpdateUiForDropLocation(const DropLocation &dropLocation) override;
	void ResetDropUiState() override;

	void RemoveDropHighlight();

	void OnSortOrderChanged();

	RawBookmarkItems GetBookmarksForItems(const std::vector<ListViewItem *> &items) const;

	std::unique_ptr<BookmarkListViewModel> m_model;
	const std::unique_ptr<ListView> m_view;
	HINSTANCE m_resourceInstance;
	BookmarkTree *const m_bookmarkTree;
	const BrowserList *const m_browserList;
	const Config *const m_config;
	const AcceleratorManager *const m_acceleratorManager;
	const ResourceLoader *const m_resourceLoader;
	PlatformContext *const m_platformContext;
	std::unique_ptr<BookmarkIconManager> m_bookmarkIconManager;
	BookmarkItem *m_currentBookmarkFolder = nullptr;
	BookmarkNavigationCompletedSignal m_navigationCompletedSignal;

	std::optional<std::wstring> m_highlightedItemGuid;
};
