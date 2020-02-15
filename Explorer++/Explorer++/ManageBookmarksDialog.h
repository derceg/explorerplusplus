// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkListView.h"
#include "BookmarkTreeView.h"
#include "Navigation.h"
#include "ResourceHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include <boost/signals2.hpp>
#include <stack>
#include <unordered_set>

class BookmarkTree;
__interface IExplorerplusplus;
class ManageBookmarksDialog;

class ManageBookmarksDialogPersistentSettings : public DialogSettings
{
public:

	static ManageBookmarksDialogPersistentSettings &GetInstance();

private:

	friend ManageBookmarksDialog;

	static const TCHAR SETTINGS_KEY[];
	static const int DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH = 180;

	ManageBookmarksDialogPersistentSettings();

	ManageBookmarksDialogPersistentSettings(const ManageBookmarksDialogPersistentSettings &);
	ManageBookmarksDialogPersistentSettings & operator=(const ManageBookmarksDialogPersistentSettings &);

	void SetupDefaultColumns();

	std::vector<BookmarkListView::Column> m_listViewColumns;

	bool m_bInitialized;
	std::unordered_set<std::wstring> m_setExpansion;
};

class ManageBookmarksDialog : public BaseDialog
{
public:

	ManageBookmarksDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *pexpp,
		Navigation *navigation, BookmarkTree *bookmarkTree);
	~ManageBookmarksDialog();

protected:

	INT_PTR	OnInitDialog() override;
	INT_PTR	OnAppCommand(HWND hwnd,UINT uCmd,UINT uDevice,DWORD dwKeys) override;
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam) override;
	INT_PTR	OnNotify(NMHDR *pnmhdr) override;
	INT_PTR	OnClose() override;
	INT_PTR	OnDestroy() override;
	INT_PTR	OnNcDestroy() override;

	void	SaveState() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	static const int TOOLBAR_ID_BACK			= 10000;
	static const int TOOLBAR_ID_FORWARD			= 10001;
	static const int TOOLBAR_ID_ORGANIZE		= 10002;
	static const int TOOLBAR_ID_VIEWS			= 10003;

	ManageBookmarksDialog & operator = (const ManageBookmarksDialog &mbd);

	void		SetupToolbar();
	void		SetupTreeView();
	void		SetupListView();

	void		BrowseBack();
	void		BrowseForward();

	void		OnTreeViewSelectionChanged(BookmarkItem *bookmarkFolder);
	void		OnListViewNavigation(BookmarkItem *bookmarkFolder);

	void		UpdateToolbarState();

	LRESULT		HandleMenuOrAccelerator(WPARAM wParam);

	void		OnNewFolder();
	void		OnDeleteBookmark(const std::wstring &guid);

	void		OnTbnDropDown(NMTOOLBAR *nmtb);
	void		ShowViewMenu();
	void		ShowOrganizeMenu();

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

	BookmarkTreeView *m_bookmarkTreeView;
	BookmarkListView *m_bookmarkListView;

	std::vector<boost::signals2::scoped_connection> m_connections;

	ManageBookmarksDialogPersistentSettings *m_persistentSettings;
};