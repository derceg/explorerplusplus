// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/optional.hpp>

__interface TabInterface
{
	boost::optional<TabInfo_t>	GetTab(int tabId);
	std::wstring	GetTabName(int iTab);
	void			SetTabName(int iTab, std::wstring strName, BOOL bUseCustomName);
	void			RefreshTab(int iTabId);
	int				GetCurrentTabId() const;
	int				MoveTab(const TabInfo_t &tab, int newIndex);
	int				GetNumTabs() const;
	bool			CloseTab(const TabInfo_t &tab);

	HRESULT			CreateNewTab(const TCHAR *TabDirectory, InitialSettings_t *pSettings, TabInfo_t *pTabInfo, BOOL bSwitchToNewTab, int *pTabObjectIndex);
	HRESULT			CreateNewTab(LPCITEMIDLIST pidlDirectory, InitialSettings_t *pSettings, TabInfo_t *pTabInfo, BOOL bSwitchToNewTab, int *pTabObjectIndex);

	/* Temporary. */
	void			SetTabSelection(int Index);
};