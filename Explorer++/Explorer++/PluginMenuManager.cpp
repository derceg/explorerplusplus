// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PluginMenuManager.h"
#include "MainResource.h"
#include "../Helper/Macros.h"
#include <boost/iterator/counting_iterator.hpp>

Plugins::PluginMenuManager::PluginMenuManager(HWND mainWindow, int startId, int endId) :
	m_mainWindow(mainWindow),
	m_startId(startId),
	m_endId(endId)
{
	// Note that the start ID is inclusive and the end ID is exclusive.
	m_freeMenuItemIds.insert(boost::counting_iterator<int>(startId), boost::counting_iterator<int>(endId));
}

boost::optional<int> Plugins::PluginMenuManager::AddItemToMainMenu(const std::wstring &text)
{
	HMENU menu = GetMenu(m_mainWindow);

	if (menu == nullptr)
	{
		return boost::none;
	}

	auto id = GeneratePluginMenuItemId();

	// There are a finite number of menu positions available to
	// plugins. The limit shouldn't actually be hit in practice.
	if (!id)
	{
		return id;
	}

	auto textString = std::make_unique<TCHAR[]>(text.size() + 1);
	StringCchCopy(textString.get(), text.size() + 1, text.c_str());

	/* TODO: The menu position should be configurable. */
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = *id;
	mii.dwTypeData = textString.get();
	BOOL res = InsertMenuItem(menu, IDM_TOOLS_RUNSCRIPT, FALSE, &mii);

	if (!res)
	{
		return boost::none;
	}

	return *id;
}

void Plugins::PluginMenuManager::RemoveItemFromMainMenu(int menuItemId)
{
	HMENU menu = GetMenu(m_mainWindow);

	if (menu == nullptr)
	{
		return;
	}

	if (menuItemId < m_startId || menuItemId >= m_endId)
	{
		return;
	}

	DeleteMenu(menu, menuItemId, MF_BYCOMMAND);

	ReleasePluginMenuItemId(menuItemId);
}

boost::optional<int> Plugins::PluginMenuManager::GeneratePluginMenuItemId()
{
	if (m_freeMenuItemIds.empty())
	{
		return boost::none;
	}

	auto first = m_freeMenuItemIds.begin();

	int id = *first;

	m_freeMenuItemIds.erase(first);

	return id;
}

void Plugins::PluginMenuManager::ReleasePluginMenuItemId(int id)
{
	m_freeMenuItemIds.insert(id);
}

boost::signals2::connection Plugins::PluginMenuManager::AddMenuClickedObserver(const PluginMenuClickedSignal::slot_type &observer)
{
	return m_menuClickedSignal.connect(observer);
}

void Plugins::PluginMenuManager::OnMenuItemClicked(int menuItemId)
{
	assert(menuItemId >= m_startId && menuItemId < m_endId);

	m_menuClickedSignal(menuItemId);
}