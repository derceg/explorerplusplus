// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <wil/resource.h>
#include <memory>
#include <optional>
#include <unordered_set>

class AcceleratorManager;
class AddBookmarkDialog;
class BookmarkItem;
class BookmarkTree;
class BookmarkTreePresenter;
class ClipboardStore;

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

	bool m_initialized;
	std::wstring m_selectedBookmarkId;
	std::unordered_set<std::wstring> m_expandedBookmarkIds;
};

class AddBookmarkDialog : public BaseDialog
{
public:
	AddBookmarkDialog(const ResourceLoader *resourceLoader, HWND hParent,
		BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem,
		BookmarkItem *defaultParentSelection, BookmarkItem **selectedParentFolder,
		ClipboardStore *clipboardStore, const AcceleratorManager *acceleratorManager,
		std::optional<std::wstring> customDialogTitle = std::nullopt);
	~AddBookmarkDialog();

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

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

	BookmarkTree *m_bookmarkTree;
	BookmarkItem *m_bookmarkItem;
	BookmarkItem **m_selectedParentFolder;
	ClipboardStore *const m_clipboardStore;
	const AcceleratorManager *const m_acceleratorManager;
	std::optional<std::wstring> m_customDialogTitle;

	std::unique_ptr<BookmarkTreePresenter> m_bookmarkTreePresenter;

	AddBookmarkDialogPersistentSettings *m_persistentSettings;
};
