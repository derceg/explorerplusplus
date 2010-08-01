#ifndef TAB_MANAGER_INCLUDED
#define TAB_MANAGER_INCLUDED
#pragma once

#include <list>

using namespace std;

class Tab
{
public:

	void	GetNavigation();

private:

	HWND				hListview;
	//NavigationManager	m_NavigationManager;
};

/* Manages all tabs. */
class TabManager
{
public:

	TabManager(HWND hTabCtrl);
	~TabManager();

	void	AddTab(void);

	void	GetTab(void);
	Tab		*GetCurrentTab(void);

private:

	HWND			m_hTabCtrl;

	std::list<Tab *>	m_TabList;
};

#endif