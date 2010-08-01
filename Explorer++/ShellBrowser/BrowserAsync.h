#ifndef BROWSER_ASYNC_INCLUDED
#define BROWSER_ASYNC_INCLUDED
#pragma once

#include "BackgroundEnumerator.h"

__interface IBrowserCallback
{
	virtual	void BrowserFinished(std::list<LPITEMIDLIST> ItemList);
};

class BrowserAsync : public IEnumerateCallback
{
public:

	BrowserAsync(IBrowserCallback *pBrowserCallback);
	~BrowserAsync();

	HRESULT	BrowseFolder(LPITEMIDLIST pidlDirectory);

private:

	void	BackgroundEnumerationFinished(std::list<LPITEMIDLIST> ItemList);

	BackgroundEnumerator	*m_pbe;
	IBrowserCallback		*m_pBrowserCallback;
};

#endif