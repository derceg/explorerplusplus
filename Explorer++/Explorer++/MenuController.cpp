// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuController.h"
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
	auto *view = m_rootView->MaybeGetMenuViewForItem(id);
	CHECK(view);

	if (!view->IsItemEnabled(id))
	{
		return;
	}

	auto *delegate = view->MaybeGetDelegate();

	if (!delegate)
	{
		return;
	}

	delegate->OnItemSelected(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuController::MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto *view = m_rootView->MaybeGetMenuViewForItem(id);
	CHECK(view);

	if (!view->IsItemEnabled(id))
	{
		return;
	}

	auto *delegate = view->MaybeGetDelegate();

	if (!delegate)
	{
		return;
	}

	delegate->OnItemMiddleClicked(id, isCtrlKeyDown, isShiftKeyDown);
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
