// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GlobalTabEventDispatcher.h"
#include "BrowserWindow.h"
#include "ShellBrowser/ShellBrowser.h"
#include "Tab.h"
#include "../Helper/WeakPtr.h"

TabEventScope TabEventScope::ForBrowser(BrowserWindow *browser)
{
	DCHECK(browser != nullptr);
	return TabEventScope(browser);
}

TabEventScope TabEventScope::Global()
{
	return TabEventScope(nullptr);
}

const BrowserWindow *TabEventScope::GetBrowser() const
{
	return m_browser;
}

TabEventScope::TabEventScope(BrowserWindow *browser) : m_browser(browser)
{
}

boost::signals2::connection GlobalTabEventDispatcher::AddCreatedObserver(
	const CreatedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_createdSignal.connect(
		[browserId = GetIdFromBrowser(scope.GetBrowser()), observer](const Tab &tab, bool selected)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab, selected);
		},
		position);
}

boost::signals2::connection GlobalTabEventDispatcher::AddSelectedObserver(
	const SelectedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_selectedSignal.connect(
		[browserId = GetIdFromBrowser(scope.GetBrowser()), observer](const Tab &tab)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab);
		},
		position);
}

boost::signals2::connection GlobalTabEventDispatcher::AddMovedObserver(
	const MovedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_movedSignal.connect(
		[browserId = GetIdFromBrowser(scope.GetBrowser()), observer](const Tab &tab, int fromIndex,
			int toIndex)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab, fromIndex, toIndex);
		},
		position);
}

boost::signals2::connection GlobalTabEventDispatcher::AddPreRemovalObserver(
	const PreRemovalSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_preRemovalSignal.connect(
		[browserId = GetIdFromBrowser(scope.GetBrowser()), observer](const Tab &tab, int index)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab, index);
		},
		position);
}

boost::signals2::connection GlobalTabEventDispatcher::AddRemovedObserver(
	const RemovedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_removedSignal.connect(
		[browserId = GetIdFromBrowser(scope.GetBrowser()), observer](const Tab &tab)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab);
		},
		position);
}

boost::signals2::connection GlobalTabEventDispatcher::AddNavigationStartedObserver(
	const NavigationStartedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_navigationStartedSignal.connect(
		[browserId = GetIdFromBrowser(scope.GetBrowser()), observer](const Tab &tab,
			const NavigationRequest *request)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab, request);
		},
		position);
}

boost::signals2::connection GlobalTabEventDispatcher::AddNavigationCommittedObserver(
	const NavigationCommittedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_navigationCommittedSignal.connect(
		[browserId = GetIdFromBrowser(scope.GetBrowser()), observer](const Tab &tab,
			const NavigationRequest *request)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab, request);
		},
		position);
}

std::optional<int> GlobalTabEventDispatcher::GetIdFromBrowser(const BrowserWindow *browser)
{
	if (!browser)
	{
		return std::nullopt;
	}

	return browser->GetId();
}

bool GlobalTabEventDispatcher::DoesBrowserMatch(std::optional<int> browserId, const Tab &tab)
{
	if (!browserId)
	{
		return true;
	}

	return *browserId == tab.GetBrowser()->GetId();
}

void GlobalTabEventDispatcher::NotifyCreated(const Tab &tab, bool selected)
{
	// This class is implicitly designed to have a lifetime that's longer than any individual tab,
	// so accessing this class in the observer should always be safe (access through the WeakPtr
	// will deterministically fail if the instance has been destroyed).
	//
	// That also means that there's no need to disconnect this observer, since this class will
	// always outlive the tab.
	//
	// Also, capturing the tab by reference here is safe, since the tab object is guaranteed to
	// exist whenever this method is called.
	tab.GetShellBrowser()->AddNavigationStartedObserver(
		[weakSelf = m_weakPtrFactory.GetWeakPtr(), &tab](const NavigationRequest *request)
		{ weakSelf->m_navigationStartedSignal(tab, request); });

	tab.GetShellBrowser()->AddNavigationCommittedObserver(
		[weakSelf = m_weakPtrFactory.GetWeakPtr(), &tab](const NavigationRequest *request)
		{ weakSelf->m_navigationCommittedSignal(tab, request); });

	m_createdSignal(tab, selected);
}

void GlobalTabEventDispatcher::NotifySelected(const Tab &tab)
{
	m_selectedSignal(tab);
}

void GlobalTabEventDispatcher::NotifyMoved(const Tab &tab, int fromIndex, int toIndex)
{
	m_movedSignal(tab, fromIndex, toIndex);
}

void GlobalTabEventDispatcher::NotifyPreRemoval(const Tab &tab, int index)
{
	m_preRemovalSignal(tab, index);
}

void GlobalTabEventDispatcher::NotifyRemoved(const Tab &tab)
{
	m_removedSignal(tab);
}
