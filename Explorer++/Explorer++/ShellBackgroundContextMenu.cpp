// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBackgroundContextMenu.h"
#include "BrowserCommandController.h"
#include "BrowserWindow.h"
#include "DirectoryOperationsHelper.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowser.h"
#include "SortGroupMenu.h"
#include "ViewsMenu.h"
#include <wil/common.h>

ShellBackgroundContextMenu::ShellBackgroundContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, const PidlAbsolute &directory, HWND hwnd,
	IUnknown *site, Flags flags, BrowserWindow *browser, ClipboardStore *clipboardStore,
	const ResourceLoader *resourceLoader) :
	ShellContextMenu(menuView, acceleratorManager, directory, hwnd),
	m_browser(browser),
	m_clipboardStore(clipboardStore)
{
	BuildMenu(site, flags, resourceLoader);
}

void ShellBackgroundContextMenu::BuildMenu(IUnknown *site, Flags flags,
	const ResourceLoader *resourceLoader)
{
	auto *viewsSubMenuView =
		m_rootMenuView->AppendSubMenu(resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_VIEW));
	m_childMenus.push_back(std::make_unique<ViewsMenu>(viewsSubMenuView, m_acceleratorManager,
		m_browser, resourceLoader));

	auto *sortByMenuView = m_rootMenuView->AppendSubMenu(
		resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_SORT_BY));
	m_childMenus.push_back(std::make_unique<SortGroupMenu>(sortByMenuView, m_acceleratorManager,
		m_browser, SortGroupMenu::Mode::Sort, resourceLoader));

	auto *groupByMenuView = m_rootMenuView->AppendSubMenu(
		SubMenuItemParams(resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_GROUP_BY))
			.SetEnabledCallback(
				[this] { return m_browser->GetActiveShellBrowser()->CanEnableGroupMode(); }));
	m_childMenus.push_back(std::make_unique<SortGroupMenu>(groupByMenuView, m_acceleratorManager,
		m_browser, SortGroupMenu::Mode::Group, resourceLoader));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_REFRESH))
			.SetHelpText(resourceLoader->LoadString(IDS_VIEW_REFRESH_HELP_TEXT))
			.SetSelectedCallback(
				[this] { m_browser->GetCommandController()->ExecuteCommand(IDM_VIEW_REFRESH); }));

	m_rootMenuView->AppendSeparator();

	if (CanCustomizeDirectory(m_directory.Raw()))
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_CUSTOMIZE))
				.SetHelpText(
					resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_CUSTOMIZE_HELP_TEXT))
				.SetSelectedCallback([this] { OnCustomizeDirectory(); }));

		m_rootMenuView->AppendSeparator();
	}

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_PASTE))
			.SetHelpText(resourceLoader->LoadString(IDS_EDIT_PASTE_HELP_TEXT))
			.SetEnabledCallback([this] { return CanPaste(PasteType::Normal); })
			.SetSelectedCallback([this] { SendCommandToBrowser(IDM_EDIT_PASTE); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT))
			.SetHelpText(resourceLoader->LoadString(IDS_EDIT_PASTESHORTCUT_HELP_TEXT))
			.SetEnabledCallback([this] { return CanPaste(PasteType::Shortcut); })
			.SetSelectedCallback([this] { SendCommandToBrowser(IDM_EDIT_PASTESHORTCUT); }));

	m_rootMenuView->AppendSeparator();

	UINT contextMenuFlags = CMF_NODEFAULT;

	if (WI_IsFlagSet(flags, Flags::ExtendedVerbs))
	{
		contextMenuFlags |= CMF_EXTENDEDVERBS;
	}

	AppendShellItems(site, contextMenuFlags);

	// This menu item appears on the background context menu for a search results folder. When it's
	// clicked, the shell will request view information, using IFolderView2, along with at least one
	// undocumented interface. Because attempting to implement an undocumented interface is
	// potentially difficult and carries risk, along with the fact that the view information in
	// Explorer++ doesn't correspond precisely with the view information in Explorer anyway, this
	// item is removed here.
	RemoveShellItems({ L"savesearch" });

	RemoveRedundantSeparators();
}

bool ShellBackgroundContextMenu::CanPaste(PasteType pasteType) const
{
	return CanPasteInDirectory(m_clipboardStore, m_directory.Raw(), pasteType);
}

wil::com_ptr_nothrow<IContextMenu> ShellBackgroundContextMenu::MaybeGetShellContextMenu(
	HWND hwnd) const
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, m_directory.Raw(), nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IContextMenu> contextMenu;
	hr = shellFolder->CreateViewObject(hwnd, IID_PPV_ARGS(&contextMenu));

	if (FAILED(hr))
	{
		return nullptr;
	}

	return contextMenu;
}

void ShellBackgroundContextMenu::OnCustomizeDirectory()
{
	// Note that the call below won't always result in the customize tab being selected. That's
	// because the properties dialog will select the tab based on its display name, which can change
	// as the display language is changed in Windows.
	//
	// In Explorer, the title of the dialog is dynamically retrieved. Although it might be possible
	// to do that here as well, that strategy would break if the customize dialog resource ID ever
	// changed.
	//
	// Another alternative might be to load the "customize" string from the string table. But the
	// language used by the application has nothing to do with the language used by Windows itself.
	// Also, the text would have to be exactly the same as that used by Windows for a given
	// language, which probably wouldn't be clear to translators. Minor variations within a language
	// (e.g. customize vs customise) could cause the tab to not be selected.
	//
	// Therefore, this will only work when the actual title of the properties dialog is "customize"
	// (ignoring case). That's not ideal, but not too much of an issue, since the properties dialog
	// will always be opened, just not always on the customize tab.
	ExecuteFileAction(m_browser->GetHWND(), m_directory.Raw(), L"properties", L"customize", L"");
}

void ShellBackgroundContextMenu::SendCommandToBrowser(WORD id)
{
	SendMessage(m_browser->GetHWND(), WM_COMMAND, MAKEWPARAM(id, 0), 0);
}
