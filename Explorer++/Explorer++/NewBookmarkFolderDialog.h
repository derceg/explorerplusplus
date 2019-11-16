// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Bookmark.h"

class CNewBookmarkFolderDialog;

class CNewBookmarkFolderDialogPersistentSettings : public CDialogSettings
{
public:

	static CNewBookmarkFolderDialogPersistentSettings &GetInstance();

private:

	friend CNewBookmarkFolderDialog;

	static const TCHAR SETTINGS_KEY[];

	CNewBookmarkFolderDialogPersistentSettings();

	CNewBookmarkFolderDialogPersistentSettings(const CNewBookmarkFolderDialogPersistentSettings &);
	CNewBookmarkFolderDialogPersistentSettings & operator=(const CNewBookmarkFolderDialogPersistentSettings &);
};

class CNewBookmarkFolderDialog : public CBaseDialog
{
public:

	CNewBookmarkFolderDialog(HINSTANCE hInstance,int iResource,HWND hParent);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

private:

	void	OnOk();
	void	OnCancel();

	CNewBookmarkFolderDialogPersistentSettings	*m_pnbfdps;
};