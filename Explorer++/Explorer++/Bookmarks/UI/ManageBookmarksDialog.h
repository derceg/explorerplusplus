// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkListView.h"
#include "ResourceHelper.h"
#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <boost/signals2.hpp>
#include <unordered_set>

class BookmarkNavigationController;
class BookmarkTree;
class BookmarkTreeView;
class BrowserWindow;
class CoreInterface;
class IconFetcher;
class IconResourceLoader;
class ManageBookmarksDialog;
class WindowSubclassWrapper;

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
	ManageBookmarksDialogPersistentSettings &operator=(
		const ManageBookmarksDialogPersistentSettings &);

	void SetupDefaultColumns();

	std::vector<BookmarkListView::Column> m_listViewColumns;

	bool m_bInitialized;
	std::unordered_set<std::wstring> m_setExpansion;
};

class ManageBookmarksDialog : public ThemedDialog
{
public:
	ManageBookmarksDialog(HINSTANCE resourceInstance, HWND hParent, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, const IconResourceLoader *iconResourceLoader,
		IconFetcher *iconFetcher, BookmarkTree *bookmarkTree);
	~ManageBookmarksDialog();

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnAppCommand(HWND hwnd, UINT uCmd, UINT uDevice, DWORD dwKeys) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;
	INT_PTR OnDestroy() override;
	INT_PTR OnNcDestroy() override;

	void SaveState() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	static const int TOOLBAR_ID_BACK = 10000;
	static const int TOOLBAR_ID_FORWARD = 10001;
	static const int TOOLBAR_ID_ORGANIZE = 10002;
	static const int TOOLBAR_ID_VIEWS = 10003;

	ManageBookmarksDialog &operator=(const ManageBookmarksDialog &mbd);

	void AddDynamicControls() override;
	std::vector<ResizableDialogControl> GetResizableControls() override;

	void CreateToolbar();
	void SetupToolbar();
	void SetupTreeView();
	void SetupListView();

	LRESULT CALLBACK ToolbarParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnTreeViewSelectionChanged(BookmarkItem *bookmarkFolder);
	void OnListViewNavigation(BookmarkItem *bookmarkFolder, bool addHistoryEntry);

	void UpdateToolbarState();

	LRESULT HandleMenuOrAccelerator(WPARAM wParam);

	void OnTbnDropDown(NMTOOLBAR *nmtb);

	// View menu
	void ShowViewMenu();
	void SetViewMenuItemStates(HMENU menu);
	void OnViewMenuItemSelected(int menuItemId);

	// Organize menu
	void ShowOrganizeMenu();
	void SetOrganizeMenuItemStates(HMENU menu);
	void OnOrganizeMenuItemSelected(int menuItemId);
	void OnNewBookmark();
	void OnNewFolder();
	void OnCopy(bool cut);
	void OnPaste();
	void OnDelete();
	void OnSelectAll();

	void OnOk();
	void OnCancel();

	HWND m_toolbarParent;
	HWND m_hToolbar;
	wil::unique_himagelist m_imageListToolbar;
	IconImageListMapping m_imageListToolbarMappings;

	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;
	const IconResourceLoader *const m_iconResourceLoader;
	IconFetcher *m_iconFetcher = nullptr;

	BookmarkTree *m_bookmarkTree = nullptr;

	BookmarkItem *m_currentBookmarkFolder = nullptr;

	BookmarkTreeView *m_bookmarkTreeView = nullptr;
	BookmarkListView *m_bookmarkListView = nullptr;

	std::unique_ptr<BookmarkNavigationController> m_navigationController;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	ManageBookmarksDialogPersistentSettings *m_persistentSettings = nullptr;
};
