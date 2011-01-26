#ifndef FILECONTEXTMENUMANAGER_INCLUDED
#define FILECONTEXTMENUMANAGER_INCLUDED

#include <list>
#include "StatusBar.h"

__interface IFileContextMenuExternal
{
	/* Allows the caller to add custom entries to the
	context menu before it is shown. */
	virtual void	AddMenuEntries(LPITEMIDLIST pidlParent,std::list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,HMENU hMenu);

	/* Allows the caller to handle the processing
	of a shell menu item. For example, the 'Open'
	item may be processed internally.
	Returns TRUE if the item was processed;
	FALSE otherwise. */
	virtual BOOL	HandleShellMenuItem(LPITEMIDLIST pidlParent,std::list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,TCHAR *szCmd);

	/* Handles the processing for one of the menu
	items that was added by the caller. */
	virtual void	HandleCustomMenuItem(LPITEMIDLIST pidlParent,std::list<LPITEMIDLIST> pidlItemList,int iCmd);
};

class CFileContextMenuManager
{
public:

	CFileContextMenuManager(HWND hwnd,LPITEMIDLIST pidlParent,std::list<LPITEMIDLIST> pidlItemList);
	~CFileContextMenuManager();

	/* Shows the context menu. */
	HRESULT				ShowMenu(IFileContextMenuExternal *pfcme,int iMinID,int iMaxID,POINT *ppt,CStatusBar *pStatusBar,DWORD_PTR dwData,BOOL bRename = FALSE,BOOL bExtended = FALSE);

	LRESULT CALLBACK	ShellMenuHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,DWORD_PTR dwRefData);

private:

	static const int	CONTEXT_MENU_SUBCLASS_ID = 1;

	IContextMenu3		*m_pShellContext3;
	IContextMenu2		*m_pShellContext2;
	IContextMenu		*m_pShellContext;
	IContextMenu		*m_pActualContext;

	HWND				m_hwnd;
	int					m_iMinID;
	int					m_iMaxID;

	CStatusBar			*m_pStatusBar;

	LPITEMIDLIST		m_pidlParent;
	std::list<LPITEMIDLIST>	m_pidlItemList;
};

#endif