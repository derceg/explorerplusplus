// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemContextMenu.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/ShellHelper.h"
#include <algorithm>

ShellItemContextMenu::ShellItemContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, const PidlAbsolute &directory,
	const std::vector<PidlChild> &items, HWND hwnd, IUnknown *site, Flags flags,
	BrowserWindow *browser, const ResourceLoader *resourceLoader,
	MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback) :
	ShellItemContextMenu(menuView, acceleratorManager, directory, items, hwnd, site, flags, browser,
		nullptr, resourceLoader, std::move(maybeHandleShellMenuItemCallback))
{
}

ShellItemContextMenu::ShellItemContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, const PidlAbsolute &directory,
	const std::vector<PidlChild> &items, HWND hwnd, IUnknown *site, Flags flags,
	BrowserList *browserList, const ResourceLoader *resourceLoader,
	MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback) :
	ShellItemContextMenu(menuView, acceleratorManager, directory, items, hwnd, site, flags, nullptr,
		browserList, resourceLoader, std::move(maybeHandleShellMenuItemCallback))
{
}

ShellItemContextMenu::ShellItemContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, const PidlAbsolute &directory,
	const std::vector<PidlChild> &items, HWND hwnd, IUnknown *site, Flags flags,
	BrowserWindow *browser, BrowserList *browserList, const ResourceLoader *resourceLoader,
	MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback) :
	ShellContextMenu(menuView, acceleratorManager, directory, hwnd),
	m_items(items),
	m_browser(browser),
	m_browserList(browserList),
	m_maybeHandleShellMenuItemCallback(std::move(maybeHandleShellMenuItemCallback))
{
	CHECK(!m_items.empty());

	BuildMenu(site, flags, resourceLoader);
}

void ShellItemContextMenu::BuildMenu(IUnknown *site, Flags flags,
	const ResourceLoader *resourceLoader)
{
	UINT contextMenuFlags = CMF_NORMAL;

	if (WI_IsFlagSet(flags, Flags::ExtendedVerbs))
	{
		contextMenuFlags |= CMF_EXTENDEDVERBS;
	}

	if (WI_IsFlagSet(flags, Flags::Rename))
	{
		contextMenuFlags |= CMF_CANRENAME;
	}

	AppendShellItems(site, contextMenuFlags);

	MaybeAddOpenInNewTabItem(resourceLoader);

	RemoveRedundantSeparators();
}

void ShellItemContextMenu::MaybeAddOpenInNewTabItem(const ResourceLoader *resourceLoader)
{
	if (m_items.size() != 1)
	{
		return;
	}

	auto pidlComplete = m_directory + m_items[0];

	if (!DoesItemHaveAttributes(pidlComplete.Raw(), SFGAO_FOLDER))
	{
		return;
	}

	auto openItemIndex = MaybeGetShellItemIndex(L"open");

	m_rootMenuView->AddItem(
		MenuItemParams(resourceLoader->LoadString(IDS_GENERAL_OPEN_IN_NEW_TAB))
			.SetHelpText(resourceLoader->LoadString(IDS_GENERAL_OPEN_IN_NEW_TAB_HELP_TEXT))
			.SetSelectedCallback([this, pidlComplete] { OnOpenDirectoryInNewTab(pidlComplete); }),
		openItemIndex ? *openItemIndex + 1 : 0);
}

wil::com_ptr_nothrow<IContextMenu> ShellItemContextMenu::MaybeGetShellContextMenu(HWND hwnd) const
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, m_directory.Raw(), nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::vector<PCITEMID_CHILD> pidlItemsRaw;
	std::ranges::transform(m_items, std::back_inserter(pidlItemsRaw),
		[](const auto &pidl) { return pidl.Raw(); });

	wil::com_ptr_nothrow<IContextMenu> contextMenu;
	hr = GetUIObjectOf(shellFolder.get(), hwnd, static_cast<UINT>(pidlItemsRaw.size()),
		pidlItemsRaw.data(), IID_PPV_ARGS(&contextMenu));

	if (FAILED(hr))
	{
		return nullptr;
	}

	return contextMenu;
}

bool ShellItemContextMenu::MaybeHandleShellMenuItem(const std::wstring &verb)
{
	if (verb == L"open")
	{
		auto *browser = GetTargetBrowser();

		for (const auto &item : m_items)
		{
			auto pidlComplete = m_directory + item;
			browser->OpenItem(pidlComplete.Raw());
		}

		return true;
	}

	if (m_maybeHandleShellMenuItemCallback)
	{
		return m_maybeHandleShellMenuItemCallback(m_directory, m_items, verb);
	}

	return false;
}

void ShellItemContextMenu::OnOpenDirectoryInNewTab(const PidlAbsolute &directory)
{
	auto *browser = GetTargetBrowser();
	browser->OpenItem(directory.Raw(), OpenFolderDisposition::NewTabDefault);
}

BrowserWindow *ShellItemContextMenu::GetTargetBrowser() const
{
	if (m_browser)
	{
		return m_browser;
	}

	auto *lastActiveBrowser = m_browserList->GetLastActive();
	CHECK(lastActiveBrowser);

	return lastActiveBrowser;
}
