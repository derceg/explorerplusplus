// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DarkModeDialogBase.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <wil/resource.h>
#include <optional>
#include <unordered_set>

class AddBookmarkDialog;
class BookmarkItem;
class BookmarkTree;
class BookmarkTreeView;
__interface IExplorerplusplus;

class AddBookmarkDialogPersistentSettings : public DialogSettings
{
public:
	static AddBookmarkDialogPersistentSettings &GetInstance();

private:
	friend AddBookmarkDialog;

	static const TCHAR SETTINGS_KEY[];

	AddBookmarkDialogPersistentSettings();

	AddBookmarkDialogPersistentSettings(const AddBookmarkDialogPersistentSettings &);
	AddBookmarkDialogPersistentSettings &operator=(const AddBookmarkDialogPersistentSettings &);

	bool m_bInitialized;
	std::wstring m_guidSelected;
	std::unordered_set<std::wstring> m_setExpansion;
};

class AddBookmarkDialog : public DarkModeDialogBase
{
public:
	AddBookmarkDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
		BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem,
		BookmarkItem *defaultParentSelection, BookmarkItem **selectedParentFolder,
		std::optional<std::wstring> customDialogTitle = std::nullopt);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCtlColorEditExtra(HWND hwnd, HDC hdc) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;
	INT_PTR OnNcDestroy() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	static const COLORREF ERROR_BACKGROUND_COLOR = RGB(255, 188, 188);

	AddBookmarkDialog &operator=(const AddBookmarkDialog &abd);

	void UpdateDialogForBookmarkFolder();
	void SetDialogTitle();
	std::wstring LoadDialogTitle();

	void GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
		std::list<ResizableDialog::Control> &ControlList) override;
	void SaveState() override;

	void OnOk();
	void OnCancel();

	void SaveTreeViewState();
	void SaveTreeViewExpansionState(HWND hTreeView, HTREEITEM hItem);

	IExplorerplusplus *m_expp;

	BookmarkTree *m_bookmarkTree;
	BookmarkItem *m_bookmarkItem;
	BookmarkItem **m_selectedParentFolder;
	std::optional<std::wstring> m_customDialogTitle;

	BookmarkTreeView *m_pBookmarkTreeView;

	wil::unique_hbrush m_ErrorBrush;

	AddBookmarkDialogPersistentSettings *m_persistentSettings;
};