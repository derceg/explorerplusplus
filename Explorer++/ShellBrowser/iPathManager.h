#ifndef PATHMANAGER_INCLUDED
#define PATHMANAGER_INCLUDED

#include <list>

__interface IPathManager : IUnknown
{
	virtual int		GetNumBackPathsStored(void);
	virtual int		GetNumForwardPathsStored(void);
	virtual UINT	CreateHistoryPopupMenu(HWND,POINT *,BOOL);
	virtual void	GetBackHistory(std::list<LPITEMIDLIST> *lHistory);
	virtual void	GetForwardHistory(std::list<LPITEMIDLIST> *lHistory);

	virtual void			StoreIdl(LPITEMIDLIST pidl);
	virtual LPITEMIDLIST	RetrievePath(int iIndex);
	virtual LPITEMIDLIST	RetrievePathWithoutUpdate(int iIndex);
	virtual LPITEMIDLIST	RetrieveAndValidateIdl(int iIndex);
};

class CPathManager : public IPathManager
{
public:
	CPathManager();
	~CPathManager();

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall	AddRef(void);
	ULONG __stdcall	Release(void);

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

	#define MAX_STORED_PATHS	10
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