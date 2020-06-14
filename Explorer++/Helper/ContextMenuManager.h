// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellHelper.h"
#include "StatusBar.h"
#include <ShObjIdl.h>
#include <list>

class ContextMenuManager
{
public:
	enum class ContextMenuType
	{
		Background,
		DragAndDrop
	};

	/* Loads the context menu handlers bound to
	a specific registry key. */
	ContextMenuManager(ContextMenuType contextMenuType, PCIDLIST_ABSOLUTE pidlDirectory,
		IDataObject *pDataObject, IUnknown *pUnkSite,
		const std::vector<std::wstring> &blacklistedCLSIDEntries);

	/* Releases the DLL's as well as the IUnknown
	interfaces. */
	~ContextMenuManager();

	/* This will show the specified menu. Note that before
	the menu is shown, this method will insert any loaded
	shell extensions at the specified position. */
	bool ShowMenu(HWND hwnd, HMENU hMenu, UINT uIDPrevious, UINT uMinID, UINT uMaxID,
		const POINT &pt, StatusBar &statusBar);

private:
	enum class ItemType
	{
		Folder,
		Directory,
		File
	};

	struct MenuHandler_t
	{
		/* Note that only ONE of these
		should be used at any one time. */
		IContextMenu3 *pContextMenu3;
		IContextMenu2 *pContextMenu2;
		IContextMenu *pContextMenu;

		/* May be used to access the above in an
		independent way. */
		IContextMenu *pContextMenuActual;

		UINT uStartID;
		UINT uEndID;
	};

	static const int CONTEXT_MENU_SUBCLASS_ID = 1;

	/* Context menu handler registry entries. */
	static const TCHAR CMH_DIRECTORY_BACKGROUND[];
	static const TCHAR CMH_DIRECTORY_DRAG_AND_DROP[];
	static const TCHAR CMH_FOLDER_DRAG_AND_DROP[];

	static LRESULT CALLBACK ContextMenuHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	void AddMenuEntries(HMENU hMenu, UINT uIDPrevious, int iMinID, int iMaxID);
	HRESULT HandleMenuMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lRes);
	HRESULT GetMenuHelperText(UINT uID, TCHAR *szText, UINT cchMax);
	void InvokeMenuEntry(HWND hwnd, UINT uCmd);

	static int GetMenuItemPos(HMENU hMenu, UINT uID);
	static void RemoveDuplicateSeperators(HMENU hMenu);

	static ItemType GetItemType(PCIDLIST_ABSOLUTE pidl);

	std::list<ContextMenuHandler_t> m_ContextMenuHandlers;
	std::list<MenuHandler_t> m_MenuHandlers;

	UINT m_uMinID;
	UINT m_uMaxID;
	StatusBar *m_pStatusBar;
};