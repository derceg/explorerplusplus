#pragma once

#include "Explorer++_internal.h"
#include "../ShellBrowser/iShellView.h"
#include <unordered_map>

class CTabContainer
{
public:

	CTabContainer(HWND hTabCtrl, std::unordered_map<int, CShellBrowser *> *pShellBrowsers, IExplorerplusplus *pexpp);
	~CTabContainer();

	void			InsertTab();
	void			RemoveTab();

	int				GetSelection();
	void			SetSelection(int Index);

	CShellBrowser	*GetBrowserForTab(int Index);

private:

	HWND				m_hTabCtrl;

	std::unordered_map<int, CShellBrowser *>	*m_pShellBrowsers;
	IExplorerplusplus	*m_pexpp;
};