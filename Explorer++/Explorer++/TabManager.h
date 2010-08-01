#ifndef TAB_MANAGER_INCLUDED
#define TAB_MANAGER_INCLUDED
#pragma once

#include <list>
#include "NavigationManager.h"

using namespace std;

class TabDisplayManager : public ITabDisplayManager
{
public:

	TabDisplayManager(HWND hListview);
	~TabDisplayManager();

private:

	void	ClearDisplay(void);
	void	DisplayResults(std::list<LPITEMIDLIST> ItemList);

	HWND	m_hListview;
};

class Tab
{
public:

	Tab(HWND hListview);
	~Tab();

	NavigationManager	*GetNavigationManager(void);

private:

	NavigationManager	*m_pNavigationManager;
	TabDisplayManager	*m_pTabDisplayManager;
};

/* Manages all tabs. */
class TabManager
{
public:

	TabManager(HWND hTabCtrl);
	~TabManager();

	void	AddTab(HWND hListview);

	void	GetTab(void);
	Tab		*GetCurrentTab(void);

private:

	HWND			m_hTabCtrl;

	std::list<Tab *>	m_TabList;
};

#endif