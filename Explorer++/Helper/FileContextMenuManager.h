// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"
#include "ShellHelper.h"
#include "StatusBar.h"
#include <wil/com.h>
#include <optional>

class FileContextMenuHandler
{
public:
	virtual ~FileContextMenuHandler() = default;

	// Allows the caller to add/update items on the context menu before it's shown.
	virtual void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, IContextMenu *contextMenu, HMENU hMenu) = 0;

	// Allows the caller to handle the processing of a shell menu item. For example, the 'Open' item
	// may be processed internally.
	// Returns TRUE if the item was processed; FALSE otherwise.
	virtual BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, const TCHAR *szCmd) = 0;

	// Handles the processing for one of the menu items that was added by the caller.
	virtual void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, int iCmd) = 0;
};

class FileContextMenuManager
{
public:
	FileContextMenuManager(HWND hwnd, PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PCITEMID_CHILD> &pidlItems);
	~FileContextMenuManager();

	HRESULT ShowMenu(FileContextMenuHandler *handler, int iMinID, int iMaxID, const POINT *ppt,
		StatusBar *pStatusBar, IUnknown *site, BOOL bRename = FALSE, BOOL bExtended = FALSE);

	LRESULT CALLBACK ShellMenuHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	std::optional<std::string> GetFilesystemDirectory();

	DISALLOW_COPY_AND_ASSIGN(FileContextMenuManager);

	static const int CONTEXT_MENU_SUBCLASS_ID = 1;

	wil::com_ptr_nothrow<IContextMenu3> m_pShellContext3;
	wil::com_ptr_nothrow<IContextMenu2> m_pShellContext2;
	wil::com_ptr_nothrow<IContextMenu> m_pShellContext;
	IContextMenu *m_pActualContext;

	const HWND m_hwnd;
	int m_iMinID;
	int m_iMaxID;

	StatusBar *m_pStatusBar;

	const unique_pidl_absolute m_pidlParent;
	std::vector<PITEMID_CHILD> m_pidlItems;
};
