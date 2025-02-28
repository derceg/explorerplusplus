// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GlobalTabEventDispatcher.h"
#include "BrowserWindow.h"
#include "Tab.h"

boost::signals2::connection GlobalTabEventDispatcher::AddCreatedObserver(
	const CreatedSignal::slot_type &observer, BrowserWindow *browser,
	boost::signals2::connect_position position)
{
	return m_createdSignal.connect(
		[browserId = GetIdFromBrowser(browser), observer](const Tab &tab, bool selected)
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
	const SelectedSignal::slot_type &observer, BrowserWindow *browser,
	boost::signals2::connect_position position)
{
	return m_selectedSignal.connect(
		[browserId = GetIdFromBrowser(browser), observer](const Tab &tab)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab);
		},
		position);
}

boost::signals2::connection GlobalTabEventDispatcher::AddPreRemovalObserver(
	const PreRemovalSignal::slot_type &observer, BrowserWindow *browser,
	boost::signals2::connect_position position)
{
	return m_preRemovalSignal.connect(
		[browserId = GetIdFromBrowser(browser), observer](const Tab &tab, int index)
		{
			if (!DoesBrowserMatch(browserId, tab))
			{
				return;
			}

			observer(tab, index);
		},
		position);
}

std::optional<int> GlobalTabEventDispatcher::GetIdFromBrowser(BrowserWindow *browser)
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

void GlobalTabEventDispatcher::NotifyPreRemoval(const Tab &tab, int index)
{
	m_preRemovalSignal(tab, index);
}
