// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarksToolbar.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "NavigationHelper.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"
#include <glog/logging.h>
#include <wil/com.h>
#include <format>

class BookmarksToolbarBookmarkButton : public ToolbarButton
{
public:
	BookmarksToolbarBookmarkButton(const BookmarkItem *bookmarkItem,
		BookmarkIconManager *bookmarkIconManager, ClickedCallback clickedCallback) :
		ToolbarButton(clickedCallback),
		m_bookmarkItem(bookmarkItem),
		m_bookmarkIconManager(bookmarkIconManager),
		m_destroyed(std::make_shared<bool>(false))
	{
		DCHECK(bookmarkItem->IsBookmark());
	}

	~BookmarksToolbarBookmarkButton()
	{
		*m_destroyed = true;
	}

	std::wstring GetText() const override
	{
		return m_bookmarkItem->GetName();
	}

	std::wstring GetTooltipText() const override
	{
		return std::format(L"{}\n{}", m_bookmarkItem->GetName(), m_bookmarkItem->GetLocation());
	}

	std::optional<int> GetImageIndex() const override
	{
		if (m_iconIndex)
		{
			return *m_iconIndex;
		}

		m_iconIndex = m_bookmarkIconManager->GetBookmarkItemIconIndex(m_bookmarkItem,
			[this, destroyed = m_destroyed](int iconIndex)
			{
				// This method is called on the main thread, so it's safe to access the shared
				// destroyed variable here.
				if (*destroyed)
				{
					return;
				}

				OnIconLoaded(iconIndex);
			});

		return *m_iconIndex;
	}

	void OnIconLoaded(int iconIndex) const
	{
		m_iconIndex = iconIndex;

		NotifyParentOfUpdate();
	}

	void InvalidateCachedIcon()
	{
		m_iconIndex.reset();
	}

private:
	const BookmarkItem *m_bookmarkItem;
	BookmarkIconManager *m_bookmarkIconManager;

	// Stores the cached icon index.
	mutable std::optional<int> m_iconIndex;

	std::shared_ptr<bool> m_destroyed;
};

class BookmarksToolbarFolderButton : public ToolbarMenuButton
{
public:
	BookmarksToolbarFolderButton(const BookmarkItem *bookmarkItem,
		BookmarkIconManager *bookmarkIconManager, ClickedCallback clickedCallback) :
		ToolbarMenuButton(clickedCallback),
		m_bookmarkItem(bookmarkItem),
		m_bookmarkIconManager(bookmarkIconManager)
	{
		DCHECK(bookmarkItem->IsFolder());
	}

	std::wstring GetText() const override
	{
		return m_bookmarkItem->GetName();
	}

	std::wstring GetTooltipText() const override
	{
		return {};
	}

	std::optional<int> GetImageIndex() const override
	{
		return m_bookmarkIconManager->GetBookmarkItemIconIndex(m_bookmarkItem);
	}

private:
	const BookmarkItem *m_bookmarkItem;
	BookmarkIconManager *m_bookmarkIconManager;
};

BookmarksToolbar *BookmarksToolbar::Create(BookmarksToolbarView *view, BrowserWindow *browserWindow,
	CoreInterface *coreInterface, IconFetcher *iconFetcher, BookmarkTree *bookmarkTree)
{
	return new BookmarksToolbar(view, browserWindow, coreInterface, iconFetcher, bookmarkTree);
}

BookmarksToolbar::BookmarksToolbar(BookmarksToolbarView *view, BrowserWindow *browserWindow,
	CoreInterface *coreInterface, IconFetcher *iconFetcher, BookmarkTree *bookmarkTree) :
	BookmarkDropTargetWindow(view->GetHWND(), bookmarkTree),
	m_view(view),
	m_browserWindow(browserWindow),
	m_coreInterface(coreInterface),
	m_bookmarkTree(bookmarkTree),
	m_contextMenu(bookmarkTree, coreInterface->GetResourceInstance(), browserWindow, coreInterface),
	m_bookmarkMenu(bookmarkTree, coreInterface->GetResourceInstance(), browserWindow, coreInterface,
		iconFetcher, view->GetHWND())
{
	Initialize(iconFetcher);
}

void BookmarksToolbar::Initialize(IconFetcher *iconFetcher)
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_view->GetHWND());
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	m_bookmarkIconManager =
		std::make_unique<BookmarkIconManager>(m_coreInterface, iconFetcher, iconWidth, iconHeight);

	m_view->SetImageList(m_bookmarkIconManager->GetImageList());

	AddBookmarkItems();

	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind_front(&BookmarksToolbar::OnBookmarkItemAdded, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind_front(&BookmarksToolbar::OnBookmarkItemUpdated, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarksToolbar::OnBookmarkItemMoved, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind_front(&BookmarksToolbar::OnBookmarkItemPreRemoval, this)));

	m_connections.push_back(m_coreInterface->AddToolbarContextMenuObserver(
		std::bind_front(&BookmarksToolbar::OnToolbarContextMenuPreShow, this)));
	m_connections.push_back(m_coreInterface->AddToolbarContextMenuSelectedObserver(
		std::bind_front(&BookmarksToolbar::OnToolbarContextMenuItemSelected, this)));

	m_view->AddWindowDestroyedObserver(std::bind_front(&BookmarksToolbar::OnWindowDestroyed, this));
}

void BookmarksToolbar::AddBookmarkItems()
{
	size_t index = 0;

	for (const auto &bookmarkItem : m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren())
	{
		AddBookmarkItem(bookmarkItem.get(), index);

		index++;
	}
}

void BookmarksToolbar::AddBookmarkItem(BookmarkItem *bookmarkItem, size_t index)
{
	std::unique_ptr<ToolbarButton> button;

	if (bookmarkItem->IsBookmark())
	{
		button = std::make_unique<BookmarksToolbarBookmarkButton>(bookmarkItem,
			m_bookmarkIconManager.get(),
			std::bind_front(&BookmarksToolbar::OnBookmarkClicked, this, bookmarkItem));
	}
	else
	{
		button = std::make_unique<BookmarksToolbarFolderButton>(bookmarkItem,
			m_bookmarkIconManager.get(),
			std::bind_front(&BookmarksToolbar::OnBookmarkFolderClicked, this, bookmarkItem));
	}

	button->SetMiddleClickedCallback(
		std::bind_front(&BookmarksToolbar::OnButtonMiddleClicked, this, bookmarkItem));
	button->SetRightClickedCallback(
		std::bind_front(&BookmarksToolbar::OnButtonRightClicked, this, bookmarkItem));
	button->SetDragStartedCallback(
		std::bind_front(&BookmarksToolbar::OnButtonDragStarted, this, bookmarkItem));

	m_view->AddButton(std::move(button), index);
}

void BookmarksToolbar::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	if (bookmarkItem.GetParent() != m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		return;
	}

	AddBookmarkItem(&bookmarkItem, index);
}

void BookmarksToolbar::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem,
	BookmarkItem::PropertyType propertyType)
{
	if (bookmarkItem.GetParent() != m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		return;
	}

	auto index = bookmarkItem.GetParent()->GetChildIndex(&bookmarkItem);

	if (bookmarkItem.IsBookmark() && propertyType == BookmarkItem::PropertyType::Location)
	{
		auto button = static_cast<BookmarksToolbarBookmarkButton *>(m_view->GetButton(index));
		button->InvalidateCachedIcon();
	}

	m_view->UpdateButton(index);
}

void BookmarksToolbar::OnBookmarkItemMoved(BookmarkItem *bookmarkItem,
	const BookmarkItem *oldParent, size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
{
	if (oldParent == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		m_view->RemoveButton(oldIndex);
	}

	if (newParent == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		AddBookmarkItem(bookmarkItem, newIndex);
	}
}

void BookmarksToolbar::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (bookmarkItem.GetParent() != m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		return;
	}

	auto index = bookmarkItem.GetParent()->GetChildIndex(&bookmarkItem);
	m_view->RemoveButton(index);
}

void BookmarksToolbar::OnBookmarkClicked(BookmarkItem *bookmarkItem, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(false, event.ctrlKey, event.shiftKey), m_coreInterface,
		m_browserWindow);
}

void BookmarksToolbar::OnBookmarkFolderClicked(BookmarkItem *bookmarkItem, const MouseEvent &event)
{
	if (event.ctrlKey)
	{
		BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
			DetermineOpenDisposition(false, event.ctrlKey, event.shiftKey), m_coreInterface,
			m_browserWindow);
		return;
	}

	auto index = bookmarkItem->GetParent()->GetChildIndex(bookmarkItem);
	RECT buttonRect = m_view->GetButtonRect(index);

	POINT pt = { buttonRect.left, buttonRect.bottom };
	ClientToScreen(m_view->GetHWND(), &pt);

	m_bookmarkMenu.ShowMenu(bookmarkItem, pt);
}

void BookmarksToolbar::OnButtonMiddleClicked(const BookmarkItem *bookmarkItem,
	const MouseEvent &event)
{
	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(true, event.ctrlKey, event.shiftKey), m_coreInterface,
		m_browserWindow);
}

void BookmarksToolbar::OnButtonRightClicked(BookmarkItem *bookmarkItem, const MouseEvent &event)
{
	POINT ptScreen = event.ptClient;
	ClientToScreen(m_view->GetHWND(), &ptScreen);

	m_contextMenu.ShowMenu(m_view->GetHWND(), bookmarkItem->GetParent(), { bookmarkItem },
		ptScreen);
}

void BookmarksToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt)
{
	if (sourceWindow != m_view->GetHWND())
	{
		return;
	}

	std::wstring newBookmark = ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
		IDS_BOOKMARKS_TOOLBAR_NEW_BOOKMARK);
	MenuHelper::AddStringItem(menu, IDM_BT_NEWBOOKMARK, newBookmark, IDM_TOOLBARS_CUSTOMIZE, FALSE);

	std::wstring newBookmarkFolder = ResourceHelper::LoadString(
		m_coreInterface->GetResourceInstance(), IDS_BOOKMARKS_TOOLBAR_NEW_FOLDER);
	MenuHelper::AddStringItem(menu, IDM_BT_NEWFOLDER, newBookmarkFolder, IDM_TOOLBARS_CUSTOMIZE,
		FALSE);

	std::wstring paste = ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
		IDS_BOOKMARKS_TOOLBAR_PASTE);
	MenuHelper::AddStringItem(menu, IDM_BT_PASTE, paste, IDM_TOOLBARS_CUSTOMIZE, FALSE);

	if (!IsClipboardFormatAvailable(BookmarkClipboard::GetClipboardFormat()))
	{
		MenuHelper::EnableItem(menu, IDM_BT_PASTE, FALSE);
	}

	m_contextMenuLocation = pt;
}

void BookmarksToolbar::OnToolbarContextMenuItemSelected(HWND sourceWindow, int menuItemId)
{
	if (sourceWindow != m_view->GetHWND())
	{
		return;
	}

	CHECK(m_contextMenuLocation);

	POINT ptClient = *m_contextMenuLocation;
	ScreenToClient(m_view->GetHWND(), &ptClient);
	size_t targetIndex = m_view->FindNextButtonIndex(ptClient);

	switch (menuItemId)
	{
	case IDM_BT_NEWBOOKMARK:
		OnNewBookmarkItem(BookmarkItem::Type::Bookmark, targetIndex);
		break;

	case IDM_BT_NEWFOLDER:
		OnNewBookmarkItem(BookmarkItem::Type::Folder, targetIndex);
		break;

	case IDM_BT_PASTE:
		OnPaste(targetIndex);
		break;
	}

	m_contextMenuLocation.reset();
}

void BookmarksToolbar::OnNewBookmarkItem(BookmarkItem::Type type, size_t targetIndex)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type,
		m_bookmarkTree->GetBookmarksToolbarFolder(), targetIndex, m_view->GetHWND(),
		m_coreInterface);
}

void BookmarksToolbar::OnPaste(size_t targetIndex)
{
	BookmarkHelper::PasteBookmarkItems(m_bookmarkTree, m_bookmarkTree->GetBookmarksToolbarFolder(),
		targetIndex);
}

BookmarksToolbarView *BookmarksToolbar::GetView() const
{
	return m_view;
}

void BookmarksToolbar::ShowOverflowMenu(const POINT &ptScreen)
{
	m_bookmarkMenu.ShowMenu(m_bookmarkTree->GetBookmarksToolbarFolder(), ptScreen,
		[this](const BookmarkItem *bookmarkItem)
		{
			auto index = bookmarkItem->GetParent()->GetChildIndex(bookmarkItem);
			return !m_view->IsButtonVisible(index);
		});
}

void BookmarksToolbar::OnWindowDestroyed()
{
	delete this;
}

void BookmarksToolbar::OnButtonDragStarted(const BookmarkItem *bookmarkItem)
{
	auto dropSource = winrt::make_self<DropSourceImpl>();

	auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);
	auto dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

	wil::com_ptr_nothrow<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_ALL,
		IID_PPV_ARGS(&dragSourceHelper));

	if (FAILED(hr))
	{
		return;
	}

	// The image the toolbar generates below will be based on the hot item (i.e. whichever item the
	// cursor is over). That's an issue, as the drag is only started once the cursor has moved a
	// certain amount. By the time the cursor has moved enough to start a drag, it might be over
	// another toolbar item or not over the toolbar at all. The drag image would then either include
	// the wrong button, or no button at all (a default empty image would be used instead).
	// Setting the hot item here ensures the correct button is shown in the drag image in both of
	// those cases.
	m_view->SetHotItem(bookmarkItem->GetParent()->GetChildIndex(bookmarkItem));

	// The toolbar control has built-in handling for the DI_GETDRAGIMAGE message, so it will
	// generate the appropriate image, based on the hot item and current cursor position (the offset
	// parameter provided to the function below is ignored).
	dragSourceHelper->InitializeFromWindow(m_view->GetHWND(), nullptr, dataObject.get());

	DWORD effect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);
}

BookmarkDropTargetWindow::DropLocation BookmarksToolbar::GetDropLocation(const POINT &pt)
{
	POINT ptClient = pt;
	ScreenToClient(m_view->GetHWND(), &ptClient);

	auto index = m_view->MaybeGetIndexOfButtonAtPoint(ptClient);

	if (index)
	{
		RECT buttonRect = m_view->GetButtonRect(*index);

		auto bookmarkItem =
			m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren().at(*index).get();

		if (bookmarkItem->IsFolder())
		{
			RECT folderCentralRect = buttonRect;
			int indent =
				static_cast<int>(FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE * GetRectWidth(&buttonRect));
			InflateRect(&folderCentralRect, -indent, 0);

			if (ptClient.x < folderCentralRect.left)
			{
				return { m_bookmarkTree->GetBookmarksToolbarFolder(), *index, false };
			}
			else if (ptClient.x > folderCentralRect.right)
			{
				return { m_bookmarkTree->GetBookmarksToolbarFolder(), *index + 1, false };
			}
			else
			{
				return { bookmarkItem, bookmarkItem->GetChildren().size(), true };
			}
		}
		else
		{
			if (ptClient.x > (buttonRect.left + GetRectWidth(&buttonRect) / 2))
			{
				return { m_bookmarkTree->GetBookmarksToolbarFolder(), *index + 1, false };
			}
			else
			{
				return { m_bookmarkTree->GetBookmarksToolbarFolder(), *index, false };
			}
		}
	}
	else
	{
		auto nextIndex = m_view->FindNextButtonIndex(ptClient);
		return { m_bookmarkTree->GetBookmarksToolbarFolder(), nextIndex, false };
	}
}

void BookmarksToolbar::UpdateUiForDropLocation(const DropLocation &dropLocation)
{
	RemoveDropHighlight();

	if (dropLocation.parentFolder == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		m_view->ShowInsertMark(dropLocation.position);
	}
	else
	{
		m_view->RemoveInsertMark();

		auto selectedButtonIndex =
			dropLocation.parentFolder->GetParent()->GetChildIndex(dropLocation.parentFolder);

		auto button = m_view->GetButton(selectedButtonIndex);
		button->SetChecked(true);

		m_dropTargetFolder = dropLocation.parentFolder;
	}
}

void BookmarksToolbar::ResetDropUiState()
{
	m_view->RemoveInsertMark();
	RemoveDropHighlight();
}

void BookmarksToolbar::RemoveDropHighlight()
{
	if (m_dropTargetFolder)
	{
		auto index = m_dropTargetFolder->GetParent()->GetChildIndex(m_dropTargetFolder);

		auto button = m_view->GetButton(index);
		button->SetChecked(false);

		m_dropTargetFolder = nullptr;
	}
}
