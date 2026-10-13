// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RootMenuView.h"
#include "TestHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelpTextHost.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowSubclass.h"

RootMenuView::RootMenuView(wil::unique_hmenu menu, HWND ownerWindow, MenuType menuType,
	MenuHelpTextHost *menuHelpTextHost) :
	MenuView(std::move(menu)),
	m_menuType(menuType),
	m_menuHelpTextHost(menuHelpTextHost)
{
	// Not all menus support drag and drop, but for those that don't, the menu drag messages will
	// simply be ignored.
	MenuHelper::AddStyleFlags(m_menu, MNS_DRAGDROP);

	// ownerWindow can be null in tests only.
	CHECK(ownerWindow || IsInTest());

	if (ownerWindow)
	{
		m_subclass = std::make_unique<WindowSubclass>(ownerWindow,
			std::bind_front(&RootMenuView::OwnerWindowSubclass, this));
	}
}

RootMenuView::~RootMenuView() = default;

LRESULT RootMenuView::OwnerWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ENTERMENULOOP:
		OnEnterMenuLoop(wParam);
		break;

	case WM_EXITMENULOOP:
		OnExitMenuLoop(wParam);
		break;

	case WM_INITMENUPOPUP:
		OnInitMenuPopup(hwnd, reinterpret_cast<HMENU>(wParam));
		break;

	case WM_UNINITMENUPOPUP:
		OnUninitMenuPopup(reinterpret_cast<HMENU>(wParam));
		break;

	case WM_MENUSELECT:
		OnMenuItemSelected(reinterpret_cast<HMENU>(lParam), LOWORD(wParam), HIWORD(wParam));
		break;

	case WM_MENUCOMMAND:
		if (OnMenuCommand(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam)))
		{
			return 0;
		}
		break;

	case WM_MBUTTONUP:
		if (OnMiddleButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
				WI_IsFlagSet(wParam, MK_CONTROL), WI_IsFlagSet(wParam, MK_SHIFT)))
		{
			return 0;
		}
		break;

	case WM_MENURBUTTONUP:
		if (OnMenuRightButtonUp(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam)))
		{
			return 0;
		}
		break;

	case WM_MENUDRAG:
		if (auto result = OnMenuDrag(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam)))
		{
			return *result;
		}
		break;

	case WM_MENUGETOBJECT:
		if (auto result = OnMenuGetObject(reinterpret_cast<MENUGETOBJECTINFO *>(lParam)))
		{
			return *result;
		}
		break;
	}

	if (auto result = MaybeForwardMessageToExternalItemDelegate(msg, wParam, lParam))
	{
		return *result;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void RootMenuView::OnEnterMenuLoop(bool shortcutMenu)
{
	if (m_menuType == MenuType::WindowMenu && !shortcutMenu)
	{
		OnMenuOpened();
	}
}

void RootMenuView::OnExitMenuLoop(bool shortcutMenu)
{
	if (m_menuType == MenuType::WindowMenu && !shortcutMenu)
	{
		OnMenuClosed();
	}
}

void RootMenuView::OnInitMenuPopup(HWND hwnd, HMENU menu)
{
	if (m_menuType == MenuType::ContextMenu && menu == m_menu)
	{
		OnMenuOpened();
	}

	OnNativePopupWillShowForDpi(menu, DpiCompatibility::GetInstance().GetDpiForWindow(hwnd));
}

void RootMenuView::OnUninitMenuPopup(HMENU menu)
{
	OnNativePopupClosed(menu);

	if (m_menuType == MenuType::ContextMenu && menu == m_menu)
	{
		OnMenuClosed();
	}
}

void RootMenuView::OnMenuOpened()
{
	m_helpTextConnection = m_menuHelpTextHost->AddMenuHelpTextRequestObserver(
		std::bind_front(&RootMenuView::OnHelpTextRequested, this));

	m_menuHelpTextHost->NotifyTopLevelMenuShown();
}

void RootMenuView::OnMenuClosed()
{
	m_menuHelpTextHost->NotifyTopLevelMenuClosed();

	m_dropTarget.reset();
	m_helpTextConnection.disconnect();
}

void RootMenuView::OnMenuItemSelected(HMENU menu, WORD indexOrId, UINT flags)
{
	if (!menu || !IsPartOfMenu(menu))
	{
		return;
	}

	m_menuHelpTextHost->MenuItemSelected(menu, indexOrId, flags);
}

bool RootMenuView::OnMenuCommand(HMENU menu, int index)
{
	auto id = MaybeGetItemIdForNativeMenu(menu, index);

	if (!id)
	{
		return false;
	}

	SelectItem(*id, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));

	return true;
}

void RootMenuView::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (IsExternalItem(id))
	{
		SelectExternalItem(id);
		return;
	}

	auto *item = GetItemInSubtree(id);
	item->Select(isCtrlKeyDown, isShiftKeyDown);
}

void RootMenuView::SelectExternalItem(UINT id)
{
	auto *externalItemDelegate = MaybeGetExternalItemDelegate();

	if (!externalItemDelegate)
	{
		return;
	}

	externalItemDelegate->OnExternalItemSelected(id);
}

bool RootMenuView::OnMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto menuItemId = MenuHelper::MaybeGetItemAtPoint(m_menu, pt);

	if (!menuItemId)
	{
		return false;
	}

	MiddleClickItem(*menuItemId, isCtrlKeyDown, isShiftKeyDown);

	return true;
}

void RootMenuView::MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (IsExternalItem(id))
	{
		return;
	}

	auto *item = GetItemInSubtree(id);
	item->MiddleClick(isCtrlKeyDown, isShiftKeyDown);
}

bool RootMenuView::OnMenuRightButtonUp(HMENU menu, int index)
{
	auto id = MaybeGetItemIdForNativeMenu(menu, index);

	if (!id)
	{
		return false;
	}

	DWORD messagePos = GetMessagePos();
	POINT ptScreen = { GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos) };
	RightClickItem(*id, ptScreen);

	return true;
}

void RootMenuView::RightClickItem(UINT id, const POINT &ptScreen)
{
	if (IsExternalItem(id))
	{
		return;
	}

	auto *item = GetItemInSubtree(id);
	item->RightClick(ptScreen);
}

std::optional<LRESULT> RootMenuView::OnMenuDrag(HMENU menu, int index)
{
	if (!IsPartOfMenu(menu))
	{
		return std::nullopt;
	}

	// It appears that dragging at the very top of the menu (above the first item) or at the very
	// bottom of the menu (below the last item) will trigger a WM_MENUDRAG message, with a position
	// of -1. It doesn't make sense to begin a drag here in that case, though, since no actual item
	// is being dragged.
	if (index == -1)
	{
		return MND_CONTINUE;
	}

	auto id = MaybeGetItemIdForNativeMenu(menu, index);

	if (!id)
	{
		return MND_CONTINUE;
	}

	auto action = DragItem(*id);
	return action == MenuDragAction::ContinueMenu ? MND_CONTINUE : MND_ENDMENU;
}

MenuDragAction RootMenuView::DragItem(UINT id)
{
	if (IsExternalItem(id))
	{
		return MenuDragAction::ContinueMenu;
	}

	auto *item = GetItemInSubtree(id);
	return item->Drag();
}

std::optional<LRESULT> RootMenuView::OnMenuGetObject(MENUGETOBJECTINFO *objectInfo)
{
	if (!IsPartOfMenu(objectInfo->hmenu))
	{
		return std::nullopt;
	}

	auto *view = MaybeGetMenuViewForNativeMenu(objectInfo->hmenu);

	if (!view)
	{
		return MNGO_NOINTERFACE;
	}

	auto requestedIid = static_cast<const IID *>(objectInfo->riid);

	if (!IsEqualIID(*requestedIid, IID_IDropTarget))
	{
		return MNGO_NOINTERFACE;
	}

	UINT dropIndex;
	MenuDropPosition dropPosition;

	// If neither MNGOF_TOPGAP or MNGOF_BOTTOMGAP is set, the cursor is over an item.
	if (WI_AreAllFlagsClear(objectInfo->dwFlags, MNGOF_TOPGAP | MNGOF_BOTTOMGAP))
	{
		dropIndex = objectInfo->uPos;
		dropPosition = MenuDropPosition::On;
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

			dropIndex = objectInfo->uPos - 1;
			dropPosition = MenuDropPosition::After;
		}
		else
		{
			dropIndex = objectInfo->uPos;
			dropPosition = MenuDropPosition::Before;
		}
	}

	auto dropId = MaybeGetItemIdForNativeMenu(objectInfo->hmenu, dropIndex);

	if (!dropId)
	{
		return MNGO_NOINTERFACE;
	}

	// This represents the drop target for the specified item. It needs to be held either until
	// another target is requested, or the menu is closed.
	m_dropTarget = MaybeGetDropTargetForItem(*dropId, dropPosition);

	if (!m_dropTarget)
	{
		return MNGO_NOINTERFACE;
	}

	objectInfo->pvObj = m_dropTarget.get();

	return MNGO_NOERROR;
}

wil::com_ptr_nothrow<IDropTarget> RootMenuView::MaybeGetDropTargetForItem(UINT id,
	MenuDropPosition position)
{
	if (IsExternalItem(id))
	{
		return nullptr;
	}

	auto *item = GetItemInSubtree(id);
	return item->MaybeGetDropTarget(position);
}

std::optional<UINT> RootMenuView::MaybeGetItemIdForNativeMenu(HMENU menu, int index) const
{
	if (!IsPartOfMenu(menu))
	{
		return std::nullopt;
	}

	return MenuHelper::MaybeGetItemIdIncludingSubmenu(menu, index);
}

std::optional<std::wstring> RootMenuView::OnHelpTextRequested(HMENU menu, UINT id)
{
	if (!IsPartOfMenu(menu))
	{
		return std::nullopt;
	}

	return GetItemHelpText(id);
}

std::wstring RootMenuView::GetItemHelpText(UINT id) const
{
	if (IsExternalItem(id))
	{
		return GetExternalItemHelpText(id);
	}

	const auto *item = GetItemInSubtree(id);
	return item->GetHelpText();
}

std::wstring RootMenuView::GetExternalItemHelpText(UINT id) const
{
	auto *externalItemDelegate = MaybeGetExternalItemDelegate();

	if (!externalItemDelegate)
	{
		return L"";
	}

	auto helpText = externalItemDelegate->MaybeGetExternalItemHelpText(id);

	if (!helpText)
	{
		return L"";
	}

	return *helpText;
}

std::optional<LRESULT> RootMenuView::MaybeForwardMessageToExternalItemDelegate(UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	auto *externalItemDelegate = MaybeGetExternalItemDelegate();

	if (!externalItemDelegate)
	{
		return std::nullopt;
	}

	return externalItemDelegate->MaybeHandleMessage(msg, wParam, lParam);
}

bool RootMenuView::IsPartOfMenu(HMENU potentiallyRelatedMenu) const
{
	return MenuHelper::IsPartOfMenu(m_menu, potentiallyRelatedMenu);
}

bool RootMenuView::IsExternalItem(UINT id)
{
	return id >= EXTERNAL_ID_MIN && id <= EXTERNAL_ID_MAX;
}
