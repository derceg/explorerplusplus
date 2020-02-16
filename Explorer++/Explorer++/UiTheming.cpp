// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "UiTheming.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellBrowser.h"
#include "Tab.h"
#include "TabContainer.h"

UiTheming::UiTheming(IExplorerplusplus *expp, TabContainer *tabContainer) :
	m_expp(expp),
	m_tabContainer(tabContainer),
	m_customListViewColorsApplied(false)
{
	m_connections.push_back(m_tabContainer->tabCreatedSignal.AddObserver(boost::bind(&UiTheming::OnTabCreated, this, _1, _2)));
}

void UiTheming::OnTabCreated(int tabId, BOOL switchToNewTab)
{
	UNREFERENCED_PARAMETER(switchToNewTab);

	const Tab &tab = m_tabContainer->GetTab(tabId);

	if (m_customListViewColorsApplied)
	{
		ApplyListViewColorsForTab(tab, m_listViewBackgroundColor, m_listViewTextColor);
	}
}

bool UiTheming::SetListViewColors(COLORREF backgroundColor, COLORREF textColor)
{
	m_customListViewColorsApplied = true;
	m_listViewBackgroundColor = backgroundColor;
	m_listViewTextColor = textColor;

	bool res = ApplyListViewColorsForAllTabs(backgroundColor, textColor);

	return res;
}

bool UiTheming::ApplyListViewColorsForAllTabs(COLORREF backgroundColor, COLORREF textColor)
{
	bool overallResult = true;

	for (const auto &item : m_tabContainer->GetAllTabs())
	{
		bool res = ApplyListViewColorsForTab(*item.second, backgroundColor, textColor);

		if (!res)
		{
			overallResult = false;
		}
	}

	return overallResult;
}

bool UiTheming::ApplyListViewColorsForTab(const Tab &tab, COLORREF backgroundColor, COLORREF textColor)
{
	BOOL bkRes = ListView_SetBkColor(tab.GetShellBrowser()->GetListView(), backgroundColor);
	BOOL textBkRes = ListView_SetTextBkColor(tab.GetShellBrowser()->GetListView(), backgroundColor);
	BOOL textRes = ListView_SetTextColor(tab.GetShellBrowser()->GetListView(), textColor);

	InvalidateRect(tab.GetShellBrowser()->GetListView(), nullptr, TRUE);

	if (!bkRes || !textBkRes || !textRes)
	{
		return false;
	}

	return true;
}

void UiTheming::SetTreeViewColors(COLORREF backgroundColor, COLORREF textColor)
{
	TreeView_SetBkColor(m_expp->GetTreeView(), backgroundColor);
	TreeView_SetTextColor(m_expp->GetTreeView(), textColor);

	InvalidateRect(m_expp->GetTreeView(), nullptr, TRUE);
}