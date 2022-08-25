// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarView.h"
#include "../Helper/FileContextMenuManager.h"

class CoreInterface;

class DrivesToolbarView : public ToolbarView, private FileContextMenuHandler
{
public:
	static DrivesToolbarView *Create(HWND parent, CoreInterface *coreInterface, HINSTANCE instance);

	void ShowContextMenu(const std::wstring &drivePath, const POINT &ptClient, bool showExtended);

private:
	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_IN_NEW_TAB = (MAX_SHELL_MENU_ID + 1);

	DrivesToolbarView(HWND parent, CoreInterface *coreInterface, HINSTANCE instance);
	~DrivesToolbarView() = default;

	// FileContextMenuHandler
	void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, IContextMenu *contextMenu,
		HMENU hMenu) override;
	BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, int iCmd) override;

	CoreInterface *m_coreInterface;
	HINSTANCE m_instance;
};
