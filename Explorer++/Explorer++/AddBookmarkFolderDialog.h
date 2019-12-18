// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"

class AddBookmarkFolderDialog;

class AddBookmarkFolderDialogPersistentSettings : public CDialogSettings
{
public:

	static AddBookmarkFolderDialogPersistentSettings &GetInstance();

private:

	friend AddBookmarkFolderDialog;

	static const TCHAR SETTINGS_KEY[];

	AddBookmarkFolderDialogPersistentSettings();

	AddBookmarkFolderDialogPersistentSettings(const AddBookmarkFolderDialogPersistentSettings &);
	AddBookmarkFolderDialogPersistentSettings & operator=(const AddBookmarkFolderDialogPersistentSettings &);
};

class AddBookmarkFolderDialog : public CBaseDialog
{
public:

	AddBookmarkFolderDialog(HINSTANCE hInstance,int iResource,HWND hParent);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

private:

	void	OnOk();
	void	OnCancel();

	AddBookmarkFolderDialogPersistentSettings	*m_pnbfdps;
};