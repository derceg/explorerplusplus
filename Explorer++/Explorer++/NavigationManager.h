#ifndef NAVIGATION_MANAGER_INCLUDED
#define NAVIGATION_MANAGER_INCLUDED
#pragma once

#include "../ShellBrowser/BrowserAsync.h"

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

__interface ITabDisplayManager
{
	void	ClearDisplay(void);
	void	DisplayResults(std::list<LPITEMIDLIST> ItemList);
};

/* Handles navigation. */
class NavigationManager : public IBrowserCallback
{
public:

	NavigationManager(ITabDisplayManager *pTabDisplayManager);
	~NavigationManager();

	/* Loads the specified folder within the
	current tab. */
	HRESULT	BrowseFolder(LPITEMIDLIST pidlDirectory);
	void	Back(void);
	void	Forward(void);

private:

	void	NavigationManager::BrowserFinished(std::list<LPITEMIDLIST> ItemList);

	/* TODO: This should be an interface, so that
	various types of browsers can be used (e.g.
	synchronous, asynchronous, etc). */
	BrowserAsync		*m_pBrowserAsync;

	ITabDisplayManager	*m_pTabDisplayManager;

	NavigationStatus	m_NavigationStatus;
};

#endif