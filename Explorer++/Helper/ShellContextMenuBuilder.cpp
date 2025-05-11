// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellContextMenuBuilder.h"
#include "MenuHelper.h"
#include "ShellContextMenu.h"
#include "ShellContextMenuIdRemapper.h"
#include <boost/algorithm/string/predicate.hpp>

ShellContextMenuBuilder::ShellContextMenuBuilder(HMENU menu, IContextMenu *contextMenu,
	ShellContextMenuIdRemapper *idRemapper) :
	m_menu(menu),
	m_contextMenu(contextMenu),
	m_idRemapper(idRemapper)
{
}

void ShellContextMenuBuilder::AddStringItem(UINT id, const std::wstring &text, UINT item,
	bool byPosition)
{
	MenuHelper::AddStringItem(m_menu, m_idRemapper->GenerateUpdatedId(id), text, item, byPosition);
}

void ShellContextMenuBuilder::EnableItem(UINT id, bool enable)
{
	MenuHelper::EnableItem(m_menu, m_idRemapper->GetUpdatedId(id), enable);
}

void ShellContextMenuBuilder::AddSeparator(UINT item, bool byPosition)
{
	MenuHelper::AddSeparator(m_menu, item, byPosition);
}

void ShellContextMenuBuilder::AddSubMenuItem(const std::wstring &text, wil::unique_hmenu subMenu,
	UINT item, bool byPosition)
{
	RemapMenuIds(subMenu.get());
	MenuHelper::AddSubMenuItem(m_menu, 0, text, std::move(subMenu), item, byPosition);
}

// When a submenu is added to a shell context menu, the IDs for each of the submenu items will need
// to be updated. This method updates the IDs for all items on a menu and stores mappings back to
// the original IDs.
void ShellContextMenuBuilder::RemapMenuIds(HMENU menu)
{
	for (int i = 0; i < GetMenuItemCount(menu); i++)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_FTYPE;
		auto res = GetMenuItemInfo(menu, i, true, &menuItemInfo);
		CHECK(res);

		if (WI_IsFlagSet(menuItemInfo.fType, MFT_SEPARATOR))
		{
			continue;
		}

		auto subMenu = GetSubMenu(menu, i);

		if (subMenu)
		{
			RemapMenuIds(subMenu);
			continue;
		}

		UINT originalId = GetMenuItemID(menu, i);

		menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_ID;
		menuItemInfo.wID = m_idRemapper->GenerateUpdatedId(originalId);
		res = SetMenuItemInfo(menu, i, true, &menuItemInfo);
		CHECK(res);
	}
}

void ShellContextMenuBuilder::RemoveShellItem(const std::wstring &targetVerb)
{
	int numItems = GetMenuItemCount(m_menu);

	if (numItems == -1)
	{
		DCHECK(false);
		return;
	}

	for (int i = numItems - 1; i >= 0; i--)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_ID | MIIM_FTYPE;
		BOOL res = GetMenuItemInfo(m_menu, i, true, &menuItemInfo);

		if (!res || WI_IsFlagSet(menuItemInfo.fType, MFT_SEPARATOR)
			|| menuItemInfo.wID < ShellContextMenu::MIN_SHELL_MENU_ID
			|| menuItemInfo.wID > ShellContextMenu::MAX_SHELL_MENU_ID)
		{
			continue;
		}

		wchar_t verb[64] = L"";
		HRESULT hr =
			m_contextMenu->GetCommandString(menuItemInfo.wID - ShellContextMenu::MIN_SHELL_MENU_ID,
				GCS_VERB, nullptr, reinterpret_cast<LPSTR>(verb), std::size(verb));

		if (FAILED(hr))
		{
			continue;
		}

		if (boost::iequals(verb, targetVerb))
		{
			DeleteMenu(m_menu, i, MF_BYPOSITION);
		}
	}
}
