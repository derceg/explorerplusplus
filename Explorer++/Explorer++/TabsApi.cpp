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

/* TODO: This function
should return a value.
Probably shouldn't
return a HRESULT though. */
void Plugins::TabsApi::create(std::wstring path)
{
	m_tabContainer->CreateNewTab(path.c_str(), nullptr, {}, TRUE, nullptr);
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