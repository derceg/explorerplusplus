// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/optional.hpp>

__interface TabInterface
{
	/* TODO: Ideally, there would be a method of iterating over the tabs
	without having access to the underlying container. */
	const std::unordered_map<int, Tab>	&GetAllTabs() const;

	boost::optional<Tab>	GetTab(int tabId);
	std::wstring	GetTabName(int iTab);
	void			SetTabName(int iTab, std::wstring strName, BOOL bUseCustomName);
	void			RefreshTab(int iTabId);
	int				GetCurrentTabId() const;
	int				MoveTab(const Tab &tab, int newIndex);
	int				GetNumTabs() const;
	bool			CloseTab(const Tab &tab);

	HRESULT			CreateNewTab(const TCHAR *TabDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);
	HRESULT			CreateNewTab(LPCITEMIDLIST pidlDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);

	/* Temporary. */
	void			SetTabSelection(int Index);
};