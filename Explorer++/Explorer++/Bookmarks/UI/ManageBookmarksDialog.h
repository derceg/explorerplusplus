// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "Bookmarks/UI/BookmarkListView.h"
#include "ResourceHelper.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <boost/signals2.hpp>
#include <memory>
#include <unordered_set>
#include <vector>

class AcceleratorManager;
class BookmarkNavigationController;
class BookmarkTree;
class BookmarkTreePresenter;
class BrowserWindow;
class ClipboardStore;
struct Config;
class IconFetcher;
class ManageBookmarksDialog;
class WindowSubclass;

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

	bool m_initialized;
	std::unordered_set<std::wstring> m_expandedBookmarkIds;
};

class ManageBookmarksDialog : public BaseDialog
{
public:
	ManageBookmarksDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance,
		HWND hParent, BrowserWindow *browserWindow, const Config *config,
		const AcceleratorManager *acceleratorManager, IconFetcher *iconFetcher,
		BookmarkTree *bookmarkTree, ClipboardStore *clipboardStore);

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
	~ManageBookmarksDialog() = default;

	void AddDynamicControls() override;
	std::vector<ResizableDialogControl> GetResizableControls() override;

	void CreateToolbar();
	void SetupToolbar();
	void SetupTreeView();
	void SetupListView();

	LRESULT CALLBACK ToolbarParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnTreeViewSelectionChanged(BookmarkItem *bookmarkFolder);
	void OnListViewNavigation(BookmarkItem *bookmarkFolder, const BookmarkHistoryEntry *entry);

	void UpdateToolbarState();

	LRESULT HandleMenuOrAccelerator(WPARAM wParam);

	void OnTbnDropDown(NMTOOLBAR *nmtb);

	// View menu
	void ShowViewMenu();
	void SetViewMenuItemStates(HMENU menu);
	void OnViewMenuItemSelected(int menuItemId);

	// Organize menu
	void ShowOrganizeMenu();

	void OnOk();
	void OnCancel();

	HWND m_toolbarParent;
	HWND m_hToolbar;
	wil::unique_himagelist m_imageListToolbar;
	IconImageListMapping m_imageListToolbarMappings;

	const HINSTANCE m_resourceInstance;
	BrowserWindow *const m_browserWindow;
	const Config *const m_config;
	const AcceleratorManager *const m_acceleratorManager;
	IconFetcher *const m_iconFetcher;
	BookmarkTree *const m_bookmarkTree;
	ClipboardStore *const m_clipboardStore;

	BookmarkItem *m_currentBookmarkFolder = nullptr;

	std::unique_ptr<BookmarkTreePresenter> m_bookmarkTreePresenter;
	std::unique_ptr<BookmarkListView> m_bookmarkListView;

	std::unique_ptr<BookmarkNavigationController> m_navigationController;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	ManageBookmarksDialogPersistentSettings *m_persistentSettings = nullptr;
};
