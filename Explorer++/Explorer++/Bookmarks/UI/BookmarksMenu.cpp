// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarksMenu.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/UI/BookmarkMenuDropTarget.h"
#include "BrowserWindow.h"
#include "IconModel.h"
#include "MainResource.h"
#include "MenuView.h"
#include "PopupMenuRunner.h"
#include "ResourceLoader.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/ImageHelper.h"
#include <format>

namespace
{

class BookmarkIconModel : public IconModel
{
public:
	BookmarkIconModel(BookmarkIconManager *bookmarkIconManager,
		WeakPtr<BookmarkItem, const BookmarkItem> bookmarkItem);

	// IconModel
	wil::unique_hbitmap GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const override;

private:
	wil::unique_hbitmap MaybeGetBitmapForIconIndex(int iconIndex) const;

	BookmarkIconManager *const m_bookmarkIconManager;
	const WeakPtr<BookmarkItem, const BookmarkItem> m_bookmarkItem;

	WeakPtrFactory<BookmarkIconModel> m_weakPtrFactory{ this };
};

}

BookmarksMenu::BookmarksMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	BookmarkTree *bookmarkTree, BookmarkItem *bookmarkFolder,
	BookmarkIconManager *bookmarkIconManager, BrowserWindow *browser, HWND parentWindow,
	PlatformContext *platformContext, const ResourceLoader *resourceLoader,
	BookmarkMenuBuilder::IncludePredicate includePredicate, UINT startId, UINT endId) :
	MenuBase(menuView, acceleratorManager, startId, endId),
	m_idCounter(startId),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkIconManager(bookmarkIconManager),
	m_browser(browser),
	m_parentWindow(parentWindow),
	m_platformContext(platformContext),
	m_resourceLoader(resourceLoader)
{
	DCHECK(bookmarkFolder->IsFolder());

	m_menuView->EnableDragAndDrop(true);

	BuildMenu(m_menuView, bookmarkFolder, includePredicate);
}

UINT BookmarksMenu::GetNextId() const
{
	return m_idCounter;
}

void BookmarksMenu::BuildMenu(MenuView *menuView, BookmarkItem *bookmarkFolder,
	BookmarkMenuBuilder::IncludePredicate includePredicate)
{
	if (bookmarkFolder->GetChildren().empty())
	{
		AddEmptyItem(menuView, bookmarkFolder);
		return;
	}

	for (const auto &childItem : bookmarkFolder->GetChildren())
	{
		if (includePredicate && !includePredicate(childItem.get()))
		{
			continue;
		}

		UINT id = m_idCounter++;

		if (id >= GetIdRange().endId)
		{
			return;
		}

		auto iconModel = std::make_unique<BookmarkIconModel>(m_bookmarkIconManager,
			static_cast<const BookmarkItem *>(childItem.get())->GetWeakPtr());

		if (childItem->IsFolder())
		{
			auto *subMenuView =
				menuView->AppendSubMenu(this, id, childItem->GetName(), std::move(iconModel));
			BuildMenu(subMenuView, childItem.get());
		}
		else
		{
			menuView->AppendItem(this, id, childItem->GetName(), std::move(iconModel),
				childItem->GetLocation());
		}

		m_idToBookmarkMap.insert({ id, { childItem->GetWeakPtr(), MenuItemType::BookmarkItem } });
	}
}

void BookmarksMenu::AddEmptyItem(MenuView *menuView, BookmarkItem *bookmarkFolder)
{
	UINT id = m_idCounter++;

	if (id >= GetIdRange().endId)
	{
		return;
	}

	std::wstring menuText =
		std::format(L"({})", m_resourceLoader->LoadString(IDS_BOOKMARK_FOLDER_EMPTY));
	menuView->AppendItem(this, id, menuText);
	menuView->EnableItem(id, false);

	// This effectively maps the empty item to the parent folder, so that operations on this item
	// will occur on the parent.
	m_idToBookmarkMap.insert({ id, { bookmarkFolder->GetWeakPtr(), MenuItemType::EmptyItem } });
}

void BookmarksMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	const auto *bookmarkItem = MaybeGetBookmarkItemForMenuItem(id);

	if (!bookmarkItem)
	{
		return;
	}

	DCHECK(bookmarkItem->IsBookmark());

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(false, isCtrlKeyDown, isShiftKeyDown), m_browser);
}

void BookmarksMenu::OnItemMiddleClicked(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	const auto *bookmarkItem = MaybeGetBookmarkItemForMenuItem(id);

	if (!bookmarkItem)
	{
		return;
	}

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(true, isCtrlKeyDown, isShiftKeyDown), m_browser);
}

void BookmarksMenu::OnItemRightClicked(UINT id, const POINT &ptScreen)
{
	auto *bookmarkItem = MaybeGetBookmarkItemForMenuItem(id);

	if (!bookmarkItem)
	{
		return;
	}

	PopupMenuRunner popupRunner(m_browser);
	BookmarkContextMenu contextMenu(popupRunner.GetView(), m_acceleratorManager, m_bookmarkTree,
		{ bookmarkItem }, m_resourceLoader, m_browser, m_parentWindow, m_platformContext);
	popupRunner.Show(m_parentWindow, ptScreen);
}

MenuDragAction BookmarksMenu::OnItemDragged(UINT id)
{
	auto *entry = GetEntryForMenuItem(id);
	auto &weakBookmarkItem = entry->bookmarkItem;

	if (entry->menuItemType == MenuItemType::EmptyItem || !weakBookmarkItem)
	{
		return MenuDragAction::ContinueMenu;
	}

	auto *bookmarkItem = weakBookmarkItem.Get();

	auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);
	auto dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

	auto dropSource = winrt::make_self<DropSourceImpl>();

	DWORD effect = DROPEFFECT_NONE;
	HRESULT hr = DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);

	if (FAILED(hr) || hr == DRAGDROP_S_CANCEL || effect == DROPEFFECT_NONE)
	{
		return MenuDragAction::ContinueMenu;
	}

	return MenuDragAction::CloseMenu;
}

wil::com_ptr_nothrow<IDropTarget> BookmarksMenu::MaybeGetDropTargetForLocation(
	const MenuDropLocation &dropLocation)
{
	auto *entry = GetEntryForMenuItem(dropLocation.id);
	auto &weakBookmarkItem = entry->bookmarkItem;

	if (!weakBookmarkItem)
	{
		return nullptr;
	}

	auto *bookmarkItem = weakBookmarkItem.Get();

	BookmarkItem *targetFolder = nullptr;
	size_t targetIndex;

	if (entry->menuItemType == MenuItemType::EmptyItem)
	{
		targetFolder = bookmarkItem;
		targetIndex = 0;
	}
	else if (dropLocation.position == MenuDropLocation::Position::On && bookmarkItem->IsFolder())
	{
		targetFolder = bookmarkItem;
		targetIndex = targetFolder->GetChildren().size();
	}
	else
	{
		targetFolder = bookmarkItem->GetParent();
		targetIndex = targetFolder->GetChildIndex(bookmarkItem);

		if (dropLocation.position == MenuDropLocation::Position::After)
		{
			targetIndex++;
		}
	}

	auto dropTarget =
		winrt::make_self<BookmarkMenuDropTarget>(targetFolder, targetIndex, m_bookmarkTree);

	return dropTarget.get();
}

BookmarkItem *BookmarksMenu::MaybeGetBookmarkItemForMenuItem(UINT id)
{
	auto *entry = GetEntryForMenuItem(id);
	auto &weakBookmarkItem = entry->bookmarkItem;

	if (!weakBookmarkItem)
	{
		return nullptr;
	}

	return weakBookmarkItem.Get();
}

const BookmarksMenu::MenuItemEntry *BookmarksMenu::GetEntryForMenuItem(UINT id) const
{
	auto itr = m_idToBookmarkMap.find(id);
	CHECK(itr != m_idToBookmarkMap.end());
	return &itr->second;
}

namespace
{

BookmarkIconModel::BookmarkIconModel(BookmarkIconManager *bookmarkIconManager,
	WeakPtr<BookmarkItem, const BookmarkItem> bookmarkItem) :
	m_bookmarkIconManager(bookmarkIconManager),
	m_bookmarkItem(bookmarkItem)
{
}

wil::unique_hbitmap BookmarkIconModel::GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const
{
	// TODO: The requested DPI should be taken into account.
	UNREFERENCED_PARAMETER(dpi);

	if (!m_bookmarkItem)
	{
		return nullptr;
	}

	int iconIndex = m_bookmarkIconManager->GetBookmarkItemIconIndex(m_bookmarkItem.Get(),
		[weakSelf = m_weakPtrFactory.GetWeakPtr(), updateCallback](int updatedIconIndex)
		{
			if (!weakSelf)
			{
				return;
			}

			auto bitmap = weakSelf->MaybeGetBitmapForIconIndex(updatedIconIndex);

			if (!bitmap)
			{
				return;
			}

			updateCallback(std::move(bitmap));
		});

	return MaybeGetBitmapForIconIndex(iconIndex);
}

wil::unique_hbitmap BookmarkIconModel::MaybeGetBitmapForIconIndex(int iconIndex) const
{
	wil::com_ptr_nothrow<IImageList> imageList;
	HRESULT hr =
		HIMAGELIST_QueryInterface(m_bookmarkIconManager->GetImageList(), IID_PPV_ARGS(&imageList));

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::unique_hbitmap bitmap;
	hr = ImageHelper::CreateHBITMAPFromImageListIcon(imageList.get(), iconIndex, bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return bitmap;
}

}
