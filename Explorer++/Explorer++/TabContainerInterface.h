// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <unordered_map>

typedef boost::signals2::signal<void(int, BOOL)> TabCreatedSignal;
typedef boost::signals2::signal<void(int)> TabRemovedSignal;

// Eventually, this will be driven by a dedicated class, rather than the
// Explorerplusplus class.
__interface TabContainerInterface
{
	/* TODO: Ideally, there would be a method of iterating over the tabs
	without having access to the underlying container. */
	const std::unordered_map<int, Tab>	&GetAllTabs() const;

	Tab				*GetTab(int tabId);
	int				GetCurrentTabId() const;
	int				MoveTab(const Tab &tab, int newIndex);
	int				GetNumTabs() const;
	void			LockTab(Tab &tab, bool lock);
	void			LockTabAndAddress(Tab &tab, bool lock);
	bool			CloseTab(const Tab &tab);

	HRESULT			CreateNewTab(const TCHAR *TabDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);
	HRESULT			CreateNewTab(LPCITEMIDLIST pidlDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);

	/* Temporary. */
	void			SetTabSelection(int Index);

	boost::signals2::connection	AddTabCreatedObserver(const TabCreatedSignal::slot_type &observer);
	boost::signals2::connection	AddTabRemovedObserver(const TabRemovedSignal::slot_type &observer);
};