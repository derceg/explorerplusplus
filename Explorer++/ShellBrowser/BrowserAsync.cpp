#include "stdafx.h"
#include "BrowserAsync.h"
#include "BackgroundEnumerator.h"

BrowserAsync::BrowserAsync()
{
	m_pbe = new BackgroundEnumerator(this);
}

BrowserAsync::~BrowserAsync()
{

}

HRESULT BrowserAsync::BrowseFolder(LPITEMIDLIST pidlDirectory)
{
	HRESULT hr;

	SetCursor(LoadCursor(NULL,IDC_APPSTARTING));

	hr = m_pbe->EnumerateDirectory(pidlDirectory);

	return hr;
}

void BrowserAsync::BackgroundEnumerationFinished(void)
{

}