// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemsMenu.h"
#include "IconFetcher.h"
#include "Navigator.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"

ShellItemsMenu::ShellItemsMenu(const std::vector<PidlAbsolute> &pidls, Navigator *navigator,
	IconFetcher *iconFetcher) :
	m_navigator(navigator),
	m_iconFetcher(iconFetcher)
{
	[[maybe_unused]] HRESULT hr = SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList));
	assert(SUCCEEDED(hr));

	m_menuView = BuildMenu(pidls);
}

void ShellItemsMenu::Show(HWND hwnd, const POINT &point)
{
	m_menuView->Show(hwnd, point);
}

std::shared_ptr<PopupMenuView> ShellItemsMenu::BuildMenu(const std::vector<PidlAbsolute> &pidls)
{
	auto menu = std::make_shared<PopupMenuView>(this);

	for (const auto &pidl : pidls)
	{
		AddMenuItemForPidl(menu, pidl.Raw());
	}

	return menu;
}

void ShellItemsMenu::AddMenuItemForPidl(std::shared_ptr<PopupMenuView> menuView,
	PCIDLIST_ABSOLUTE pidl)
{
	std::wstring name;
	HRESULT hr = GetDisplayName(pidl, SHGDN_NORMAL, name);

	if (FAILED(hr))
	{
		assert(false);

		name = L"(Unknown)";
	}

	int id = m_idCounter++;

	auto bitmap = GetShellItemIcon(pidl);

	// The item may not have a cached icon, or the cached icon may be out of date. Therefore,
	// the updated icon will always be retrieved here. If this finishes before the menu is
	// closed, the menu item will be updated. It's still useful even if the menu is closed
	// first, since the updated icon will be cached.
	m_iconFetcher->QueueIconTask(pidl,
		[weakMenuView = std::weak_ptr<PopupMenuView>(menuView), id,
			systemImageList = m_systemImageList](int iconIndex)
		{ OnIconRetrieved(weakMenuView, id, systemImageList.get(), iconIndex); });

	menuView->AppendItem(id, name, std::move(bitmap));

	m_idPidlMap.insert({ id, pidl });
}

wil::unique_hbitmap ShellItemsMenu::GetShellItemIcon(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring itemPath;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, itemPath);

	if (FAILED(hr))
	{
		return nullptr;
	}

	SFGAOF attributes = SFGAO_FOLDER;
	hr = GetItemAttributes(pidl, &attributes);

	if (FAILED(hr))
	{
		return nullptr;
	}

	DefaultIconType defaultIconType;

	if (WI_IsFlagSet(attributes, SFGAO_FOLDER))
	{
		defaultIconType = DefaultIconType::Folder;
	}
	else
	{
		defaultIconType = DefaultIconType::File;
	}

	int iconIndex = m_iconFetcher->GetCachedIconIndexOrDefault(itemPath, defaultIconType);

	return ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), iconIndex);
}

void ShellItemsMenu::OnIconRetrieved(std::weak_ptr<PopupMenuView> weakMenuView, int menuItemId,
	IImageList *systemImageList, int iconIndex)
{
	auto menuView = weakMenuView.lock();

	if (!menuView)
	{
		// The icon can be returned after the menu has been closed. In that case, there's
		// nothing that needs to be done.
		return;
	}

	auto bitmap = ImageHelper::ImageListIconToBitmap(systemImageList, iconIndex);

	if (!bitmap)
	{
		assert(false);
		return;
	}

	menuView->SetBitmapForItem(menuItemId, std::move(bitmap));
}

void ShellItemsMenu::OnMenuItemSelected(int menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	OpenSelectedItem(menuItemId, false, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OnMenuItemMiddleClicked(int menuItemId, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	OpenSelectedItem(menuItemId, true, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OpenSelectedItem(int menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	auto &pidl = m_idPidlMap.at(menuItemId);
	m_navigator->OpenItem(pidl.Raw(),
		m_navigator->DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown));
}
