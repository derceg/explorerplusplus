#ifndef PATHMANAGER_INCLUDED
#define PATHMANAGER_INCLUDED

#include <list>

class CPathManager
{
public:
	CPathManager();
	~CPathManager();

	int				GetNumBackPathsStored(void);
	int				GetNumForwardPathsStored(void);
	UINT			CreateHistoryPopupMenu(HWND,POINT *,BOOL);
	void			GetBackHistory(std::list<LPITEMIDLIST> *lHistory);
	void			GetForwardHistory(std::list<LPITEMIDLIST> *lHistory);

	void			StoreIdl(LPITEMIDLIST pidl);
	LPITEMIDLIST	RetrievePath(int iIndex);
	LPITEMIDLIST	RetrievePathWithoutUpdate(int iIndex);
	LPITEMIDLIST	RetrieveAndValidateIdl(int iIndex);

private:
	int m_iRefCount;

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

#endif