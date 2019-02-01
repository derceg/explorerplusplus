// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/iShellView.h"
#include "Tab.h"
#include "TabInterface.h"
#include <unordered_map>

class CTabContainer
{
public:

	CTabContainer(HWND hTabCtrl, std::unordered_map<int, Tab> *tabInfo, TabInterface *ti);
	~CTabContainer();

	void			InsertTab();
	void			RemoveTab();

	int				GetSelection();
	void			SetSelection(int Index);

	CShellBrowser	*GetBrowserForTab(int Index);

private:

	HWND				m_hTabCtrl;

	std::unordered_map<int, Tab> *m_tabInfo;
	TabInterface		*m_ti;
};