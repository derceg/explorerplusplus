// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "ListViewColumn.h"
#include "ListViewDelegate.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/SortDirection.h"
#include <boost/signals2.hpp>
#include <memory>
#include <optional>
#include <vector>

class AcceleratorManager;
class AsyncIconFetcher;
class KeyboardState;
class ListView;
class ResourceLoader;
class SearchTabsDialog;
class SearchTabsListViewModel;
class SearchTabsModel;
class Tab;
class TabList;
class WindowSubclass;

class SearchTabsDialogPersistentSettings : public DialogSettings
{
public:
	static SearchTabsDialogPersistentSettings &GetInstance();

private:
	friend SearchTabsDialog;

	static const inline std::wstring SETTINGS_KEY = L"SearchTabs";

	SearchTabsDialogPersistentSettings();

	std::optional<ListViewColumnId> m_sortColumn;
	std::optional<SortDirection> m_sortDirection;
	std::wstring m_searchTerm;
};

class SearchTabsDialog : public BaseDialog, private ListViewDelegate
{
public:
	static SearchTabsDialog *Create(HWND parent, std::unique_ptr<SearchTabsModel> model,
		const TabList *tabList, AsyncIconFetcher *iconFetcher, const KeyboardState *keyboardState,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader);

private:
	enum class MoveDirection
	{
		Up,
		Down
	};

	SearchTabsDialog(HWND parent, std::unique_ptr<SearchTabsModel> model, const TabList *tabList,
		AsyncIconFetcher *iconFetcher, const KeyboardState *keyboardState,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader);
	~SearchTabsDialog() = default;

	INT_PTR OnInitDialog() override;
	wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SetupListView();
	void SelectFirstItem();
	void SetupEditControl();

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMoveListViewSelection(MoveDirection direction);

	// ListViewDelegate
	void OnItemsActivated(const std::vector<ListViewItem *> &items) override;

	void OnOk();
	void SelectTabForItem(ListViewItem *item);
	void OnCancel();
	INT_PTR OnClose() override;
	void SaveState() override;

	const std::unique_ptr<SearchTabsModel> m_model;
	const TabList *const m_tabList;
	AsyncIconFetcher *const m_iconFetcher;
	const KeyboardState *const m_keyboardState;
	const AcceleratorManager *const m_acceleratorManager;
	std::unique_ptr<SearchTabsListViewModel> m_listViewModel;
	std::unique_ptr<ListView> m_listView;
	std::unique_ptr<WindowSubclass> m_editSubclass;
	std::vector<boost::signals2::scoped_connection> m_connections;
	SearchTabsDialogPersistentSettings *m_persistentSettings;
};
