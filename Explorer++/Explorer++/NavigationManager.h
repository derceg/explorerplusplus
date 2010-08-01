#ifndef NAVIGATION_MANAGER_INCLUDED
#define NAVIGATION_MANAGER_INCLUDED
#pragma once

#include "NavigationHistory.h"
#include "../ShellBrowser/BrowserAsync.h"

enum NotificationEvent
{
	NOTIFICATION_EVENT_BROWSING_STARTED,
	NOTIFICATION_EVENT_BROWSING_FINISHED
};

enum NavigationStatus
{
	/* Navigation object is been initialized. */
	NAVIGATION_STATUS_INITIALIZING,

	/* Currently not within any folder. May
	indicate a pending transition. */
	NAVIGATION_STATUS_PENDING,

	/* Within a folder. Completely finished
	loading. */
	NAVIGATION_STATUS_COMPLETE
};

__interface IDisplayManager
{
	void	BrowsingStartedCallback(void);
	void	BrowsingFinishedCallback(std::list<LPITEMIDLIST> *pItemList);
};

/* Handles navigation.
The NavigationManager object sends out callback
notifications on the following events:
 - When browsing has started.
 - When browsing has finished.
*/
class NavigationManager : public IBrowserCallback
{
public:

	NavigationManager();
	~NavigationManager();

	/* Navigation. */
	HRESULT	BrowseFolder(LPITEMIDLIST pidlDirectory);
	void	Back(void);
	void	Forward(void);

	/* Callbacks. */
	void	AddCallback(IDisplayManager *pDisplayManager);

private:

	void	BrowserFinished(std::list<LPITEMIDLIST> ItemList);
	void	NotifyListeners(NotificationEvent Type,LPVOID pData);

	/* TODO: This should be an interface, so that
	various types of browsers can be used (e.g.
	synchronous, asynchronous, etc). */
	BrowserAsync		*m_pBrowserAsync;

	std::list<IDisplayManager *>	m_DisplayManagerList;

	NavigationHistory	*m_pNavigationHistory;
	NavigationStatus	m_NavigationStatus;
};

#endif