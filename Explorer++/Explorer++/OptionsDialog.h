// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DarkModeDialogBase.h"
#include <wil/resource.h>
#include <optional>
#include <unordered_map>

struct Config;
class CoreInterface;
class OptionsPage;

class OptionsDialog : public DarkModeDialogBase
{
public:
	OptionsDialog(HINSTANCE resourceInstance, HWND parent, std::shared_ptr<Config> config,
		CoreInterface *coreInterface);

private:
	// The amount of horizontal spacing between the treeview and each page.
	static constexpr int TREEVIEW_PAGE_HORIZONTAL_SPACING = 4;

	INT_PTR OnInitDialog() override;
	void AddDynamicControls() override;
	wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void AddPages();
	void AddPage(std::unique_ptr<OptionsPage> page);
	void SelectPage(int id);

	INT_PTR OnNotify(NMHDR *nmhdr) override;
	void OnTreeViewSelectionChanged(const NMTREEVIEW *changeInfo);

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	LRESULT HandleMenuOrAccelerator(WPARAM wParam);
	void OnOk();
	void OnApply();
	void OnCancel();
	INT_PTR OnClose() override;

	void OnSettingChanged();

	INT_PTR OnDestroy() override;
	INT_PTR OnNcDestroy() override;

	std::shared_ptr<Config> m_config;
	HINSTANCE m_resourceInstance;
	CoreInterface *m_coreInterface;

	int m_idCounter = 0;
	std::unordered_map<int, std::unique_ptr<OptionsPage>> m_pageMap;
	std::unordered_map<int, HTREEITEM> m_treeMap;
	std::optional<int> m_currentPageId;
	bool m_initializationFinished = false;

	wil::unique_hicon m_optionsDialogIcon;

	static inline std::optional<int> m_lastSelectedPageId;
};
