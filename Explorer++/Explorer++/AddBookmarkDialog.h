// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "BookmarkItem.h"
#include "BookmarkTree.h"
#include "BookmarkTreeView.h"
#include "CoreInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <wil/resource.h>
#include <unordered_set>

class AddBookmarkDialog;

class CAddBookmarkDialogPersistentSettings : public CDialogSettings
{
public:

	static CAddBookmarkDialogPersistentSettings &GetInstance();

private:

	friend AddBookmarkDialog;

	static const TCHAR SETTINGS_KEY[];

	CAddBookmarkDialogPersistentSettings();

	CAddBookmarkDialogPersistentSettings(const CAddBookmarkDialogPersistentSettings &);
	CAddBookmarkDialogPersistentSettings & operator=(const CAddBookmarkDialogPersistentSettings &);

	bool m_bInitialized;
	std::wstring m_guidSelected;
	std::unordered_set<std::wstring> m_setExpansion;
};

class AddBookmarkDialog : public CBaseDialog
{
public:

	AddBookmarkDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
		BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem,
		BookmarkItem **selectedParentFolder);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCtlColorEdit(HWND hwnd,HDC hdc);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();
	INT_PTR	OnNcDestroy();

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	static const COLORREF ERROR_BACKGROUND_COLOR = RGB(255,188,188);

	AddBookmarkDialog & operator = (const AddBookmarkDialog &abd);

	void UpdateDialogForBookmarkFolder();

	void GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void SaveState();

	void OnOk();
	void OnCancel();

	void SaveTreeViewState();
	void SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem);

	IExplorerplusplus *m_expp;

	BookmarkTree *m_bookmarkTree;
	BookmarkItem *m_bookmarkItem;
	BookmarkItem **m_selectedParentFolder;

	BookmarkTreeView *m_pBookmarkTreeView;

	wil::unique_hbrush m_ErrorBrush;

	CAddBookmarkDialogPersistentSettings *m_pabdps;
};