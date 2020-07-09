// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

__interface IExplorerplusplus;
class Tab;
class TabContainer;

class UiTheming
{
public:
	UiTheming(IExplorerplusplus *expp, TabContainer *tabContainer);

	bool SetListViewColors(COLORREF backgroundColor, COLORREF textColor);
	void SetTreeViewColors(COLORREF backgroundColor, COLORREF textColor);

private:
	void OnTabCreated(int tabId, BOOL switchToNewTab);

	bool ApplyListViewColorsForAllTabs(COLORREF backgroundColor, COLORREF textColor);
	bool ApplyListViewColorsForTab(const Tab &tab, COLORREF backgroundColor, COLORREF textColor);

	IExplorerplusplus *m_expp;
	TabContainer *m_tabContainer;

	std::vector<boost::signals2::scoped_connection> m_connections;

	bool m_customListViewColorsApplied;
	COLORREF m_listViewBackgroundColor;
	COLORREF m_listViewTextColor;
};