// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "Literals.h"
#include <wil/resource.h>
#include <optional>
#include <unordered_map>

class App;
struct Config;
class CoreInterface;
class OptionsPage;
class WindowSubclass;

class OptionsDialog : public BaseDialog
{
public:
	OptionsDialog(HINSTANCE resourceInstance, HWND parent, App *app, Config *config,
		CoreInterface *coreInterface);

private:
	// The amount of horizontal spacing between the navigation controls on the left side of the
	// dialog and the content pages on the right side.
	static constexpr auto NAVIGATION_CONTENT_HORIZONTAL_SPACING = 4_px;

	INT_PTR OnInitDialog() override;
	void SetupSearchField();
	LRESULT SearchFieldWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void AddDynamicControls() override;
	wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void AddPages();
	void AddPage(std::unique_ptr<OptionsPage> page);
	void AddPagesToTreeView();
	static bool PageFilter(const std::pair<const int, std::unique_ptr<OptionsPage>> &pageItem,
		const std::wstring &searchText);
	HTREEITEM AddPageToTreeView(const OptionsPage *page, int pageId);
	void SelectPage(int pageId);

	INT_PTR OnCtlColorStatic(HWND hwnd, HDC hdc) override;

	INT_PTR OnNotify(NMHDR *nmhdr) override;
	void OnTreeViewSelectionChanged(const NMTREEVIEW *changeInfo);

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	void OnSearchTextChanged();
	void OnOk();
	void OnApply();
	void OnCancel();
	INT_PTR OnClose() override;

	void OnSettingChanged();

	INT_PTR OnDestroy() override;
	INT_PTR OnNcDestroy() override;

	App *const m_app;
	Config *const m_config;
	HINSTANCE m_resourceInstance;
	CoreInterface *m_coreInterface;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	int m_idCounter = 0;
	std::unordered_map<int, std::unique_ptr<OptionsPage>> m_pageMap;
	std::unordered_map<int, HTREEITEM> m_treeMap;
	std::optional<int> m_currentPageId;
	bool m_initializationFinished = false;
	bool m_ignoreSelectionUpdate = false;

	wil::unique_hicon m_optionsDialogIcon;

	static inline std::optional<int> m_lastSelectedPageId;
};
