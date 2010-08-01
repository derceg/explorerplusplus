#ifndef BACKGROUND_ENUMERATOR_INCLUDED
#define BACKGROUND_ENUMERATOR_INCLUDED
#pragma once

#include <list>

__interface IEnumerateCallback
{
	virtual	void BackgroundEnumerationFinished(std::list<LPITEMIDLIST> ItemList);
};

class BackgroundEnumerator
{
public:

	BackgroundEnumerator(IEnumerateCallback *pEnumerateCallback);
	~BackgroundEnumerator();

	HRESULT					EnumerateDirectory(LPITEMIDLIST pidlDirectory);
	std::list<LPITEMIDLIST>	*EnumerateDirectoryInternal(LPITEMIDLIST pidlDirectory);

	void					EnumerateDirectoryFinished(std::list<LPITEMIDLIST> *pItemList);

	void					StopEnumeration(void);

private:

	std::list<HANDLE>	m_ThreadList;
	CRITICAL_SECTION	m_csStop;
	BOOL				m_bStopEnumeration;

	IEnumerateCallback	*m_pEnumerateCallback;
};

#endif