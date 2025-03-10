// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabEvents.h"
#include "BrowserWindow.h"
#include "ShellBrowser/ShellBrowser.h"
#include "Tab.h"

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

boost::signals2::connection TabEvents::AddCreatedObserver(const CreatedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position)
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

boost::signals2::connection TabEvents::AddSelectedObserver(
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

boost::signals2::connection TabEvents::AddMovedObserver(const MovedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position)
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

boost::signals2::connection TabEvents::AddPreRemovalObserver(
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

boost::signals2::connection TabEvents::AddRemovedObserver(const RemovedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position)
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

std::optional<int> TabEvents::GetIdFromBrowser(const BrowserWindow *browser)
{
	if (!browser)
	{
		return std::nullopt;
	}

	return browser->GetId();
}

bool TabEvents::DoesBrowserMatch(std::optional<int> browserId, const Tab &tab)
{
	if (!browserId)
	{
		return true;
	}

	return *browserId == tab.GetBrowser()->GetId();
}

void TabEvents::NotifyCreated(const Tab &tab, bool selected)
{
	m_createdSignal(tab, selected);
}

void TabEvents::NotifySelected(const Tab &tab)
{
	m_selectedSignal(tab);
}

void TabEvents::NotifyMoved(const Tab &tab, int fromIndex, int toIndex)
{
	m_movedSignal(tab, fromIndex, toIndex);
}

void TabEvents::NotifyPreRemoval(const Tab &tab, int index)
{
	m_preRemovalSignal(tab, index);
}

void TabEvents::NotifyRemoved(const Tab &tab)
{
	m_removedSignal(tab);
}
