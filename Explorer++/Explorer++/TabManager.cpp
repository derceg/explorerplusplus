#include "stdafx.h"
#include <list>
#include "TabManager.h"
#include "../Helper/Helper.h"

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
void TabManager::AddTab(HWND hListview)
{
	/*TCITEM tcItem;

	tcItem.mask			= TCIF_TEXT|TCIF_PARAM;
	tcItem.pszText		= _T("Test");
	tcItem.lParam		= 0;

	SendMessage(m_hTabCtrl,TCM_INSERTITEM,(WPARAM)0,(LPARAM)&tcItem);*/

	Tab *pTab = new Tab(hListview);

	m_TabList.push_back(pTab);
}

Tab *TabManager::GetCurrentTab(void)
{
	if(m_TabList.size() > 0)
	{
		return static_cast<Tab *>(*m_TabList.begin());
	}

	return NULL;
}

Tab::Tab(HWND hListview)
{
	/* Create the navigation manager. */
	m_pNavigationManager = new NavigationManager();

	/* Create the display manager. */
	m_pTabDisplayManager = new TabDisplayManager(hListview);

	m_pNavigationManager->AddCallback(m_pTabDisplayManager);

	/* TODO: Add item cache callback. Both the Tab class and
	display class should be able to access this cache. */
}

Tab::~Tab()
{
	delete m_pNavigationManager;
}

NavigationManager *Tab::GetNavigationManager(void)
{
	return m_pNavigationManager;
}

TabDisplayManager::TabDisplayManager(HWND hListview)
{
	m_hListview = hListview;
}

void TabDisplayManager::BrowsingStartedCallback(void)
{
	ListView_DeleteAllItems(m_hListview);
}

/* TODO: This will have to be moved onto the main UI thread. */
void TabDisplayManager::BrowsingFinishedCallback(std::list<LPITEMIDLIST> *pItemList)
{
	LVITEM lvItem;
	LPITEMIDLIST pidlDirectory;

	GetIdlFromParsingName(_T("K:\\"),&pidlDirectory);

	LPITEMIDLIST pidlComplete;
	TCHAR szDisplayName[MAX_PATH];
	int iItem = 0;

	SendMessage(m_hListview,WM_SETREDRAW,(WPARAM)FALSE,(LPARAM)NULL);

	for each(auto pidl in *pItemList)
	{
		pidlComplete = ILCombine(pidlDirectory,pidl);

		GetDisplayName(pidlComplete,szDisplayName,SHGDN_INFOLDER);

		lvItem.mask		= LVIF_TEXT|LVIF_PARAM;
		lvItem.iItem	= iItem++;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szDisplayName;
		lvItem.lParam	= 0;
		ListView_InsertItem(m_hListview,&lvItem);

		CoTaskMemFree(pidlComplete);
	}

	SendMessage(m_hListview,WM_SETREDRAW,(WPARAM)TRUE,(LPARAM)NULL);
}