// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include <boost/signals2.hpp>
#include <memory>
#include <vector>

class NavigationEvents;
class ResourceLoader;
class SearchTabsDialog;
class ShellBrowserEvents;
class Tab;
class TabEvents;
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
};

class SearchTabsDialog : public BaseDialog
{
public:
	static SearchTabsDialog *Create(HWND parent, const TabList *tabList, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		const ResourceLoader *resourceLoader);

private:
	enum class ColumnType
	{
		TabName,
		Path
	};

	struct Column
	{
		ColumnType type;
		float percentageWidth;
	};

	enum class MoveDirection
	{
		Up,
		Down
	};

	static inline const Column COLUMNS[] = { { ColumnType::TabName, 0.3f },
		{ ColumnType::Path, 0.7f } };

	SearchTabsDialog(HWND parent, const TabList *tabList, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		const ResourceLoader *resourceLoader);

	INT_PTR OnInitDialog() override;
	wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SetupListView();
	void InsertColumns();
	void InsertColumn(const Column &column, int index);
	std::wstring GetColumnText(ColumnType columnType);
	void RefreshTabList();
	void AddTabs();
	void AddTab(const Tab *tab, int index);
	bool TabFilter(const Tab *tab);
	void SetupEditControl();
	void OnTabsChanged();

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *nmhdr);
	void OnListViewDoubleClick(const NMITEMACTIVATE *itemActivate);
	void OnGetDispInfo(NMLVDISPINFO *dispInfo);
	std::wstring GetTabColumnText(const Tab *tab, ColumnType columnType);
	LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMoveListViewSelection(MoveDirection direction);

	void OnOk();
	Tab *GetTabFromListView(int index);
	void OnCancel();
	INT_PTR OnClose() override;
	void SaveState() override;
	INT_PTR OnNcDestroy() override;

	const TabList *const m_tabList;
	TabEvents *const m_tabEvents;
	ShellBrowserEvents *const m_shellBrowserEvents;
	NavigationEvents *const m_navigationEvents;
	std::unique_ptr<WindowSubclass> m_editSubclass;
	static inline std::wstring m_filter;
	std::vector<boost::signals2::scoped_connection> m_connections;
	SearchTabsDialogPersistentSettings *m_persistentSettings;
};
