#include "stdafx.h"
#include "NavigationManager.h"
#include "../ShellBrowser/BrowserAsync.h"

NavigationManager::NavigationManager(ITabDisplayManager *pTabDisplayManager)
{
	m_NavigationStatus = NAVIGATION_STATUS_INITIALIZING;

	m_pBrowserAsync = new BrowserAsync(this);
	m_pTabDisplayManager = pTabDisplayManager;

	m_NavigationStatus = NAVIGATION_STATUS_PENDING;
}

NavigationManager::~NavigationManager()
{
	delete m_pBrowserAsync;
}

HRESULT NavigationManager::BrowseFolder(LPITEMIDLIST pidlDirectory)
{
	/* Set the proper navigation status. This may have
	to be done within a critical section. */
	m_NavigationStatus = NAVIGATION_STATUS_PENDING;

	/* Inform the display object that we're about to
	navigate into another folder. The display object
	should clear out any current display information
	in preparation (i.e. delete the current listview
	items). */
	m_pTabDisplayManager->ClearDisplay();

	return m_pBrowserAsync->BrowseFolder(pidlDirectory);
}

void NavigationManager::BrowserFinished(std::list<LPITEMIDLIST> ItemList)
{
	/* Tell the display manager to display the results of
	the enumeration. */
	m_pTabDisplayManager->DisplayResults(ItemList);

	m_NavigationStatus = NAVIGATION_STATUS_COMPLETE;
}