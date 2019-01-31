// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabsAPI.h"

Plugins::TabsApi::TabsApi(IExplorerplusplus *pexpp, TabInterface *ti) :
	m_pexpp(pexpp),
	m_ti(ti)
{

}

Plugins::TabsApi::~TabsApi()
{

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
	m_pexpp->BrowseFolder(path.c_str(), SBSP_ABSOLUTE, TRUE, TRUE);
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