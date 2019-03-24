// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabsAPI.h"

Plugins::TabsApi::TabsApi(TabContainerInterface *tabContainer, TabInterface *tabInterface) :
	m_tabContainer(tabContainer),
	m_tabInterface(tabInterface)
{

}

Plugins::TabsApi::~TabsApi()
{

}

std::vector<Plugins::TabsApi::Tab> Plugins::TabsApi::getAll()
{
	std::vector<Tab> tabs;

	for (auto &item : m_tabContainer->GetAllTabs())
	{
		Tab tab(item.second);
		tabs.push_back(tab);
	}

	return tabs;
}

boost::optional<Plugins::TabsApi::Tab> Plugins::TabsApi::get(int tabId)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return boost::none;
	}

	Tab tab(*tabInternal);

	return tab;
}

int Plugins::TabsApi::create(sol::table createProperties)
{
	boost::optional<std::wstring> location = createProperties["location"];

	if (!location || location->empty())
	{
		return -1;
	}

	TabSettings tabSettings;

	boost::optional<std::wstring> name = createProperties["name"];

	if (name && !name->empty())
	{
		tabSettings.name = name;
	}

	boost::optional<int> index = createProperties["index"];

	if (index)
	{
		int finalIndex = *index;
		int numTabs = m_tabContainer->GetNumTabs();

		if (finalIndex < 0)
		{
			finalIndex = 0;
		}
		else if (finalIndex > numTabs)
		{
			finalIndex = numTabs;
		}

		tabSettings.index = finalIndex;
	}

	boost::optional<bool> active = createProperties["active"];

	if (active)
	{
		tabSettings.selected = *active;
	}

	boost::optional<bool> locked = createProperties["locked"];

	if (locked)
	{
		tabSettings.locked = *locked;
	}

	boost::optional<bool> addressLocked = createProperties["addressLocked"];

	if (addressLocked)
	{
		tabSettings.addressLocked = *addressLocked;
	}

	int tabId;
	HRESULT hr = m_tabContainer->CreateNewTab(location->c_str(), tabSettings, nullptr, nullptr, &tabId);

	if (FAILED(hr))
	{
		/* TODO: Ideally, an error message would be available in case of
		failure. */
		return -1;
	}

	return tabId;
}

void Plugins::TabsApi::update(int tabId, sol::table properties)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

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
			tabInternal->ClearCustomName();
		}
		else
		{
			tabInternal->SetCustomName(*name);
		}
	}

	boost::optional<int> viewMode = properties["viewMode"];

	if (viewMode)
	{
		/* TODO: It would be better if checks like this were done by an
		enum library. */
		if (viewMode >= static_cast<int>(ViewMode::FIRST) && viewMode <= static_cast<int>(ViewMode::LAST))
		{
			tabInternal->GetShellBrowser()->SetCurrentViewMode(static_cast<ViewMode>(*viewMode));
		}
	}

	boost::optional<bool> locked = properties["locked"];

	if (locked)
	{
		tabInternal->SetLocked(*locked);
	}

	boost::optional<bool> addressLocked = properties["addressLocked"];

	if (addressLocked)
	{
		tabInternal->SetAddressLocked(*addressLocked);
	}

	boost::optional<bool> active = properties["active"];

	if (active && *active)
	{
		m_tabContainer->SelectTab(*tabInternal);
	}
}

void Plugins::TabsApi::refresh(int tabId)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return;
	}

	m_tabInterface->RefreshTab(*tabInternal);
}

int Plugins::TabsApi::move(int tabId, int newIndex)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

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
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return false;
	}

	return m_tabContainer->CloseTab(*tabInternal);
}