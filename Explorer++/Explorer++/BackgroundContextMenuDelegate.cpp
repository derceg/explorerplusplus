// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BackgroundContextMenuDelegate.h"
#include "BrowserWindow.h"
#include "DirectoryOperationsHelper.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowser.h"
#include "SortMenuBuilder.h"
#include "ViewsMenuBuilder.h"
#include "../Helper/ShellContextMenuBuilder.h"

BackgroundContextMenuDelegate::BackgroundContextMenuDelegate(const BrowserWindow *browser,
	const ResourceLoader *resourceLoader) :
	m_browser(browser),
	m_resourceLoader(resourceLoader)
{
}

void BackgroundContextMenuDelegate::UpdateMenuEntries(PCIDLIST_ABSOLUTE directory,
	ShellContextMenuBuilder *builder)
{
	RemoveNonFunctionalItems(builder);

	UINT position = 0;

	ViewsMenuBuilder viewsMenuBuilder(m_resourceLoader);
	auto viewsMenu = viewsMenuBuilder.BuildMenu(m_browser);
	std::wstring text = m_resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_VIEW);
	builder->AddSubMenuItem(text, std::move(viewsMenu), position++, true);

	SortMenuBuilder sortMenuBuilder(m_resourceLoader);
	auto sortMenus = sortMenuBuilder.BuildMenus(*m_browser->GetActiveShellBrowser()->GetTab());
	text = m_resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_SORT_BY);
	builder->AddSubMenuItem(text, std::move(sortMenus.sortByMenu), position++, true);

	text = m_resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_GROUP_BY);
	builder->AddSubMenuItem(text, std::move(sortMenus.groupByMenu), position++, true);

	text = m_resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_REFRESH);
	builder->AddStringItem(IDM_BACKGROUND_CONTEXT_MENU_REFRESH, text, position++, true);

	builder->AddSeparator(position++, true);

	if (CanCustomizeDirectory(directory))
	{
		text = m_resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_CUSTOMIZE);
		builder->AddStringItem(IDM_BACKGROUND_CONTEXT_MENU_CUSTOMIZE, text, position++, true);
		builder->AddSeparator(position++, true);
	}

	text = m_resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_PASTE);
	builder->AddStringItem(IDM_BACKGROUND_CONTEXT_MENU_PASTE, text, position++, true);

	if (!CanPasteInDirectory(directory, PasteType::Normal))
	{
		builder->EnableItem(IDM_BACKGROUND_CONTEXT_MENU_PASTE, false);
	}

	text = m_resourceLoader->LoadString(IDS_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT);
	builder->AddStringItem(IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT, text, position++, true);

	if (!CanPasteInDirectory(directory, PasteType::Shortcut))
	{
		builder->EnableItem(IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT, false);
	}

	builder->AddSeparator(position++, true);
}

void BackgroundContextMenuDelegate::RemoveNonFunctionalItems(ShellContextMenuBuilder *builder)
{
	// This menu item appears on the background context menu for a search results folder. When it's
	// clicked, the shell will request view information, using IFolderView2, along with at least one
	// undocumented interface. Because attempting to implement an undocumented interface is
	// potentially difficult and carries risk, along with the fact that the view information in
	// Explorer++ doesn't correspond precisely with the view information in Explorer anyway, this
	// item is removed here.
	builder->RemoveShellItem(L"savesearch");
}

bool BackgroundContextMenuDelegate::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::wstring &verb)
{
	UNREFERENCED_PARAMETER(directory);
	UNREFERENCED_PARAMETER(verb);

	return false;
}

void BackgroundContextMenuDelegate::HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory,
	UINT menuItemId)
{
	UNREFERENCED_PARAMETER(directory);

	// Custom items in the context menu will be handled by the WM_COMMAND handler.
	SendMessage(m_browser->GetHWND(), WM_COMMAND, MAKEWPARAM(menuItemId, 0), 0);
}

std::wstring BackgroundContextMenuDelegate::GetHelpTextForCustomItem(UINT menuItemId)
{
	// By default, the help text will be looked up via the menu item ID.
	UINT menuHelpTextId = menuItemId;

	switch (menuItemId)
	{
	case IDM_BACKGROUND_CONTEXT_MENU_REFRESH:
		menuHelpTextId = IDM_VIEW_REFRESH;
		break;

	case IDM_BACKGROUND_CONTEXT_MENU_PASTE:
		menuHelpTextId = IDM_EDIT_PASTE;
		break;

	case IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT:
		menuHelpTextId = IDM_EDIT_PASTESHORTCUT;
		break;
	}

	auto helpText = m_resourceLoader->MaybeLoadString(menuHelpTextId);

	if (helpText)
	{
		return *helpText;
	}

	return L"";
}
