// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconResourceLoader.h"
#include "OptionsPage.h"

class DarkModeManager;

class AppearanceOptionsPage : public OptionsPage
{
public:
	AppearanceOptionsPage(HWND parent, const ResourceLoader *resourceLoader,
		HINSTANCE resourceInstance, Config *config, CoreInterface *coreInterface,
		SettingChangedCallback settingChangedCallback, HWND tooltipWindow,
		const DarkModeManager *darkModeManager);

	void SaveSettings() override;

private:
	std::unique_ptr<ResizableDialogHelper> InitializeResizeDialogHelper() override;
	void InitializeControls() override;
	std::wstring GetIconSetText(IconSet iconSet);

	void OnCommand(WPARAM wParam, LPARAM lParam) override;

	const DarkModeManager *const m_darkModeManager;
};
