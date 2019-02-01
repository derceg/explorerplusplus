// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabsAPI.h"

Plugins::TabsApi::TabsApi(TabInterface *ti) :
	m_ti(ti)
{

}

Plugins::TabsApi::~TabsApi()
{

}

std::vector<Plugins::TabsApi::Tab> Plugins::TabsApi::getAll()
{
	std::vector<Tab> tabs;

	for (auto item : m_ti->GetAllTabs())
	{
		Tab tab(item.second);
		tabs.push_back(tab);
	}

	return tabs;
}

boost::optional<Plugins::TabsApi::Tab> Plugins::TabsApi::get(int tabId)
{
	auto tabInternal = m_ti->GetTab(tabId);

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
	m_ti->CreateNewTab(path.c_str(), nullptr, nullptr, TRUE, nullptr);
}

int Plugins::TabsApi::move(int tabId, int newIndex)
{
	auto tabInternal = m_ti->GetTab(tabId);

	if (!tabInternal)
	{
		return -1;
	}

	if (newIndex < 0)
	{
		newIndex = m_ti->GetNumTabs();
	}

	return m_ti->MoveTab(*tabInternal, newIndex);
}

bool Plugins::TabsApi::close(int tabId)
{
	auto tabInternal = m_ti->GetTab(tabId);

	if (!tabInternal)
	{
		return false;
	}

	return m_ti->CloseTab(*tabInternal);
}