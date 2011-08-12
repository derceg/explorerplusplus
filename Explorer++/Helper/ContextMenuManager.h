#ifndef CONTEXTMENUMANAGER_INCLUDED
#define CONTEXTMENUMANAGER_INCLUDED

#include <list>
#include <ShObjIdl.h>
#include "ShellHelper.h"
#include "StatusBar.h"

class CContextMenuManager
{
	friend LRESULT CALLBACK ContextMenuHookProc(HWND hwnd,UINT Msg,WPARAM wParam,
	LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	enum ContextMenuType_t
	{
		CONTEXT_MENU_TYPE_BACKGROUND,
		CONTEXT_MENU_TYPE_DRAG_AND_DROP
	};

	/* Loads the context menu handlers bound to
	a specific registry key. */
	CContextMenuManager(ContextMenuType_t ContextMenuType,LPCITEMIDLIST pidlDirectory,IDataObject *pDataObject,IUnknown *pUnkSite);

	/* Releases the DLL's as well as the IUnknown
	interfaces. */
	~CContextMenuManager();

	/* This will show the specified menu. Note that before
	the menu is shown, this method will insert any loaded
	shell extensions at the specified position. */
	bool	ShowMenu(HWND hwnd,HMENU hMenu,UINT uIDPrevious,UINT uMinID,UINT uMaxID,const POINT &pt,CStatusBar &StatusBar);

private:

	enum ItemType_t
	{
		ITEM_TYPE_FOLDER,
		ITEM_TYPE_DIRECTORY,
		ITEM_TYPE_FILE
	};

	struct MenuHandler_t
	{
		/* Note that only ONE of these
		should be used at any one time. */
		IContextMenu3	*pContextMenu3;
		IContextMenu2	*pContextMenu2;
		IContextMenu	*pContextMenu;

		/* May be used to access the above in an
		independent way. */
		IContextMenu	*pContextMenuActual;

		UINT			uStartID;
		UINT			uEndID;
	};

	static const int	CONTEXT_MENU_SUBCLASS_ID = 1;

	/* Context menu handler registry entries. */
	static const TCHAR CMH_DIRECTORY_BACKGROUND[];
	static const TCHAR CMH_DIRECTORY_DRAG_AND_DROP[];
	static const TCHAR CMH_FOLDER_DRAG_AND_DROP[];

	void	AddMenuEntries(HMENU hMenu,UINT uIDPrevious,int iMinID,int iMaxID);
	HRESULT	HandleMenuMessage(UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT &lRes);
	HRESULT	GetMenuHelperText(UINT uID,TCHAR *szText,UINT cchMax);
	void	InvokeMenuEntry(HWND hwnd,UINT uCmd);

	int		GetMenuItemPos(HMENU hMenu,UINT uID);
	void	RemoveDuplicateSeperators(HMENU hMenu);

	ItemType_t	GetItemType(LPCITEMIDLIST pidl);

	std::list<ContextMenuHandler_t>	m_ContextMenuHandlers;
	std::list<MenuHandler_t>		m_MenuHandlers;

	UINT							m_uMinID;
	UINT							m_uMaxID;
	CStatusBar						*m_pStatusBar;
};

#endif