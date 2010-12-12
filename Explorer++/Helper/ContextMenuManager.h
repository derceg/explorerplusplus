#ifndef CONTEXTMENUMANAGER_INCLUDED
#define CONTEXTMENUMANAGER_INCLUDED

#include <list>
#include <ShObjIdl.h>
#include "ShellHelper.h"

enum ContextMenuTypes
{
	CMT_DIRECTORY_BACKGROUND_HANDLERS,
	CMT_DRAGDROP_HANDLERS
};

class CContextMenuManager
{
public:

	/* Loads the context menu handlers bound to
	a specific registry key. */
	CContextMenuManager(ContextMenuTypes ContextMenuType,LPCITEMIDLIST pidlDirectory,IDataObject *pDataObject,IUnknown *pUnkSite);

	/* Releases the DLL's as well as the IUnknown
	interfaces. */
	~CContextMenuManager();

	/* Adds the menu entries for each of the
	context menu handlers that was initialized. */
	void	AddMenuEntries(HMENU hMenu,int iStartPos,int iMinID,int iMaxID);

	/* Handles a menu entry that was added by
	one of the context menu handlers. */
	void	HandleMenuEntry(HWND hwnd,int iCmd);

private:

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

		int				iStartID;
		int				iEndID;
	};

	list<ContextMenuHandler_t>	m_ContextMenuHandlers;
	list<MenuHandler_t>			m_MenuHandlers;
};

#endif