// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/optional.hpp>

// Eventually, this will be driven by a dedicated class, rather than the
// Explorerplusplus class.
__interface TabContainerInterface
{
	/* TODO: Ideally, there would be a method of iterating over the tabs
	without having access to the underlying container. */
	const std::unordered_map<int, Tab>	&GetAllTabs() const;

	boost::optional<Tab>	GetTab(int tabId);
	int				GetCurrentTabId() const;
	int				MoveTab(const Tab &tab, int newIndex);
	int				GetNumTabs() const;
	bool			CloseTab(const Tab &tab);

	HRESULT			CreateNewTab(const TCHAR *TabDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);
	HRESULT			CreateNewTab(LPCITEMIDLIST pidlDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);

	/* Temporary. */
	void			SetTabSelection(int Index);
};