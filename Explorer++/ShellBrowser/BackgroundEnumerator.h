#ifndef BACKGROUND_ENUMERATOR_INCLUDED
#define BACKGROUND_ENUMERATOR_INCLUDED
#pragma once

#include <list>

__interface IEnumerateCallback
{
	virtual	void BackgroundEnumerationFinished(void);
};

class BackgroundEnumerator
{
public:

	BackgroundEnumerator(IEnumerateCallback *pEnumerateCallback);
	~BackgroundEnumerator();

	HRESULT	EnumerateDirectory(LPITEMIDLIST pidlDirectory);
	void	EnumerateDirectoryInternal(void);
	void	EnumerateDirectoryFinished(void);

	void	StopEnumeration(void);

private:

	CRITICAL_SECTION	m_csStop;
	std::list<LPITEMIDLIST>	m_ItemList;
	HANDLE				m_hThread;
	BOOL				m_bStopEnumeration;

	IEnumerateCallback	*m_pEnumerateCallback;
	LPITEMIDLIST		m_pidlDirectory;
};

#endif