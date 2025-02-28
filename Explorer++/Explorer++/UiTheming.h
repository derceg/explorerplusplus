// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

class App;
class CoreInterface;
class Tab;
class TabContainer;

class UiTheming
{
public:
	UiTheming(App *app, CoreInterface *coreInterface, TabContainer *tabContainer);

	bool SetListViewColors(COLORREF backgroundColor, COLORREF textColor);
	void SetTreeViewColors(COLORREF backgroundColor, COLORREF textColor);

private:
	void OnTabCreated(const Tab &tab, bool selected);

	bool ApplyListViewColorsForAllTabs(COLORREF backgroundColor, COLORREF textColor);
	bool ApplyListViewColorsForTab(const Tab &tab, COLORREF backgroundColor, COLORREF textColor);

	CoreInterface *m_coreInterface;
	TabContainer *m_tabContainer;

	std::vector<boost::signals2::scoped_connection> m_connections;

	bool m_customListViewColorsApplied;
	COLORREF m_listViewBackgroundColor;
	COLORREF m_listViewTextColor;
};
