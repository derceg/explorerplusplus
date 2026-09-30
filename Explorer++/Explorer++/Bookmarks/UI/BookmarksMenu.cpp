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
	IncludePredicate includePredicate) :
	MenuBase(menuView, acceleratorManager),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkIconManager(bookmarkIconManager),
	m_browser(browser),
	m_parentWindow(parentWindow),
	m_platformContext(platformContext),
	m_resourceLoader(resourceLoader)
{
	DCHECK(bookmarkFolder->IsFolder());

	m_rootMenuView->EnableDragAndDrop(true);

	BuildMenu(m_rootMenuView, bookmarkFolder, includePredicate);
}

void BookmarksMenu::BuildMenu(MenuView *menuView, BookmarkItem *bookmarkFolder,
	IncludePredicate includePredicate)
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

		AddBookmarkItem(menuView, childItem.get());
	}
}

void BookmarksMenu::AddEmptyItem(MenuView *menuView, BookmarkItem *bookmarkFolder)
{
	auto weakBookmarkFolder = bookmarkFolder->GetWeakPtr();
	menuView->AppendItem(MenuItemParams(
		std::format(L"({})", m_resourceLoader->LoadString(IDS_BOOKMARK_FOLDER_EMPTY)))
			.SetEnabledCallback([] { return false; })
			.SetRightClickedCallback(std::bind_front(&BookmarksMenu::OnBookmarkItemRightClicked,
				this, weakBookmarkFolder))
			.SetMaybeGetDropTargetCallback(std::bind_front(&BookmarksMenu::MaybeGetDropTarget, this,
				weakBookmarkFolder, MenuItemType::EmptyItem)));
}

void BookmarksMenu::AddBookmarkItem(MenuView *menuView, BookmarkItem *bookmarkItem)
{
	auto iconModel = std::make_unique<BookmarkIconModel>(m_bookmarkIconManager,
		static_cast<const BookmarkItem *>(bookmarkItem)->GetWeakPtr());
	auto weakBookmarkItem = bookmarkItem->GetWeakPtr();

	if (bookmarkItem->IsFolder())
	{
		auto *subMenuView = menuView->AppendSubMenu(SubMenuItemParams(bookmarkItem->GetName())
				.SetIconModel(std::move(iconModel))
				.SetMiddleClickedCallback(std::bind_front(
					&BookmarksMenu::OnBookmarkItemMiddleClicked, this, weakBookmarkItem))
				.SetRightClickedCallback(std::bind_front(&BookmarksMenu::OnBookmarkItemRightClicked,
					this, weakBookmarkItem))
				.SetDraggedCallback(
					std::bind_front(&BookmarksMenu::OnBookmarkItemDragged, this, weakBookmarkItem))
				.SetMaybeGetDropTargetCallback(std::bind_front(&BookmarksMenu::MaybeGetDropTarget,
					this, weakBookmarkItem, MenuItemType::BookmarkItem)));

		BuildMenu(subMenuView, bookmarkItem);
	}
	else
	{
		menuView->AppendItem(MenuItemParams(bookmarkItem->GetName())
				.SetIconModel(std::move(iconModel))
				.SetHelpText(bookmarkItem->GetLocation())
				.SetSelectedCallback(
					std::bind_front(&BookmarksMenu::OnBookmarkSelected, this, weakBookmarkItem))
				.SetMiddleClickedCallback(std::bind_front(
					&BookmarksMenu::OnBookmarkItemMiddleClicked, this, weakBookmarkItem))
				.SetRightClickedCallback(std::bind_front(&BookmarksMenu::OnBookmarkItemRightClicked,
					this, weakBookmarkItem))
				.SetDraggedCallback(
					std::bind_front(&BookmarksMenu::OnBookmarkItemDragged, this, weakBookmarkItem))
				.SetMaybeGetDropTargetCallback(std::bind_front(&BookmarksMenu::MaybeGetDropTarget,
					this, weakBookmarkItem, MenuItemType::BookmarkItem)));
	}
}

void BookmarksMenu::OnBookmarkSelected(WeakPtr<BookmarkItem> bookmark, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	if (!bookmark)
	{
		return;
	}

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmark.Get(),
		DetermineOpenDisposition(false, isCtrlKeyDown, isShiftKeyDown), m_browser);
}

void BookmarksMenu::OnBookmarkItemMiddleClicked(WeakPtr<BookmarkItem> bookmarkItem,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!bookmarkItem)
	{
		return;
	}

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem.Get(),
		DetermineOpenDisposition(true, isCtrlKeyDown, isShiftKeyDown), m_browser);
}

void BookmarksMenu::OnBookmarkItemRightClicked(WeakPtr<BookmarkItem> bookmarkItem,
	const POINT &ptScreen)
{
	if (!bookmarkItem)
	{
		return;
	}

	PopupMenuRunner popupRunner(m_parentWindow, m_browser);
	BookmarkContextMenu contextMenu(popupRunner.GetView(), m_acceleratorManager, m_bookmarkTree,
		{ bookmarkItem.Get() }, m_resourceLoader, m_browser, m_parentWindow, m_platformContext);
	popupRunner.Show(ptScreen);
}

MenuDragAction BookmarksMenu::OnBookmarkItemDragged(WeakPtr<BookmarkItem> bookmarkItem)
{
	if (!bookmarkItem)
	{
		return MenuDragAction::ContinueMenu;
	}

	auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem.Get());
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

wil::com_ptr_nothrow<IDropTarget> BookmarksMenu::MaybeGetDropTarget(
	WeakPtr<BookmarkItem> bookmarkItem, MenuItemType itemType, MenuDropPosition position)
{
	if (!bookmarkItem)
	{
		return nullptr;
	}

	BookmarkItem *targetFolder = nullptr;
	size_t targetIndex;

	if (itemType == MenuItemType::EmptyItem)
	{
		targetFolder = bookmarkItem.Get();
		targetIndex = 0;
	}
	else if (position == MenuDropPosition::On && bookmarkItem->IsFolder())
	{
		targetFolder = bookmarkItem.Get();
		targetIndex = targetFolder->GetChildren().size();
	}
	else
	{
		targetFolder = bookmarkItem->GetParent();
		targetIndex = targetFolder->GetChildIndex(bookmarkItem.Get());

		if (position == MenuDropPosition::After)
		{
			targetIndex++;
		}
	}

	auto dropTarget =
		winrt::make_self<BookmarkMenuDropTarget>(targetFolder, targetIndex, m_bookmarkTree);

	return dropTarget.get();
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
