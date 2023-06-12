// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "OptionsPage.h"
#include <memory>
#include <vector>

class AdvancedOptionsPage : public OptionsPage
{
public:
	AdvancedOptionsPage(HWND parent, HINSTANCE resourceInstance, Config *config,
		CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
		HWND tooltipWindow);

	void SaveSettings() override;

	bool DoesPageContainText(const std::wstring &text,
		StringComparatorFunc stringComparator) override;

private:
	enum class AdvancedOptionId
	{
		CheckSystemIsPinnedToNameSpaceTree,
		OpenTabsInForeground,
		GoUpOnDoubleClick,
		QuickAccessInTreeView
	};

	enum class AdvancedOptionType
	{
		Boolean
	};

	struct AdvancedOption
	{
		AdvancedOptionId id;
		std::wstring name;
		AdvancedOptionType type;
		std::wstring description;
	};

	std::unique_ptr<ResizableDialogHelper> InitializeResizeDialogHelper() override;
	void InitializeControls() override;
	std::vector<AdvancedOption> InitializeAdvancedOptions();
	void InsertAdvancedOptionsIntoListView();
	bool GetBooleanConfigValue(AdvancedOptionId id);
	void SetBooleanConfigValue(AdvancedOptionId id, bool value);

	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	AdvancedOption *GetAdvancedOptionByIndex(int index);

	std::vector<AdvancedOption> m_advancedOptions;
};
