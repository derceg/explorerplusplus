// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"
#include <boost/signals2.hpp>
#include <memory>
#include <vector>

class CoreInterface;
class SearchTabsDialog;
class Tab;
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

class SearchTabsDialog : public ThemedDialog
{
public:
	static SearchTabsDialog *Create(HINSTANCE resourceInstance, HWND parent,
		CoreInterface *coreInterface);

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

	enum class SelectionOption
	{
		SelectActiveTab,
		SelectFirst
	};

	enum class MoveDirection
	{
		Up,
		Down
	};

	static inline const Column COLUMNS[] = { { ColumnType::TabName, 0.3f },
		{ ColumnType::Path, 0.7f } };

	SearchTabsDialog(HINSTANCE resourceInstance, HWND parent, CoreInterface *coreInterface);

	INT_PTR OnInitDialog() override;
	wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SetupListView();
	void InsertColumns();
	void InsertColumn(const Column &column, int index);
	std::wstring GetColumnText(ColumnType columnType);
	void RefreshTabList(SelectionOption selectionOption = SelectionOption::SelectFirst);
	void AddTabs(SelectionOption selectionOption);
	void AddTab(const Tab &tab, int index);
	bool TabFilter(const Tab &tab, const std::wstring &filter);
	void SetupEditControl();
	void OnTabsChanged();

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *nmhdr);
	void OnListViewDoubleClick(const NMITEMACTIVATE *itemActivate);
	void OnGetDispInfo(NMLVDISPINFO *dispInfo);
	std::wstring GetTabColumnText(const Tab &tab, ColumnType columnType);
	LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMoveListViewSelection(MoveDirection direction);

	void OnOk();
	Tab &GetTabFromListView(int index);
	void OnCancel();
	INT_PTR OnClose() override;
	void SaveState() override;
	INT_PTR OnNcDestroy() override;

	CoreInterface *m_coreInterface;
	std::unique_ptr<WindowSubclass> m_editSubclass;
	static inline std::wstring m_filter;
	std::vector<boost::signals2::scoped_connection> m_connections;
	SearchTabsDialogPersistentSettings *m_persistentSettings;
};
