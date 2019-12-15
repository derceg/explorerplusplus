// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "BookmarkTreeView.h"
#include "CoreInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Bookmark.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <wil/resource.h>
#include <unordered_set>

class CAddBookmarkDialog;

class CAddBookmarkDialogPersistentSettings : public CDialogSettings
{
public:

	static CAddBookmarkDialogPersistentSettings &GetInstance();

private:

	friend CAddBookmarkDialog;

	static const TCHAR SETTINGS_KEY[];

	CAddBookmarkDialogPersistentSettings();

	CAddBookmarkDialogPersistentSettings(const CAddBookmarkDialogPersistentSettings &);
	CAddBookmarkDialogPersistentSettings & operator=(const CAddBookmarkDialogPersistentSettings &);

	bool m_bInitialized;
	std::wstring m_guidSelected;
	std::unordered_set<std::wstring> m_setExpansion;
};

class CAddBookmarkDialog : public CBaseDialog
{
public:

	CAddBookmarkDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
		CBookmarkFolder &AllBookmarks, CBookmark &Bookmark);

	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark,std::size_t Position);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);
	void	OnBookmarkModified(const std::wstring &guid);
	void	OnBookmarkFolderModified(const std::wstring &guid);
	void	OnBookmarkRemoved(const std::wstring &guid);
	void	OnBookmarkFolderRemoved(const std::wstring &guid);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCtlColorEdit(HWND hwnd,HDC hdc);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();
	INT_PTR	OnNcDestroy();

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	static const COLORREF ERROR_BACKGROUND_COLOR = RGB(255,188,188);

	CAddBookmarkDialog & operator = (const CAddBookmarkDialog &abd);

	void		GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void		SaveState();

	void		OnOk();
	void		OnCancel();

	void		SaveTreeViewState();
	void		SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem);

	IExplorerplusplus *m_expp;

	CBookmarkFolder &m_AllBookmarks;
	CBookmark &m_Bookmark;

	CBookmarkTreeView *m_pBookmarkTreeView;

	wil::unique_hbrush m_ErrorBrush;

	CAddBookmarkDialogPersistentSettings *m_pabdps;
};