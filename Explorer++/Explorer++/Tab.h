// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/iShellView.h"

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

	CShellBrowser *GetShellBrowser() const;
	void SetShellBrowser(CShellBrowser *shellBrowser);
	bool GetLocked() const;
	void SetLocked(bool locked);
	bool GetAddressLocked() const;
	void SetAddressLocked(bool addressLocked);

	const int	id;
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

	CShellBrowser *m_shellBrowser;
	bool m_locked;
	bool m_addressLocked;
};