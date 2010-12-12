#ifndef FILECONTEXTMENUMANAGER_INCLUDED
#define FILECONTEXTMENUMANAGER_INCLUDED

interface IFileContextMenuExternal
{
	/* Allows the caller to add custom entries to the
	context menu before it is shown. */
	virtual void	AddMenuEntries(DWORD_PTR dwData,HMENU hMenu);

	/* Allows the caller to handle the processing
	of a shell menu item. For example, the 'Open'
	item may be processed internally.
	Returns TRUE if the item was processed;
	FALSE otherwise. */
	virtual BOOL	HandleShellMenuItem();

	/* Handles the processing for one of the menu
	items that was added by the caller. */
	virtual void	HandleCustomMenuItem();
};

class CFileContextMenuManager
{
public:

	CFileContextMenuManager(HWND hwnd,LPITEMIDLIST pidlParent,
		LPCITEMIDLIST *ppidl,int nFiles);
	~CFileContextMenuManager();

	/* Shows the context menu. */
	HRESULT				ShowMenu(IFileContextMenuExternal *pfcme,int iMinID,int iMaxID,POINT *ppt,BOOL bRename = FALSE,BOOL bExtended = FALSE);

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
};

#endif