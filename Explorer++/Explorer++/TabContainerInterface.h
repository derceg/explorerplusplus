// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/signals2.hpp>
#include <unordered_map>

typedef boost::signals2::signal<void(const Tab &tab)> TabSelectedSignal;
typedef boost::signals2::signal<void(int tabId)> TabRemovedSignal;

typedef boost::signals2::signal<void(const Tab &tab)> NavigationCompletedSignal;

// Eventually, this will be driven by a dedicated class, rather than the
// Explorerplusplus class.
__interface TabContainerInterface
{
	bool			CloseTab(const Tab &tab);

	void			OnTabSelectionChanged(bool broadcastEvent = true);

	HRESULT			BrowseFolderInCurrentTab(const TCHAR *szPath, UINT wFlags);
	HRESULT			BrowseFolder(Tab &tab, const TCHAR *szPath, UINT wFlags);
	HRESULT			BrowseFolderInCurrentTab(LPCITEMIDLIST pidlDirectory, UINT wFlags);
	HRESULT			BrowseFolder(Tab &tab, LPCITEMIDLIST pidlDirectory, UINT wFlags);

	boost::signals2::connection	AddTabSelectedObserver(const TabSelectedSignal::slot_type &observer);
	boost::signals2::connection	AddTabRemovedObserver(const TabRemovedSignal::slot_type &observer);

	boost::signals2::connection	AddNavigationCompletedObserver(const NavigationCompletedSignal::slot_type &observer);
};