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

ShellItemsMenu::ShellItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	const std::vector<PidlAbsolute> &pidls, BrowserWindow *browserWindow, IconFetcher *iconFetcher,
	UINT menuStartId, UINT menuEndId) :
	MenuBase(menuView, acceleratorManager),
	m_browserWindow(browserWindow),
	m_iconFetcher(iconFetcher),
	m_menuStartId(menuStartId),
	m_menuEndId(menuEndId),
	m_idCounter(menuStartId),
	m_destroyed(std::make_shared<bool>(false))
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind_front(&ShellItemsMenu::OnMenuItemSelected, this)));
	m_connections.push_back(m_menuView->AddItemMiddleClickedObserver(
		std::bind_front(&ShellItemsMenu::OnMenuItemMiddleClicked, this)));

	RebuildMenu(pidls);
}

ShellItemsMenu::~ShellItemsMenu()
{
	*m_destroyed = true;
}

void ShellItemsMenu::RebuildMenu(const std::vector<PidlAbsolute> &pidls)
{
	m_menuView->ClearMenu();
	m_idCounter = m_menuStartId;
	m_idPidlMap.clear();
	m_pendingIconCallbackIds.clear();

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

	if (id >= m_menuEndId)
	{
		return;
	}

	auto bitmap = GetShellItemIcon(pidl);
	QueueIconUpdateTask(pidl, id);

	std::wstring displayPath;

	if (auto optionalDisplayPath = GetFolderPathForDisplay(pidl))
	{
		displayPath = *optionalDisplayPath;
	}

	m_menuView->AppendItem(id, name, std::move(bitmap), displayPath);

	auto [itr, didInsert] = m_idPidlMap.insert({ id, pidl });
	DCHECK(didInsert);
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

void ShellItemsMenu::QueueIconUpdateTask(PCIDLIST_ABSOLUTE pidl, UINT menuItemId)
{
	int iconCallbackId = m_iconCallbackIdCounter++;

	// The item may not have a cached icon, or the cached icon may be out of date. Therefore, this
	// call will retrieve the updated icon. If this finishes before the menu is closed, the menu
	// item will be updated. It's still useful even if the call finishes after the menu is closed,
	// since the updated icon will be cached.
	m_iconFetcher->QueueIconTask(pidl,
		[this, destroyed = m_destroyed, menuItemId, iconCallbackId](int iconIndex)
		{
			if (*destroyed)
			{
				// The icon can be returned after the menu has been closed. In that case, there's
				// nothing that needs to be done.
				return;
			}

			OnIconRetrieved(menuItemId, iconIndex, iconCallbackId);
		});

	auto [itr, didInsert] = m_pendingIconCallbackIds.insert(iconCallbackId);
	DCHECK(didInsert);
}

void ShellItemsMenu::OnIconRetrieved(UINT menuItemId, int iconIndex, int callbackId)
{
	auto itr = m_pendingIconCallbackIds.find(callbackId);

	// As the menu can be cleared when the set of items changes, this icon notification might be for
	// a previous menu item. If it is, it can be ignored.
	if (itr == m_pendingIconCallbackIds.end())
	{
		return;
	}

	m_pendingIconCallbackIds.erase(itr);

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
