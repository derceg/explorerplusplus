#ifndef NAVIGATION_HISTORY_INCLUDED
#define NAVIGATION_HISTORY_INCLUDED
#pragma once

#include <list>

struct HistoryInformation
{
	/* The directory that was naviagted to. */
	LPITEMIDLIST	pidlDirectory;

	/* The date/time of the navigation. */
	SYSTEMTIME		st;
};

class NavigationHistory
{
public:

	void	AddHistoryItem(void);

private:

	std::list<HistoryInformation>	m_HistoryList;
};

#endif