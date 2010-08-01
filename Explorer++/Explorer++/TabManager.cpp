#include "stdafx.h"
#include <list>
#include "TabManager.h"

/* Each tab has:
 - A navigation object (this contains the asynchronous browser object).
 - A tab contents object. This object is responsible for
   displaying the results of the enumeration (i.e. inserting
   the items into the listview).
*/

/*void BrowseFolder()
{
	m_pNavigationManager->LoadFolder();
	m_pTabContents->Display();
}*/

TabManager::TabManager(HWND hTabCtrl)
{
	m_hTabCtrl = hTabCtrl;
}

TabManager::~TabManager()
{

}

/* Adds a new tab, and creates the internal
structures needed to manage it. */
void TabManager::AddTab(void)
{
	TCITEM tcItem;

	tcItem.mask			= TCIF_TEXT|TCIF_PARAM;
	tcItem.pszText		= _T("Test");
	tcItem.lParam		= 0;

	SendMessage(m_hTabCtrl,TCM_INSERTITEM,(WPARAM)0,(LPARAM)&tcItem);

	Tab *pTab = new Tab();

	m_TabList.push_back(pTab);
}

Tab *TabManager::GetCurrentTab(void)
{
	if(m_TabList.size() > 0)
	{
		return *m_TabList.begin();
	}

	return NULL;
}