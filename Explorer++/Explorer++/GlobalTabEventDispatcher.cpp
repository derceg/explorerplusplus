// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GlobalTabEventDispatcher.h"
#include "BrowserWindow.h"
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
