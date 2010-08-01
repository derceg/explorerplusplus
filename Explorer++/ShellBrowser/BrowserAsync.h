#ifndef BROWSER_ASYNC_INCLUDED
#define BROWSER_ASYNC_INCLUDED
#pragma once

#include "BackgroundEnumerator.h"

class BrowserAsync : public IEnumerateCallback
{
public:

	BrowserAsync();
	~BrowserAsync();

	HRESULT	BrowseFolder(LPITEMIDLIST pidlDirectory);

private:

	void	BackgroundEnumerationFinished(void);

	BackgroundEnumerator	*m_pbe;
};

#endif