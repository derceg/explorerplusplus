// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabEvents.h"
#include "BrowserWindow.h"
#include "ShellBrowser/ShellBrowser.h"
#include "Tab.h"

TabEventScope TabEventScope::ForBrowser(const BrowserWindow &browser)
{
	return TabEventScope(browser);
}

TabEventScope TabEventScope::Global()
{
	return TabEventScope();
}

TabEventScope::TabEventScope() : m_scope(Scope::Global)
{
}

TabEventScope::TabEventScope(const BrowserWindow &browser) :
	m_scope(Scope::Browser),
	m_browserId(browser.GetId())
{
}

TabEventScope::Scope TabEventScope::GetScope() const
{
	return m_scope;
}

std::optional<int> TabEventScope::GetBrowserId() const
{
	return m_browserId;
}

boost::signals2::connection TabEvents::AddCreatedObserver(const CreatedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position)
{
	return m_createdSignal.connect(MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddSelectedObserver(
	const SelectedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_selectedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddMovedObserver(const MovedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position)
{
	return m_movedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddPreRemovalObserver(
	const PreRemovalSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position)
{
	return m_preRemovalSignal.connect(MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddRemovedObserver(const RemovedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position)
{
	return m_removedSignal.connect(MakeFilteredObserver(observer, scope), position);
}

bool TabEvents::DoesEventMatchScope(const TabEventScope &scope, const Tab &tab)
{
	switch (scope.GetScope())
	{
	case TabEventScope::Scope::Browser:
		return tab.GetBrowser()->GetId() == scope.GetBrowserId();

	case TabEventScope::Scope::Global:
	default:
		return true;
	}
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
