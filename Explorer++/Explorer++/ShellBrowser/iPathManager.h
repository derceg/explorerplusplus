// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>

class CPathManager
{
public:
	CPathManager();
	~CPathManager();

	int				GetNumBackPathsStored(void) const;
	int				GetNumForwardPathsStored(void) const;
	UINT			CreateHistoryPopupMenu(HWND,POINT *,BOOL);
	std::list<LPITEMIDLIST>	GetBackHistory() const;
	std::list<LPITEMIDLIST>	GetForwardHistory() const;

	void			StoreIdl(LPITEMIDLIST pidl);
	LPITEMIDLIST	RetrievePath(int iIndex);
	LPITEMIDLIST	RetrievePathWithoutUpdate(int iIndex);
	LPITEMIDLIST	RetrieveAndValidateIdl(int iIndex);

private:
	#define DEFAULT_ALLOCATION	10

	/* Points one past the "current" path. */
	int m_iCurrent;

	/* Points one past the end of the valid
	paths. */
	int m_nTotal;

	/* Number of paths allocated.  */
	int m_nAllocated;

	void ShiftIdlArray(int iStart);

	LPITEMIDLIST *ppidlList;
};