// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemsMenu.h"
#include "BrowserWindow.h"
#include "IconFetcher.h"
#include "MenuView.h"
#include "NavigationHelper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"
#include <glog/logging.h>

ShellItemsMenu::ShellItemsMenu(MenuView *menuView, const std::vector<PidlAbsolute> &pidls,
	BrowserWindow *browserWindow, IconFetcher *iconFetcher) :
	MenuBase(menuView),
	m_browserWindow(browserWindow),
	m_iconFetcher(iconFetcher),
	m_destroyed(std::make_shared<bool>(false))
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));

	BuildMenu(pidls);

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind_front(&ShellItemsMenu::OnMenuItemSelected, this)));
	m_connections.push_back(m_menuView->AddItemMiddleClickedObserver(
		std::bind_front(&ShellItemsMenu::OnMenuItemMiddleClicked, this)));
}

ShellItemsMenu::~ShellItemsMenu()
{
	*m_destroyed = true;
}

void ShellItemsMenu::BuildMenu(const std::vector<PidlAbsolute> &pidls)
{
	for (const auto &pidl : pidls)
	{
		AddMenuItemForPidl(pidl.Raw());
	}
}

void ShellItemsMenu::AddMenuItemForPidl(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring name;
	HRESULT hr = GetDisplayName(pidl, SHGDN_NORMAL, name);

	if (FAILED(hr))
	{
		DCHECK(false);

		name = L"(Unknown)";
	}

	auto id = m_idCounter++;

	auto bitmap = GetShellItemIcon(pidl);

	// The item may not have a cached icon, or the cached icon may be out of date. Therefore,
	// the updated icon will always be retrieved here. If this finishes before the menu is
	// closed, the menu item will be updated. It's still useful even if the menu is closed
	// first, since the updated icon will be cached.
	m_iconFetcher->QueueIconTask(pidl,
		[this, destroyed = m_destroyed, id](int iconIndex)
		{
			if (*destroyed)
			{
				// The icon can be returned after the menu has been closed. In that case, there's
				// nothing that needs to be done.
				return;
			}

			OnIconRetrieved(id, iconIndex);
		});

	m_menuView->AppendItem(id, name, std::move(bitmap));

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

void ShellItemsMenu::OnIconRetrieved(UINT menuItemId, int iconIndex)
{
	auto bitmap = ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), iconIndex);

	if (!bitmap)
	{
		DCHECK(false);
		return;
	}

	m_menuView->SetBitmapForItem(menuItemId, std::move(bitmap));
}

void ShellItemsMenu::OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	OpenSelectedItem(menuItemId, false, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	OpenSelectedItem(menuItemId, true, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OpenSelectedItem(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	auto &pidl = m_idPidlMap.at(menuItemId);
	m_browserWindow->OpenItem(pidl.Raw(),
		DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown));
}
