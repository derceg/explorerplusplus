#include "stdafx.h"
#include "NavigationManager.h"
#include "../ShellBrowser/BrowserAsync.h"

NavigationManager::NavigationManager()
{
	m_NavigationStatus = NAVIGATION_STATUS_INITIALIZING;

	m_pBrowserAsync = new BrowserAsync(this);
	m_pNavigationHistory = new NavigationHistory();

	m_NavigationStatus = NAVIGATION_STATUS_PENDING;
}

NavigationManager::~NavigationManager()
{
	delete m_pBrowserAsync;
	delete m_pNavigationHistory;
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
	NotifyListeners(NOTIFICATION_EVENT_BROWSING_STARTED,NULL);

	return m_pBrowserAsync->BrowseFolder(pidlDirectory);
}

void NavigationManager::BrowserFinished(std::list<LPITEMIDLIST> ItemList)
{
	/* Notify each of the registered listeners (via the
	specified callback handlers) that browsing has
	finished. */
	NotifyListeners(NOTIFICATION_EVENT_BROWSING_FINISHED,(LPVOID)&ItemList);

	/* Store this path in history. */

	/* Change navigation status. */
	m_NavigationStatus = NAVIGATION_STATUS_COMPLETE;
}

void NavigationManager::NotifyListeners(NotificationEvent Type,LPVOID pData)
{
	for each(auto pDisplayManager in m_DisplayManagerList)
	{
		switch(Type)
		{
		case NOTIFICATION_EVENT_BROWSING_STARTED:
			pDisplayManager->BrowsingStartedCallback();
			break;

		case NOTIFICATION_EVENT_BROWSING_FINISHED:
			pDisplayManager->BrowsingFinishedCallback(static_cast<std::list<LPITEMIDLIST> *>(pData));
			break;
		}
	}
}

/* Adds a new callback handler. */
void NavigationManager::AddCallback(IDisplayManager *pDisplayManager)
{
	m_DisplayManagerList.push_back(pDisplayManager);
}