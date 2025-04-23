// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SearchTabsModel.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabEvents.h"
#include "TabList.h"
#include "../Helper/ShellHelper.h"
#include <boost/algorithm/string/predicate.hpp>
#include <ranges>

SearchTabsModel::SearchTabsModel(const TabList *tabList, TabEvents *tabEvents,
	ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents) :
	m_tabList(tabList)
{
	m_connections.push_back(tabEvents->AddCreatedObserver(
		std::bind(&SearchTabsModel::OnTabsChanged, this), TabEventScope::Global()));
	m_connections.push_back(tabEvents->AddSelectedObserver(
		std::bind(&SearchTabsModel::OnTabsChanged, this), TabEventScope::Global()));
	m_connections.push_back(tabEvents->AddUpdatedObserver(
		std::bind(&SearchTabsModel::OnTabsChanged, this), TabEventScope::Global()));
	m_connections.push_back(tabEvents->AddMovedObserver(
		std::bind(&SearchTabsModel::OnTabsChanged, this), TabEventScope::Global()));
	m_connections.push_back(tabEvents->AddRemovedObserver(
		std::bind(&SearchTabsModel::OnTabsChanged, this), TabEventScope::Global()));

	m_connections.push_back(shellBrowserEvents->AddDirectoryPropertiesChangedObserver(
		std::bind(&SearchTabsModel::OnTabsChanged, this), ShellBrowserEventScope::Global()));

	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind(&SearchTabsModel::OnTabsChanged, this), NavigationEventScope::Global()));
}

void SearchTabsModel::SetSearchTerm(const std::wstring &searchTerm)
{
	if (searchTerm == m_searchTerm)
	{
		return;
	}

	m_searchTerm = searchTerm;

	updatedSignal.m_signal();
}

const std::wstring &SearchTabsModel::GetSearchTerm() const
{
	return m_searchTerm;
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<Tab *> SearchTabsModel::GetResults() const
{
	auto tabs = m_tabList->GetAllByLastActiveTime();

	for (auto *tab : tabs | std::views::filter(std::bind_front(&SearchTabsModel::TabFilter, this)))
	{
		co_yield tab;
	}
}

void SearchTabsModel::OnTabsChanged()
{
	updatedSignal.m_signal();
}

bool SearchTabsModel::TabFilter(const Tab *tab) const
{
	if (m_searchTerm.empty())
	{
		return true;
	}

	if (boost::icontains(tab->GetName(), m_searchTerm))
	{
		return true;
	}

	if (boost::icontains(GetTabDirectory(tab), m_searchTerm))
	{
		return true;
	}

	return false;
}

std::wstring SearchTabsModel::GetTabDirectory(const Tab *tab)
{
	const auto *currentEntry = tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
	return GetDisplayNameWithFallback(currentEntry->GetPidl().Raw(), SHGDN_FORPARSING);
}
