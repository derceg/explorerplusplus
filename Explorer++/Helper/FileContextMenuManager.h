// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"
#include "ShellHelper.h"
#include "StatusBar.h"
#include <wil/com.h>
#include <list>

__interface IFileContextMenuExternal
{
	/* Allows the caller to add custom entries to the
	context menu before it is shown. */
	void AddMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems,
		DWORD_PTR dwData, HMENU hMenu);

	/* Allows the caller to handle the processing
	of a shell menu item. For example, the 'Open'
	item may be processed internally.
	Returns TRUE if the item was processed;
	FALSE otherwise. */
	BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd);

	/* Handles the processing for one of the menu
	items that was added by the caller. */
	void HandleCustomMenuItem(
		PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, int iCmd);
};

class FileContextMenuManager
{
public:
	FileContextMenuManager(
		HWND hwnd, PCIDLIST_ABSOLUTE pidlParent, const std::vector<PCITEMID_CHILD> &pidlItems);
	~FileContextMenuManager();

	/* Shows the context menu. */
	HRESULT ShowMenu(IFileContextMenuExternal *pfcme, int iMinID, int iMaxID, const POINT *ppt,
		StatusBar *pStatusBar, DWORD_PTR dwData, BOOL bRename = FALSE, BOOL bExtended = FALSE);

	LRESULT CALLBACK ShellMenuHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	DISALLOW_COPY_AND_ASSIGN(FileContextMenuManager);

	static const int CONTEXT_MENU_SUBCLASS_ID = 1;

	wil::com_ptr<IContextMenu3> m_pShellContext3;
	wil::com_ptr<IContextMenu2> m_pShellContext2;
	wil::com_ptr<IContextMenu> m_pShellContext;
	IContextMenu *m_pActualContext;

	const HWND m_hwnd;
	int m_iMinID;
	int m_iMaxID;

	StatusBar *m_pStatusBar;

	const unique_pidl_absolute m_pidlParent;
	std::vector<PITEMID_CHILD> m_pidlItems;
};