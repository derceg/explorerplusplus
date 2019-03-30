// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainer.h"
#include "Explorer++_internal.h"
#include "../Helper/TabHelper.h"
#include <boost/algorithm/string.hpp>

CTabContainer::CTabContainer(HWND hTabCtrl, std::unordered_map<int, Tab> *tabInfo, TabContainerInterface *tabContainer) :
	m_hTabCtrl(hTabCtrl),
	m_tabInfo(tabInfo),
	m_tabContainer(tabContainer)
{
	m_navigationCompletedConnection = m_tabContainer->AddNavigationCompletedObserver(boost::bind(&CTabContainer::OnNavigationCompleted, this, _1));
	m_tabUpdatedConnection = m_tabContainer->AddTabUpdatedObserver(boost::bind(&CTabContainer::OnTabUpdated, this, _1, _2));
}

CTabContainer::~CTabContainer()
{
	m_navigationCompletedConnection.disconnect();
	m_tabUpdatedConnection.disconnect();
}

int CTabContainer::GetSelection()
{
	int Index = TabCtrl_GetCurSel(m_hTabCtrl);
	assert(Index != -1);

	return Index;
}

CShellBrowser *CTabContainer::GetBrowserForTab(int Index)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(m_hTabCtrl,Index,&tcItem);
	assert(res);

	if(!res)
	{
		return NULL;
	}

	return m_tabInfo->at(static_cast<int>(tcItem.lParam)).GetShellBrowser();
}

void CTabContainer::OnNavigationCompleted(const Tab &tab)
{
	UpdateTabNameInWindow(tab);
}

void CTabContainer::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::NAME:
		UpdateTabNameInWindow(tab);
		break;
	}
}

void CTabContainer::UpdateTabNameInWindow(const Tab &tab)
{
	std::wstring name = tab.GetName();
	boost::replace_all(name, L"&", L"&&");

	int index = m_tabContainer->GetTabIndex(tab);
	TabCtrl_SetItemText(m_hTabCtrl, index, name.c_str());
}