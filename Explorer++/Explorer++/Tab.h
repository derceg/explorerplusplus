// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/iShellView.h"
#include "../Helper/Macros.h"

// Used when creating a tab.
struct TabSettings
{
	BOOL	bLocked;
	BOOL	bAddressLocked;
	BOOL	bUseCustomName;
	TCHAR	szName[MAX_PATH];
};

class Tab
{
public:

	Tab(int id);

	int GetId() const;
	CShellBrowser *GetShellBrowser() const;
	void SetShellBrowser(CShellBrowser *shellBrowser);
	bool GetLocked() const;
	void SetLocked(bool locked);
	bool GetAddressLocked() const;
	void SetAddressLocked(bool addressLocked);

	HWND	listView;
	BOOL	bUseCustomName;
	TCHAR	szName[MAX_PATH];

	/* Although each tab manages its
	own columns, it does not know
	about any column defaults.
	Therefore, it makes more sense
	for this setting to remain here. */
	//BOOL	bUsingDefaultColumns;

private:

	DISALLOW_COPY_AND_ASSIGN(Tab);

	const int m_id;
	CShellBrowser *m_shellBrowser;
	bool m_locked;
	bool m_addressLocked;
};