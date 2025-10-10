// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkListPresenter.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkColumnHelper.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/UI/BookmarkListViewModel.h"
#include "ListView.h"
#include "MainResource.h"
#include "NoOpMenuHelpTextHost.h"
#include "PlatformContext.h"
#include "PopupMenuView.h"
#include "ResourceLoader.h"
#include "TestHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>

BookmarkListPresenter::BookmarkListPresenter(std::unique_ptr<ListView> view,
	HINSTANCE resourceInstance, BookmarkTree *bookmarkTree, const BookmarkColumnModel &columnModel,
	std::optional<BookmarkColumn> sortColumn, SortDirection sortDirection, BrowserWindow *browser,
	const Config *config, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader, IconFetcher *iconFetcher,
	PlatformContext *platformContext) :
	BookmarkDropTargetWindow(view->GetHWND(), bookmarkTree),
	m_view(std::move(view)),
	m_resourceInstance(resourceInstance),
	m_bookmarkTree(bookmarkTree),
	m_browser(browser),
	m_config(config),
	m_acceleratorManager(acceleratorManager),
	m_resourceLoader(resourceLoader),
	m_platformContext(platformContext)
{
	m_view->AddExtendedStyles(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_view->GetHWND());
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);

	m_bookmarkIconManager =
		std::make_unique<BookmarkIconManager>(m_resourceLoader, iconFetcher, iconWidth, iconHeight);
	m_view->SetImageList(m_bookmarkIconManager->GetImageList(),
		ListView::ImageListType::SmallIcons);

	m_model = std::make_unique<BookmarkListViewModel>(m_bookmarkTree, m_bookmarkIconManager.get(),
		columnModel, m_config);
	m_model->sortOrderChangedSignal.AddObserver(
		std::bind_front(&BookmarkListPresenter::OnSortOrderChanged, this));
	SetSortDetails(sortColumn, sortDirection);
	m_view->SetModel(m_model.get());

	m_view->SetDelegate(this);
}

BookmarkListPresenter::~BookmarkListPresenter() = default;

ListView *BookmarkListPresenter::GetView()
{
	return m_view.get();
}

const ListView *BookmarkListPresenter::GetView() const
{
	return m_view.get();
}

BookmarkItem *BookmarkListPresenter::GetCurrentFolder()
{
	return m_currentBookmarkFolder;
}

const BookmarkItem *BookmarkListPresenter::GetCurrentFolder() const
{
	return m_currentBookmarkFolder;
}

void BookmarkListPresenter::NavigateToBookmarkFolder(BookmarkItem *bookmarkFolder,
	const BookmarkHistoryEntry *entry)
{
	DCHECK(bookmarkFolder->IsFolder());

	m_currentBookmarkFolder = bookmarkFolder;
	m_model->SetCurrentFolder(bookmarkFolder);
	m_navigationCompletedSignal(bookmarkFolder, entry);
}

boost::signals2::connection BookmarkListPresenter::AddNavigationCompletedObserver(
	const BookmarkNavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCompletedSignal.connect(observer, position);
}

std::optional<BookmarkColumn> BookmarkListPresenter::GetSortColumn() const
{
	auto sortColumnId = m_model->GetSortColumnId();

	if (!sortColumnId)
	{
		return std::nullopt;
	}

	return BookmarkColumnModel::ColumnIdToBookmarkColumn(*sortColumnId);
}

SortDirection BookmarkListPresenter::GetSortDirection() const
{
	return m_model->GetSortDirection();
}

void BookmarkListPresenter::SetSortDetails(std::optional<BookmarkColumn> sortColumn,
	SortDirection direction)
{
	std::optional<ListViewColumnId> columnId;

	if (sortColumn)
	{
		columnId = BookmarkColumnModel::BookmarkColumnToColumnId(*sortColumn);
	}

	m_model->SetSortDetails(columnId, direction);
}

void BookmarkListPresenter::OnSortOrderChanged()
{
	// It's only possible to drop items when using the default sort mode (i.e. when a sort column
	// isn't set), since that's the only mode in which the listview indexes match the bookmark item
	// indexes.
	SetBlockDrop(m_model->GetSortColumnId().has_value());
}

bool BookmarkListPresenter::CanSelectAllItems() const
{
	return true;
}

void BookmarkListPresenter::SelectAllItems()
{
	m_view->SelectAllItems();
}

void BookmarkListPresenter::SelectOnly(const BookmarkItem *bookmarkItem)
{
	m_view->DeselectAllItems();
	m_view->SelectItem(m_model->GetItemForBookmark(bookmarkItem));
}

RawBookmarkItems BookmarkListPresenter::GetSelectedItems() const
{
	return GetSelectedChildItems(m_currentBookmarkFolder);
}

RawBookmarkItems BookmarkListPresenter::GetSelectedChildItems(
	const BookmarkItem *targetFolder) const
{
	if (targetFolder != m_currentBookmarkFolder)
	{
		return {};
	}

	return GetBookmarksForItems(m_view->GetSelectedItems());
}

void BookmarkListPresenter::CreateFolder(size_t index)
{
	const auto *bookmarkFolder = m_bookmarkTree->AddBookmarkItem(m_currentBookmarkFolder,
		std::make_unique<BookmarkItem>(std::nullopt,
			m_resourceLoader->LoadString(IDS_BOOKMARKS_NEWBOOKMARKFOLDER), std::nullopt),
		index);

	SelectOnly(bookmarkFolder);
	m_view->StartRenamingItem(m_model->GetItemForBookmark(bookmarkFolder));
}

void BookmarkListPresenter::OnItemsActivated(const std::vector<ListViewItem *> &items)
{
	auto bookmarkItems = GetBookmarksForItems(items);

	if (bookmarkItems.size() == 1 && bookmarkItems[0]->IsFolder())
	{
		NavigateToBookmarkFolder(bookmarkItems[0]);
	}
	else
	{
		OpenFolderDisposition disposition = OpenFolderDisposition::NewTabDefault;

		for (BookmarkItem *bookmarkItem : bookmarkItems)
		{
			BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem, disposition, m_browser);

			disposition = OpenFolderDisposition::BackgroundTab;
		}
	}
}

bool BookmarkListPresenter::OnItemRenamed(ListViewItem *item, const std::wstring &name)
{
	auto *bookmarkItem = m_model->GetBookmarkForItem(item);

	if (m_bookmarkTree->IsPermanentNode(bookmarkItem))
	{
		return false;
	}

	bookmarkItem->SetName(name);

	return true;
}

void BookmarkListPresenter::OnItemsDeleted(const std::vector<ListViewItem *> &items)
{
	BookmarkHelper::RemoveBookmarks(m_bookmarkTree, GetBookmarksForItems(items));
}

void BookmarkListPresenter::OnItemsCopied(const std::vector<ListViewItem *> &items)
{
	BookmarkHelper::CopyBookmarkItems(m_platformContext->GetClipboardStore(), m_bookmarkTree,
		GetBookmarksForItems(items), ClipboardAction::Copy);
}

void BookmarkListPresenter::OnItemsCut(const std::vector<ListViewItem *> &items)
{
	BookmarkHelper::CopyBookmarkItems(m_platformContext->GetClipboardStore(), m_bookmarkTree,
		GetBookmarksForItems(items), ClipboardAction::Cut);
}

void BookmarkListPresenter::OnPaste(ListViewItem *lastSelectedItemOpt)
{
	BookmarkHelper::PasteBookmarkItems(m_platformContext->GetClipboardStore(), m_bookmarkTree,
		m_currentBookmarkFolder,
		lastSelectedItemOpt ? m_model->GetItemIndex(lastSelectedItemOpt) + 1
							: m_currentBookmarkFolder->GetChildren().size());
}

void BookmarkListPresenter::OnShowBackgroundContextMenu(const POINT &ptScreen)
{
	wil::unique_hmenu parentMenu(
		LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_BOOKMARK_LISTVIEW_CONTEXT_MENU)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	int menuItemId = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, ptScreen.x, ptScreen.y, 0,
		m_view->GetHWND(), nullptr);

	if (menuItemId != 0)
	{
		OnBackgroundContextMenuItemSelected(menuItemId);
	}
}

void BookmarkListPresenter::OnBackgroundContextMenuItemSelected(int menuItemId)
{
	switch (menuItemId)
	{
	case IDM_BOOKMARKS_NEW_BOOKMARK:
		OnNewBookmark();
		break;

	case IDM_BOOKMARKS_NEW_FOLDER:
		CreateFolder(m_currentBookmarkFolder->GetChildren().size());
		break;

	default:
		DCHECK(false);
		break;
	}
}

void BookmarkListPresenter::OnNewBookmark()
{
	size_t targetIndex;
	auto selectedItems = m_view->GetSelectedItems();

	if (!selectedItems.empty())
	{
		targetIndex = m_model->GetItemIndex(*selectedItems.rbegin()) + 1;
	}
	else
	{
		targetIndex = m_currentBookmarkFolder->GetChildren().size();
	}

	const auto *bookmark = BookmarkHelper::AddBookmarkItem(m_bookmarkTree,
		BookmarkItem::Type::Bookmark, m_currentBookmarkFolder, targetIndex, m_view->GetHWND(),
		m_browser, m_acceleratorManager, m_resourceLoader, m_platformContext);

	if (!bookmark)
	{
		return;
	}

	if (bookmark->GetParent() != m_currentBookmarkFolder)
	{
		return;
	}

	SelectOnly(bookmark);
}

void BookmarkListPresenter::OnShowItemContextMenu(const std::vector<ListViewItem *> &items,
	const POINT &ptScreen)
{
	PopupMenuView popupMenu(NoOpMenuHelpTextHost::GetInstance());
	BookmarkContextMenu contextMenu(&popupMenu, m_acceleratorManager, m_bookmarkTree,
		GetBookmarksForItems(items), m_resourceLoader, m_browser, m_view->GetHWND(),
		m_platformContext);
	popupMenu.Show(m_view->GetHWND(), ptScreen);
}

void BookmarkListPresenter::OnShowHeaderContextMenu(const POINT &ptScreen)
{
	auto menu = BuildColumnsMenu();

	if (!menu)
	{
		return;
	}

	// The name column can't be removed.
	MenuHelper::EnableItem(menu.get(), BookmarkColumn::Name, FALSE);

	int cmd = TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, ptScreen.x, ptScreen.y, 0,
		m_view->GetHWND(), nullptr);

	if (cmd != 0)
	{
		OnHeaderContextMenuItemSelected(cmd);
	}
}

wil::unique_hmenu BookmarkListPresenter::BuildColumnsMenu()
{
	wil::unique_hmenu menu(CreatePopupMenu());

	if (!menu)
	{
		return nullptr;
	}

	const auto *columnModel = m_model->GetColumnModel();

	for (auto columnId : columnModel->GetAllColumnIds())
	{
		const auto &column = columnModel->GetColumnById(columnId);
		auto bookmarkColumn = BookmarkColumnModel::ColumnIdToBookmarkColumn(columnId);
		std::wstring columnText =
			m_resourceLoader->LoadString(GetBookmarkColumnStringId(bookmarkColumn));

		MenuHelper::AddStringItem(menu.get(), bookmarkColumn, columnText);
		MenuHelper::CheckItem(menu.get(), bookmarkColumn, column.visible);
	}

	return menu;
}

void BookmarkListPresenter::OnHeaderContextMenuItemSelected(int menuItemId)
{
	auto columnType = BookmarkColumn::_from_integral_nothrow(menuItemId);
	CHECK(columnType);
	ToggleColumn(*columnType);
}

RawBookmarkItems BookmarkListPresenter::GetBookmarksForItems(
	const std::vector<ListViewItem *> &items) const
{
	return boost::copy_range<RawBookmarkItems>(items
		| boost::adaptors::transformed(
			[this](auto *item) { return m_model->GetBookmarkForItem(item); }));
}

const BookmarkColumnModel *BookmarkListPresenter::GetColumnModel() const
{
	return m_model->GetColumnModel();
}

void BookmarkListPresenter::ToggleColumn(BookmarkColumn column)
{
	auto *columnModel = m_model->GetColumnModel();
	auto columnId = BookmarkColumnModel::BookmarkColumnToColumnId(column);
	columnModel->SetColumnVisible(columnId, !columnModel->IsColumnVisible(columnId));
}

void BookmarkListPresenter::OnBeginDrag(const std::vector<ListViewItem *> &items)
{
	auto rawBookmarkItems = GetBookmarksForItems(items);

	if (rawBookmarkItems.empty())
	{
		return;
	}

	OwnedRefBookmarkItems bookmarkItems;

	for (auto &rawBookmarkItem : rawBookmarkItems)
	{
		auto &ownedPtr = rawBookmarkItem->GetParent()->GetChildOwnedPtr(rawBookmarkItem);
		bookmarkItems.push_back(ownedPtr);
	}

	auto dataObject = BookmarkDataExchange::CreateDataObject(bookmarkItems);

	wil::com_ptr_nothrow<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_ALL,
		IID_PPV_ARGS(&dragSourceHelper));

	if (SUCCEEDED(hr))
	{
		dragSourceHelper->InitializeFromWindow(m_view->GetHWND(), nullptr, dataObject.get());
	}

	DWORD effect;
	auto dropSource = winrt::make_self<DropSourceImpl>();
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);
}

BookmarkListPresenter::DropLocation BookmarkListPresenter::GetDropLocation(const POINT &pt)
{
	POINT ptClient = pt;
	ScreenToClient(m_view->GetHWND(), &ptClient);

	auto *item = m_view->MaybeGetItemAtPoint(ptClient);

	BookmarkItem *parentFolder = nullptr;
	size_t position;
	bool parentFolderSelected = false;

	if (item)
	{
		auto *bookmarkItem = m_model->GetBookmarkForItem(item);
		auto itemRect = m_view->GetItemRect(item);
		int itemIndex = m_model->GetItemIndex(item);

		if (bookmarkItem->IsFolder())
		{
			RECT folderCentralRect = itemRect;
			int indent =
				static_cast<int>(FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE * GetRectHeight(&itemRect));
			InflateRect(&folderCentralRect, 0, -indent);

			if (ptClient.y < folderCentralRect.top)
			{
				parentFolder = m_currentBookmarkFolder;
				position = itemIndex;
			}
			else if (ptClient.y > folderCentralRect.bottom)
			{
				parentFolder = m_currentBookmarkFolder;
				position = itemIndex + 1;
			}
			else
			{
				parentFolder = bookmarkItem;
				position = bookmarkItem->GetChildren().size();
				parentFolderSelected = true;
			}
		}
		else
		{
			parentFolder = m_currentBookmarkFolder;
			position = itemIndex;

			if (ptClient.y > (itemRect.top + GetRectHeight(&itemRect) / 2))
			{
				position++;
			}
		}
	}
	else
	{
		parentFolder = m_currentBookmarkFolder;
		position = m_view->FindNextItemIndex(ptClient);
	}

	return { parentFolder, position, parentFolderSelected };
}

void BookmarkListPresenter::UpdateUiForDropLocation(const DropLocation &dropLocation)
{
	ResetDropUiState();

	if (m_model->GetNumItems() == 0)
	{
		// In this case, there's nothing that needs to be done. The dragged item can't be over a
		// view item, since there are no view items. And there's no need to show an insert mark,
		// since an insert mark is only useful for delineating positions between view items.
		return;
	}

	if (dropLocation.parentFolder == m_currentBookmarkFolder)
	{
		auto finalIndex = static_cast<int>(dropLocation.position);
		InsertMarkPosition markPosition;

		if (finalIndex == m_model->GetNumItems())
		{
			finalIndex--;
			markPosition = InsertMarkPosition::After;
		}
		else
		{
			markPosition = InsertMarkPosition::Before;
		}

		m_view->ShowInsertMark(m_model->GetItemAtIndex(finalIndex), markPosition);
	}
	else
	{
		m_view->HighlightItem(m_model->GetItemForBookmark(dropLocation.parentFolder));
		m_highlightedItemGuid = dropLocation.parentFolder->GetGUID();
	}
}

void BookmarkListPresenter::ResetDropUiState()
{
	m_view->RemoveInsertMark();
	RemoveDropHighlight();
}

void BookmarkListPresenter::RemoveDropHighlight()
{
	if (!m_highlightedItemGuid)
	{
		return;
	}

	auto *bookmarkItem = m_bookmarkTree->MaybeGetBookmarkItemById(*m_highlightedItemGuid);

	if (bookmarkItem)
	{
		m_view->UnhighlightItem(m_model->GetItemForBookmark(bookmarkItem));
	}

	m_highlightedItemGuid.reset();
}

ListViewDelegate *BookmarkListPresenter::GetDelegateForTesting()
{
	CHECK(IsInTest());
	return this;
}

BookmarkListViewModel *BookmarkListPresenter::GetModelForTesting()
{
	return const_cast<BookmarkListViewModel *>(std::as_const(*this).GetModelForTesting());
}

const BookmarkListViewModel *BookmarkListPresenter::GetModelForTesting() const
{
	CHECK(IsInTest());
	return m_model.get();
}
