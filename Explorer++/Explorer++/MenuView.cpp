// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuView.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/MenuHelpTextHost.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WeakPtr.h"

MenuView::MenuView(MenuHelpTextHost *menuHelpTextHost) : m_menuHelpTextHost(menuHelpTextHost)
{
}

MenuView::~MenuView()
{
	m_viewDestroyedSignal();
}

void MenuView::AppendItem(UINT id, const std::wstring &text,
	std::unique_ptr<const IconModel> iconModel, const std::wstring &helpText,
	const std::optional<std::wstring> &acceleratorText)
{
	// The value 0 shouldn't be used as an item ID. That's because a call like TrackPopupMenu() will
	// use a return value of 0 to indicate the menu was canceled, or an error occurred.
	DCHECK_NE(id, 0U);

	std::wstring finalText = text;

	if (acceleratorText)
	{
		finalText += L"\t" + *acceleratorText;
	}

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID | MIIM_STRING;
	menuItemInfo.wID = id;
	menuItemInfo.dwTypeData = finalText.data();

	auto res = InsertMenuItem(GetMenu(), GetMenuItemCount(GetMenu()), true, &menuItemInfo);
	CHECK(res);

	auto [itr, didInsert] = m_idToItemMap.try_emplace(id, std::move(iconModel), helpText);
	CHECK(didInsert);

	// It's only possible to add images to the menu when the DPI is known (so that the appropriate
	// DPI scaling factor can be applied). If there is no current DPI set (because the menu isn't
	// being shown), nothing needs to be done. The image will be added to the menu once the menu is
	// shown.
	if (m_currentDpi)
	{
		SetItemImage(id);
	}
}

void MenuView::SetItemImage(UINT id)
{
	const auto *item = GetItem(id);

	if (!item->iconModel)
	{
		return;
	}

	auto bitmap = item->iconModel->GetBitmap(GetCurrentDpi(),
		[id, self = m_weakPtrFactory.GetWeakPtr()](wil::unique_hbitmap updatedBitmap)
		{
			if (!self)
			{
				// The updated image can be returned after the menu has been closed or cleared. In
				// either case, there's nothing that needs to be done.
				return;
			}

			self->UpdateItemBitmap(id, std::move(updatedBitmap));
		});

	UpdateItemBitmap(id, std::move(bitmap));
}

void MenuView::UpdateItemBitmap(UINT id, wil::unique_hbitmap bitmap)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	menuItemInfo.hbmpItem = bitmap.get();
	auto res = SetMenuItemInfo(GetMenu(), id, false, &menuItemInfo);
	CHECK(res);

	if (bitmap)
	{
		auto *item = GetItem(id);
		item->bitmap = std::move(bitmap);
	}
}

void MenuView::AppendSeparator()
{
	MenuHelper::AddSeparator(GetMenu());
}

void MenuView::EnableItem(UINT id, bool enable)
{
	MenuHelper::EnableItem(GetMenu(), id, enable);
}

void MenuView::CheckItem(UINT id, bool check)
{
	MenuHelper::CheckItem(GetMenu(), id, check);
}

void MenuView::RemoveTrailingSeparators()
{
	MenuHelper::RemoveTrailingSeparators(GetMenu());
}

void MenuView::ClearMenu()
{
	for (int i = GetMenuItemCount(GetMenu()) - 1; i >= 0; i--)
	{
		auto res = DeleteMenu(GetMenu(), i, MF_BYPOSITION);
		DCHECK(res);
	}

	m_idToItemMap.clear();
	m_lastRenderedImageDpi.reset();
	m_weakPtrFactory.InvalidateWeakPtrs();
}

void MenuView::OnMenuWillShow(HWND ownerWindow)
{
	OnMenuWillShowForDpi(DpiCompatibility::GetInstance().GetDpiForWindow(ownerWindow));
}

void MenuView::OnMenuWillShowForDpi(UINT dpi)
{
	DCHECK(!m_currentDpi);

	m_currentDpi = dpi;

	MaybeAddImagesToMenu();

	m_helpTextConnection = m_menuHelpTextHost->AddMenuHelpTextRequestObserver(
		std::bind_front(&MenuView::OnHelpTextRequested, this));
}

void MenuView::OnMenuClosed()
{
	DCHECK(m_currentDpi);

	m_currentDpi.reset();
	m_helpTextConnection.disconnect();
}

std::optional<std::wstring> MenuView::OnHelpTextRequested(HMENU menu, int id)
{
	if (!MenuHelper::IsPartOfMenu(GetMenu(), menu))
	{
		return std::nullopt;
	}

	return GetItemHelpText(id);
}

void MenuView::MaybeAddImagesToMenu()
{
	if (GetCurrentDpi() == m_lastRenderedImageDpi)
	{
		// The DPI hasn't changed since the images were last added, so there's nothing that needs to
		// be done.
		return;
	}

	for (UINT id : m_idToItemMap | std::views::keys)
	{
		SetItemImage(id);
	}

	m_lastRenderedImageDpi = GetCurrentDpi();
}

UINT MenuView::GetCurrentDpi()
{
	CHECK(m_currentDpi);
	return *m_currentDpi;
}

std::wstring MenuView::GetItemHelpText(UINT id) const
{
	const auto *item = GetItem(id);
	return item->helpText;
}

MenuView::Item *MenuView::GetItem(int id)
{
	auto itr = m_idToItemMap.find(id);
	CHECK(itr != m_idToItemMap.end());
	return &itr->second;
}

const MenuView::Item *MenuView::GetItem(int id) const
{
	auto itr = m_idToItemMap.find(id);
	CHECK(itr != m_idToItemMap.end());
	return &itr->second;
}

void MenuView::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!MenuHelper::IsMenuItemEnabled(GetMenu(), id, false))
	{
		return;
	}

	m_itemSelectedSignal(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuView::MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!MenuHelper::IsMenuItemEnabled(GetMenu(), id, false))
	{
		return;
	}

	m_itemMiddleClickedSignal(id, isCtrlKeyDown, isShiftKeyDown);
}

boost::signals2::connection MenuView::AddItemSelectedObserver(
	const ItemSelectedSignal::slot_type &observer)
{
	return m_itemSelectedSignal.connect(observer);
}

boost::signals2::connection MenuView::AddItemMiddleClickedObserver(
	const ItemMiddleClickedSignal::slot_type &observer)
{
	return m_itemMiddleClickedSignal.connect(observer);
}

boost::signals2::connection MenuView::AddViewDestroyedObserver(
	const ViewDestroyedSignal::slot_type &observer)
{
	return m_viewDestroyedSignal.connect(observer);
}
