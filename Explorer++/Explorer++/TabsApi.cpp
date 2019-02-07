// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabsAPI.h"

Plugins::TabsApi::TabsApi(TabContainerInterface *tabContainer) :
	m_tabContainer(tabContainer),
	m_observerIdCounter(1)
{

}

Plugins::TabsApi::~TabsApi()
{
	for (auto item : m_tabCreatedConnections)
	{
		item.second.disconnect();
	}
}

std::vector<Plugins::TabsApi::Tab> Plugins::TabsApi::getAll()
{
	std::vector<Tab> tabs;

	for (auto item : m_tabContainer->GetAllTabs())
	{
		Tab tab(item.second);
		tabs.push_back(tab);
	}

	return tabs;
}

boost::optional<Plugins::TabsApi::Tab> Plugins::TabsApi::get(int tabId)
{
	auto tabInternal = m_tabContainer->GetTab(tabId);

	if (!tabInternal)
	{
		return boost::none;
	}

	Tab tab(*tabInternal);

	return tab;
}

/* TODO: This function
should return a value.
Probably shouldn't
return a HRESULT though. */
void Plugins::TabsApi::create(std::wstring path)
{
	m_tabContainer->CreateNewTab(path.c_str(), nullptr, nullptr, TRUE, nullptr);
}

int Plugins::TabsApi::move(int tabId, int newIndex)
{
	auto tabInternal = m_tabContainer->GetTab(tabId);

	if (!tabInternal)
	{
		return -1;
	}

	if (newIndex < 0)
	{
		newIndex = m_tabContainer->GetNumTabs();
	}

	return m_tabContainer->MoveTab(*tabInternal, newIndex);
}

bool Plugins::TabsApi::close(int tabId)
{
	auto tabInternal = m_tabContainer->GetTab(tabId);

	if (!tabInternal)
	{
		return false;
	}

	return m_tabContainer->CloseTab(*tabInternal);
}

int Plugins::TabsApi::addTabCreatedObserver(sol::protected_function observer)
{
	if (!observer)
	{
		return -1;
	}

	auto connection = m_tabContainer->AddTabCreatedObserver([this, observer] (int tabId, BOOL switchToNewTab) {
		UNREFERENCED_PARAMETER(switchToNewTab);

		onTabCreated(tabId, observer);
	});

	int id = m_observerIdCounter++;
	m_tabCreatedConnections.insert(std::make_pair(id, connection));

	return id;
}

void Plugins::TabsApi::onTabCreated(int tabId, sol::protected_function observer)
{
	auto tabInternal = m_tabContainer->GetTab(tabId);

	if (!tabInternal)
	{
		return;
	}

	Tab tab(*tabInternal);
	observer(tab);
}

void Plugins::TabsApi::removeTabCreatedObserver(int id)
{
	auto itr = m_tabCreatedConnections.find(id);

	if (itr == m_tabCreatedConnections.end())
	{
		return;
	}

	itr->second.disconnect();

	m_tabCreatedConnections.erase(itr);
}