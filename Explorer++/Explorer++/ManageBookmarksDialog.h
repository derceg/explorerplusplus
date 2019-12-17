// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "BookmarkListView.h"
#include "BookmarkTree.h"
#include "BookmarkTreeView.h"
#include "CoreInterface.h"
#include "Navigation.h"
#include "ResourceHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <stack>
#include <unordered_set>

class CManageBookmarksDialog;

class CManageBookmarksDialogPersistentSettings : public CDialogSettings
{
public:

	static CManageBookmarksDialogPersistentSettings &GetInstance();

private:

	friend CManageBookmarksDialog;

	enum ColumnType_t
	{
		COLUMN_TYPE_NAME = 1,
		COLUMN_TYPE_LOCATION = 2,
		COLUMN_TYPE_DATE_CREATED = 3,
		COLUMN_TYPE_DATE_MODIFIED = 4
	};

	struct ColumnInfo_t
	{
		ColumnType_t	ColumnType;
		int				iWidth;
		bool			bActive;
	};

	static const TCHAR SETTINGS_KEY[];
	static const int DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH = 180;

	CManageBookmarksDialogPersistentSettings();

	CManageBookmarksDialogPersistentSettings(const CManageBookmarksDialogPersistentSettings &);
	CManageBookmarksDialogPersistentSettings & operator=(const CManageBookmarksDialogPersistentSettings &);

	void SetupDefaultColumns();

	std::vector<ColumnInfo_t> m_vectorColumnInfo;

	bool m_bInitialized;
	std::wstring m_guidSelected;
	std::unordered_set<std::wstring> m_setExpansion;

	NBookmarkHelper::SortMode_t m_SortMode;
	bool m_bSortAscending;
};

class CManageBookmarksDialog : public CBaseDialog
{
public:

	CManageBookmarksDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *pexpp,
		Navigation *navigation, BookmarkTree *bookmarkTree);
	~CManageBookmarksDialog();

	int CALLBACK		SortBookmarks(LPARAM lParam1,LPARAM lParam2);

	// TODO: Update.
	//void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnAppCommand(HWND hwnd,UINT uCmd,UINT uDevice,DWORD dwKeys);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();
	INT_PTR	OnNcDestroy();

	void	SaveState();

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	static const int TOOLBAR_ID_BACK			= 10000;
	static const int TOOLBAR_ID_FORWARD			= 10001;
	static const int TOOLBAR_ID_ORGANIZE		= 10002;
	static const int TOOLBAR_ID_VIEWS			= 10003;

	CManageBookmarksDialog & operator = (const CManageBookmarksDialog &mbd);

	void		SetupToolbar();
	void		SetupTreeView();
	void		SetupListView();

	void		SortListViewItems(NBookmarkHelper::SortMode_t SortMode);

	void		GetColumnString(CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,UINT cchBuf);
	void		GetBookmarkItemColumnInfo(const BookmarkItem *bookmarkItem,
		CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType, TCHAR *szColumn, size_t cchBuf);
	void		GetBookmarkColumnInfo(const BookmarkItem *bookmarkItem,
		CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType, TCHAR *szColumn, size_t cchBuf);
	void		GetBookmarkFolderColumnInfo(const BookmarkItem *bookmarkItem,
		CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType, TCHAR *szColumn, size_t cchBuf);

	void		BrowseBack();
	void		BrowseForward();
	void		BrowseBookmarkFolder(BookmarkItem *bookmarkItem);

	void		UpdateToolbarState();

	void		OnNewFolder();
	void		OnDeleteBookmark(const std::wstring &guid);

	void		OnDblClk(NMHDR *pnmhdr);
	void		OnRClick(NMHDR *pnmhdr);

	void		OnTbnDropDown(NMTOOLBAR *nmtb);
	void		ShowViewMenu();
	void		ShowOrganizeMenu();

	void		OnTvnSelChanged(NMTREEVIEW *pnmtv);

	void		OnListViewRClick();
	void		OnListViewHeaderRClick();
	BOOL		OnLvnEndLabelEdit(NMLVDISPINFO *pnmlvdi);
	void		OnLvnKeyDown(NMLVKEYDOWN *pnmlvkd);
	void		OnListViewRename();

	void		OnOk();
	void		OnCancel();

	HWND m_hToolbar;
	wil::unique_himagelist m_imageListToolbar;
	IconImageListMapping m_imageListToolbarMappings;

	IExplorerplusplus *m_pexpp;
	Navigation *m_navigation;

	BookmarkTree *m_bookmarkTree;

	std::wstring m_guidCurrentFolder;

	bool m_bNewFolderAdded;
	std::wstring m_guidNewFolder;

	std::stack<std::wstring> m_stackBack;
	std::stack<std::wstring> m_stackForward;
	bool m_bSaveHistory;

	CBookmarkTreeView *m_pBookmarkTreeView;

	bool m_bListViewInitialized;
	CBookmarkListView *m_pBookmarkListView;

	CManageBookmarksDialogPersistentSettings *m_pmbdps;
};