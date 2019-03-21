// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <unordered_map>

typedef boost::signals2::signal<void(int tabId, BOOL switchToNewTab)> TabCreatedSignal;
typedef boost::signals2::signal<void(const Tab &tab, int fromIndex, int toIndex)> TabMovedSignal;
typedef boost::signals2::signal<void(const Tab &tab, Tab::PropertyType propertyType)> TabUpdatedSignal;
typedef boost::signals2::signal<void(int tabId)> TabRemovedSignal;

// Eventually, this will be driven by a dedicated class, rather than the
// Explorerplusplus class.
__interface TabContainerInterface
{
	/* TODO: Ideally, there would be a method of iterating over the tabs
	without having access to the underlying container. */
	const std::unordered_map<int, Tab>	&GetAllTabs() const;

	Tab				*GetTabOptional(int tabId);
	Tab				&GetTabByIndex(int index);
	int				GetSelectedTabId() const;
	int				GetSelectedTabIndex() const;
	void			SelectTab(const Tab &tab);
	int				MoveTab(const Tab &tab, int newIndex);
	int				GetNumTabs() const;
	bool			CloseTab(const Tab &tab);

	HRESULT			CreateNewTab(const TCHAR *TabDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);
	HRESULT			CreateNewTab(LPCITEMIDLIST pidlDirectory, InitialSettings_t *pSettings, TabSettings *pTabSettings, BOOL bSwitchToNewTab, int *pTabObjectIndex);

	HRESULT			BrowseFolderInCurrentTab(const TCHAR *szPath, UINT wFlags);
	HRESULT			BrowseFolder(Tab &tab, const TCHAR *szPath, UINT wFlags);
	HRESULT			BrowseFolderInCurrentTab(LPCITEMIDLIST pidlDirectory, UINT wFlags);
	HRESULT			BrowseFolder(Tab &tab, LPCITEMIDLIST pidlDirectory, UINT wFlags);

	boost::signals2::connection	AddTabCreatedObserver(const TabCreatedSignal::slot_type &observer);
	boost::signals2::connection	AddTabMovedObserver(const TabMovedSignal::slot_type &observer);
	boost::signals2::connection	AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer);
	boost::signals2::connection	AddTabRemovedObserver(const TabRemovedSignal::slot_type &observer);
};