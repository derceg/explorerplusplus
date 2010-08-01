#include "stdafx.h"
#include "BrowserAsync.h"
#include "BackgroundEnumerator.h"

BrowserAsync::BrowserAsync(IBrowserCallback *pBrowserCallback)
{
	m_pbe = new BackgroundEnumerator(this);

	m_pBrowserCallback = pBrowserCallback;
}

BrowserAsync::~BrowserAsync()
{

}

HRESULT BrowserAsync::BrowseFolder(LPITEMIDLIST pidlDirectory)
{
	HRESULT hr;

	SetCursor(LoadCursor(NULL,IDC_APPSTARTING));

	/* Clear the item cache. */

	hr = m_pbe->EnumerateDirectory(pidlDirectory);

	return hr;
}

void BrowserAsync::BackgroundEnumerationFinished(std::list<LPITEMIDLIST> ItemList)
{
	for each(auto pidl in ItemList)
	{
		/* Cache item information. When we browse
		to another folder, this cached information will
		be cleared.
		The following information will be stored:
		 - The items relative idl.
		 - The items display name.
		*/
	}

	m_pBrowserCallback->BrowserFinished(ItemList);
}