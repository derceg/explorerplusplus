// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "Bookmarks/UI/BookmarkColumnModel.h"
#include "ResourceHelper.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <memory>
#include <unordered_set>
#include <vector>

class AcceleratorManager;
class AppServices;
class BookmarkHistoryEntry;
class BookmarkItem;
class BookmarkListPresenter;
class BookmarkNavigationController;
class BookmarkTree;
class BookmarkTreePresenter;
class ManageBookmarksDialog;
class PlatformContext;
class WindowSubclass;

class ManageBookmarksDialogPersistentSettings : public DialogSettings
{
public:
	static ManageBookmarksDialogPersistentSettings &GetInstance();

private:
	friend ManageBookmarksDialog;

	static const TCHAR SETTINGS_KEY[];

	ManageBookmarksDialogPersistentSettings();

	ManageBookmarksDialogPersistentSettings(const ManageBookmarksDialogPersistentSettings &);
	ManageBookmarksDialogPersistentSettings &operator=(
		const ManageBookmarksDialogPersistentSettings &);

	BookmarkColumnModel m_listViewColumnModel;
	std::unordered_set<std::wstring> m_expandedBookmarkIds;
};

class ManageBookmarksDialog : public BaseDialog
{
public:
	static ManageBookmarksDialog *Create(HWND parent, AppServices *appServices);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnAppCommand(HWND hwnd, UINT uCmd, UINT uDevice, DWORD dwKeys) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	void SaveState() override;

private:
	static const int TOOLBAR_ID_BACK = 10000;
	static const int TOOLBAR_ID_FORWARD = 10001;
	static const int TOOLBAR_ID_ORGANIZE = 10002;
	static const int TOOLBAR_ID_VIEWS = 10003;

	ManageBookmarksDialog(HWND parent, AppServices *appServices);
	~ManageBookmarksDialog() = default;

	ManageBookmarksDialog &operator=(const ManageBookmarksDialog &mbd);

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
	void ShowViewMenu();
	void ShowOrganizeMenu();

	void OnOk();
	void OnCancel();

	AppServices *const m_appServices;
	BookmarkTree *const m_bookmarkTree;
	const AcceleratorManager *const m_acceleratorManager;
	PlatformContext *const m_platformContext;

	HWND m_toolbarParent;
	HWND m_hToolbar;
	wil::unique_himagelist m_imageListToolbar;
	IconImageListMapping m_imageListToolbarMappings;

	BookmarkItem *m_currentBookmarkFolder = nullptr;

	std::unique_ptr<BookmarkTreePresenter> m_bookmarkTreePresenter;
	std::unique_ptr<BookmarkListPresenter> m_bookmarkListPresenter;

	std::unique_ptr<BookmarkNavigationController> m_navigationController;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	ManageBookmarksDialogPersistentSettings *m_persistentSettings = nullptr;
};
