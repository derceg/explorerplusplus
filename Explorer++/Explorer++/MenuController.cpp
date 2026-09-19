// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuController.h"
#include "MenuDelegate.h"
#include "MenuView.h"
#include "TestHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/MenuHelpTextHost.h"
#include "../Helper/WindowSubclass.h"

MenuController::MenuController(MenuView *rootView, MenuHelpTextHost *menuHelpTextHost) :
	m_rootView(rootView),
	m_menuHelpTextHost(menuHelpTextHost)
{
	// This class is designed to be used when a top-level menu is displayed. That will typically be
	// a popup menu, but it can also be a submenu of a menu not managed with MenuView.
	DCHECK(m_rootView->IsRoot());
}

MenuController::~MenuController() = default;

void MenuController::NotifyMenuWillShow(HWND ownerWindow)
{
	DCHECK(!m_subclass);
	m_subclass = std::make_unique<WindowSubclass>(ownerWindow,
		std::bind_front(&MenuController::OwnerWindowSubclass, this));

	NotifyMenuWillShowForDpi(DpiCompatibility::GetInstance().GetDpiForWindow(ownerWindow));
}

void MenuController::NotifyMenuWillShowForDpi(UINT dpi)
{
	DCHECK_GT(m_rootView->GetNumItems(), 0);

	m_helpTextConnection = m_menuHelpTextHost->AddMenuHelpTextRequestObserver(
		std::bind_front(&MenuController::OnHelpTextRequested, this));

	// When the associated menu is a submenu of a menu not managed with MenuView, the window
	// subclass that's installed will be set up too late to catch the WM_INITMENUPOPUP message.
	// Manually invoking this method here ensures that it's always called.
	m_rootView->OnPopupWillShowForDpi(dpi);
}

LRESULT MenuController::OwnerWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITMENUPOPUP:
		OnInitMenuPopup(hwnd, reinterpret_cast<HMENU>(wParam));
		break;

	case WM_UNINITMENUPOPUP:
		OnUninitMenuPopup(reinterpret_cast<HMENU>(wParam));
		break;

	case WM_MENUSELECT:
		m_menuHelpTextHost->MenuItemSelected(reinterpret_cast<HMENU>(lParam), LOWORD(wParam),
			HIWORD(wParam));
		break;

	case WM_MBUTTONUP:
		OnMiddleButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			WI_IsFlagSet(wParam, MK_CONTROL), WI_IsFlagSet(wParam, MK_SHIFT));
		break;

	case WM_MENURBUTTONUP:
		OnMenuRightButtonUp(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam));
		break;

	case WM_MENUDRAG:
		return OnMenuDrag(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam));

	case WM_MENUGETOBJECT:
		return OnMenuGetObject(reinterpret_cast<MENUGETOBJECTINFO *>(lParam));
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void MenuController::OnInitMenuPopup(HWND hwnd, HMENU menu)
{
	auto *view = m_rootView->MaybeGetMenuViewForNativeMenu(menu);

	if (!view)
	{
		return;
	}

	view->OnPopupWillShowForDpi(DpiCompatibility::GetInstance().GetDpiForWindow(hwnd));
}

void MenuController::OnUninitMenuPopup(HMENU menu)
{
	auto *view = m_rootView->MaybeGetMenuViewForNativeMenu(menu);

	if (!view)
	{
		return;
	}

	view->OnPopupClosed();

	if (view == m_rootView)
	{
		OnMenuClosed();
	}
}

void MenuController::OnMenuClosed()
{
	m_dropTarget.reset();
	m_subclass.reset();
	m_helpTextConnection.disconnect();
}

void MenuController::OnMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto menuItemId = m_rootView->MaybeGetItemAtPoint(pt);

	if (!menuItemId)
	{
		return;
	}

	MiddleClickItem(*menuItemId, isCtrlKeyDown, isShiftKeyDown);
}

void MenuController::OnMenuRightButtonUp(HMENU menu, int index)
{
	auto *view = m_rootView->MaybeGetMenuViewForNativeMenu(menu);

	if (!view)
	{
		return;
	}

	DWORD messagePos = GetMessagePos();
	POINT ptScreen = { GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos) };
	RightClickItem(view->GetItemId(index), ptScreen);
}

LRESULT MenuController::OnMenuDrag(HMENU menu, int index)
{
	// It appears that dragging at the very top of the menu (above the first item) or at the very
	// bottom of the menu (below the last item) will trigger a WM_MENUDRAG message, with a position
	// of -1. It doesn't make sense to begin a drag here in that case, though, since no actual item
	// is being dragged.
	if (index == -1)
	{
		return MND_CONTINUE;
	}

	auto *view = m_rootView->MaybeGetMenuViewForNativeMenu(menu);

	if (!view)
	{
		return MND_CONTINUE;
	}

	UINT id = view->GetItemId(index);
	auto *delegate = MaybeGetDelegateForActionableItem(id);

	if (!delegate)
	{
		return MND_CONTINUE;
	}

	auto action = delegate->OnItemDragged(id);
	return action == MenuDragAction::ContinueMenu ? MND_CONTINUE : MND_ENDMENU;
}

LRESULT MenuController::OnMenuGetObject(MENUGETOBJECTINFO *objectInfo)
{
	auto *view = m_rootView->MaybeGetMenuViewForNativeMenu(objectInfo->hmenu);

	if (!view)
	{
		return MNGO_NOINTERFACE;
	}

	auto requestedIid = static_cast<const IID *>(objectInfo->riid);

	if (!IsEqualIID(*requestedIid, IID_IDropTarget))
	{
		return MNGO_NOINTERFACE;
	}

	MenuDropLocation dropLocation;

	// If neither MNGOF_TOPGAP or MNGOF_BOTTOMGAP is set, the cursor is over an item.
	if (WI_AreAllFlagsClear(objectInfo->dwFlags, MNGOF_TOPGAP | MNGOF_BOTTOMGAP))
	{
		dropLocation = { view->GetItemId(objectInfo->uPos), MenuDropLocation::Position::On };
	}
	else
	{
		// The documentation for the MENUGETOBJECTINFO structure appears to be worded somewhat
		// misleadingly. It states that MNGOF_TOPGAP will be set if "The mouse is on the top of the
		// item indicated by uPos.", while MNGOF_BOTTOMGAP will be set if "The mouse is on the
		// bottom of the item indicated by uPos.". If there are, for example, 3 items in the menu
		// and the source is dragged between the second and third items:
		//
		// A
		// B
		// <-- Drag position
		// C
		//
		// The following values will be set:
		// dwFlags = MNGOF_BOTTOMGAP
		// uPos = 2
		//
		// That doesn't really align with the documentation, since the cursor is not at the bottom
		// of item 2. It can be considered to be at the bottom of item 1 or the top of item 2, but
		// it can't be at the bottom of item 2.
		//
		// On the other hand, if the cursor is at the top of the first item:
		//
		// <-- Drag position
		// A
		// B
		// C
		//
		// The following values will be set:
		// dwFlags = MNGOF_TOPGAP
		// uPos = 0
		//
		// Which matches the explanation given in the documentation.
		//
		// Ultimately, it appears the uPos indicates the target drop position and
		// MNGOF_TOPGAP/MNGOF_BOTTOMGAP can be effectively ignored (since the relative position has
		// already been incorporated into uPos).
		if (objectInfo->uPos == static_cast<UINT>(view->GetNumItems()))
		{
			// It's not considered valid for a menu to have no items, so this shouldn't trigger.
			CHECK(view->GetNumItems() != 0);

			dropLocation = { view->GetItemId(objectInfo->uPos - 1),
				MenuDropLocation::Position::After };
		}
		else
		{
			dropLocation = { view->GetItemId(objectInfo->uPos),
				MenuDropLocation::Position::Before };
		}
	}

	auto *delegate = view->MaybeGetDelegate();

	if (!delegate)
	{
		return MNGO_NOINTERFACE;
	}

	// This represents the drop target for the specified item. It needs to be held either until
	// another target is requested, or the menu is closed.
	m_dropTarget = delegate->MaybeGetDropTargetForLocation(dropLocation);

	if (!m_dropTarget)
	{
		return MNGO_NOINTERFACE;
	}

	objectInfo->pvObj = m_dropTarget.get();

	return MNGO_NOERROR;
}

std::optional<std::wstring> MenuController::OnHelpTextRequested(HMENU menu, int id)
{
	auto *view = m_rootView->MaybeGetMenuViewForNativeMenu(menu);

	if (!view)
	{
		return std::nullopt;
	}

	return view->GetItemHelpText(id);
}

void MenuController::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto *delegate = MaybeGetDelegateForActionableItem(id);

	if (!delegate)
	{
		return;
	}

	delegate->OnItemSelected(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuController::MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto *delegate = MaybeGetDelegateForActionableItem(id);

	if (!delegate)
	{
		return;
	}

	delegate->OnItemMiddleClicked(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuController::RightClickItem(UINT id, const POINT &ptScreen)
{
	auto *view = m_rootView->MaybeGetMenuViewForItem(id);
	CHECK(view);

	auto *delegate = view->MaybeGetDelegate();

	if (!delegate)
	{
		return;
	}

	delegate->OnItemRightClicked(id, ptScreen);
}

MenuDelegate *MenuController::MaybeGetDelegateForActionableItem(UINT id)
{
	auto *view = m_rootView->MaybeGetMenuViewForItem(id);
	CHECK(view);

	if (!view->IsItemEnabled(id))
	{
		return nullptr;
	}

	return view->MaybeGetDelegate();
}

void MenuController::NotifyMenuWillShowForTesting(UINT dpi)
{
	CHECK(IsInTest());

	NotifyMenuWillShowForDpi(dpi);
}

void MenuController::NotifyMenuClosedForTesting()
{
	CHECK(IsInTest());

	OnMenuClosed();
}
