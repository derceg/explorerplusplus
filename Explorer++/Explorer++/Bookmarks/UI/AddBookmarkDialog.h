// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <wil/resource.h>
#include <optional>
#include <unordered_set>

class AddBookmarkDialog;
class BookmarkItem;
class BookmarkTree;
class BookmarkTreeView;
class IconResourceLoader;

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

class AddBookmarkDialog : public ThemedDialog
{
public:
	AddBookmarkDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance,
		HWND hParent, ThemeManager *themeManager, BookmarkTree *bookmarkTree,
		BookmarkItem *bookmarkItem, BookmarkItem *defaultParentSelection,
		BookmarkItem **selectedParentFolder, const IconResourceLoader *iconResourceLoader,
		std::optional<std::wstring> customDialogTitle = std::nullopt);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;
	INT_PTR OnNcDestroy() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	AddBookmarkDialog &operator=(const AddBookmarkDialog &abd);

	void UpdateDialogForBookmarkFolder();
	void SetDialogTitle();
	std::wstring LoadDialogTitle();

	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnOk();
	void OnCancel();

	void SaveTreeViewState();
	void SaveTreeViewExpansionState(HWND hTreeView, HTREEITEM hItem);

	BookmarkTree *m_bookmarkTree;
	BookmarkItem *m_bookmarkItem;
	BookmarkItem **m_selectedParentFolder;
	const IconResourceLoader *const m_iconResourceLoader;
	std::optional<std::wstring> m_customDialogTitle;

	BookmarkTreeView *m_pBookmarkTreeView;

	AddBookmarkDialogPersistentSettings *m_persistentSettings;
};
