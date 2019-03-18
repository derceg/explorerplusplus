// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabsAPI.h"

Plugins::TabsApi::TabsApi(TabContainerInterface *tabContainer, TabInterface *tabInterface) :
	m_tabContainer(tabContainer),
	m_tabInterface(tabInterface),
	m_tabCreatedIdCounter(1),
	m_tabRemovedIdCounter(1)
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

void Plugins::TabsApi::update(int tabId, sol::table properties)
{
	auto tabInternal = m_tabContainer->GetTab(tabId);

	if (!tabInternal)
	{
		return;
	}

	boost::optional<std::wstring> location = properties["location"];

	if (location && !location->empty())
	{
		m_tabContainer->BrowseFolder(*tabInternal, location->c_str(), SBSP_ABSOLUTE);
	}

	boost::optional<std::wstring> name = properties["name"];

	if (name)
	{
		if (name->empty())
		{
			m_tabInterface->ClearTabName(*tabInternal);
		}
		else
		{
			m_tabInterface->SetTabName(*tabInternal, *name);
		}
	}

	boost::optional<int> viewMode = properties["viewMode"];

	if (viewMode)
	{
		/* TODO: It would be better if checks like this were done by an
		enum library. */
		if (viewMode >= static_cast<int>(ViewMode::FIRST) && viewMode <= static_cast<int>(ViewMode::LAST))
		{
			tabInternal->shellBrowser->SetCurrentViewMode(static_cast<ViewMode>(*viewMode));
		}
	}

	boost::optional<bool> locked = properties["locked"];

	if (locked)
	{
		m_tabContainer->LockTab(*tabInternal, *locked);
	}

	boost::optional<bool> addressLocked = properties["addressLocked"];

	if (addressLocked)
	{
		m_tabContainer->LockTabAndAddress(*tabInternal, *addressLocked);
	}

	boost::optional<bool> active = properties["active"];

	if (active && *active)
	{
		m_tabContainer->SelectTab(*tabInternal);
	}
}

void Plugins::TabsApi::refresh(int tabId)
{
	auto tabInternal = m_tabContainer->GetTab(tabId);

	if (!tabInternal)
	{
		return;
	}

	m_tabInterface->RefreshTab(*tabInternal);
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

	int id = m_tabCreatedIdCounter++;
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

int Plugins::TabsApi::addTabRemovedObserver(sol::protected_function observer)
{
	if (!observer)
	{
		return -1;
	}

	auto connection = m_tabContainer->AddTabRemovedObserver(observer);

	int id = m_tabRemovedIdCounter++;
	m_tabRemovedConnections.insert(std::make_pair(id, connection));

	return id;
}

void Plugins::TabsApi::removeTabRemovedObserver(int id)
{
	auto itr = m_tabRemovedConnections.find(id);

	if (itr == m_tabRemovedConnections.end())
	{
		return;
	}

	itr->second.disconnect();

	m_tabRemovedConnections.erase(itr);
}