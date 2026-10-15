// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuView.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <memory>
#include <optional>
#include <string>

class MenuHelpTextHost;
class WindowSubclass;

// Represents a top-level menu, as opposed to MenuView, which represents one level in the menu tree.
class RootMenuView : public MenuView
{
public:
	~RootMenuView();

protected:
	enum class MenuType
	{
		// Indicates the menu is assigned to a window via SetMenu().
		WindowMenu,

		// Indicates the menu is a context menu.
		ContextMenu
	};

	RootMenuView(wil::unique_hmenu menu, HWND ownerWindow, MenuType menuType,
		MenuHelpTextHost *menuHelpTextHost);

	void SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);

private:
	friend class RootMenuViewTestApi;

	LRESULT OwnerWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnEnterMenuLoop(bool shortcutMenu);
	void OnExitMenuLoop(bool shortcutMenu);
	void OnInitMenuPopup(HWND hwnd, HMENU menu);
	void OnUninitMenuPopup(HMENU menu);
	void OnMenuOpened();
	void OnMenuClosed();
	void OnMenuItemSelected(HMENU menu, WORD indexOrId, UINT flags);
	bool OnMenuCommand(HMENU menu, int index);
	void SelectExternalItem(UINT id);
	bool OnMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);
	void MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);
	bool OnMenuRightButtonUp(HMENU menu, int index);
	void RightClickItem(UINT id, const POINT &ptScreen);
	std::optional<LRESULT> OnMenuDrag(HMENU menu, int index);
	MenuDragAction DragItem(UINT id);
	std::optional<LRESULT> OnMenuGetObject(MENUGETOBJECTINFO *objectInfo);
	wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTargetForItem(UINT id, MenuDropPosition position);
	std::optional<UINT> MaybeGetItemIdForNativeMenu(HMENU menu, int index) const;
	std::optional<std::wstring> OnHelpTextRequested(HMENU menu, UINT id);
	std::wstring GetItemHelpText(UINT id) const;
	std::wstring GetExternalItemHelpText(UINT id) const;
	std::optional<LRESULT> MaybeForwardMessageToExternalItemDelegate(UINT msg, WPARAM wParam,
		LPARAM lParam);
	bool IsPartOfMenu(HMENU potentiallyRelatedMenu) const;
	static bool IsExternalItem(UINT id);

	const MenuType m_menuType;
	MenuHelpTextHost *const m_menuHelpTextHost;
	std::unique_ptr<WindowSubclass> m_subclass;
	bool m_isMenuOpen = false;
	boost::signals2::scoped_connection m_helpTextConnection;
	wil::com_ptr_nothrow<IDropTarget> m_dropTarget;
};
