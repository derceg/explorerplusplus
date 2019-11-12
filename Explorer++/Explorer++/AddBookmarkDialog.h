// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "BookmarkTreeView.h"
#include "DpiCompatibility.h"
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

	~CAddBookmarkDialogPersistentSettings();

	static CAddBookmarkDialogPersistentSettings &GetInstance();

private:

	friend CAddBookmarkDialog;

	static const TCHAR SETTINGS_KEY[];

	CAddBookmarkDialogPersistentSettings();

	CAddBookmarkDialogPersistentSettings(const CAddBookmarkDialogPersistentSettings &);
	CAddBookmarkDialogPersistentSettings & operator=(const CAddBookmarkDialogPersistentSettings &);

	bool							m_bInitialized;
	GUID							m_guidSelected;
	NBookmarkHelper::setExpansion_t	m_setExpansion;
};

class CAddBookmarkDialog : public CBaseDialog, public NBookmark::IBookmarkItemNotification
{
public:

	CAddBookmarkDialog(HINSTANCE hInstance,int iResource,HWND hParent,CBookmarkFolder &AllBookmarks,CBookmark &Bookmark);
	~CAddBookmarkDialog();

	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark,std::size_t Position);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);
	void	OnBookmarkModified(const GUID &guid);
	void	OnBookmarkFolderModified(const GUID &guid);
	void	OnBookmarkRemoved(const GUID &guid);
	void	OnBookmarkFolderRemoved(const GUID &guid);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCtlColorEdit(HWND hwnd,HDC hdc);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();
	INT_PTR	OnDestroy();
	INT_PTR	OnNcDestroy();

private:

	static const COLORREF ERROR_BACKGROUND_COLOR = RGB(255,188,188);

	CAddBookmarkDialog & operator = (const CAddBookmarkDialog &abd);

	void		GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void		SaveState();

	void		SetDialogIcon();

	void		OnOk();
	void		OnCancel();

	void		SaveTreeViewState();
	void		SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem);

	DpiCompatibility m_dpiCompat;
	wil::unique_hicon m_icon;

	CBookmarkFolder &m_AllBookmarks;
	CBookmark &m_Bookmark;

	CBookmarkTreeView *m_pBookmarkTreeView;

	HBRUSH m_ErrorBrush;

	CAddBookmarkDialogPersistentSettings *m_pabdps;
};