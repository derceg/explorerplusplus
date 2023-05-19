// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AppearanceOptionsPage.h"
#include "Config.h"
#include "DarkModeHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/ResizableDialogHelper.h"

std::wstring GetIconSetText(IconSet iconSet, HINSTANCE resourceInstance);

AppearanceOptionsPage::AppearanceOptionsPage(HWND parent, HINSTANCE resourceInstance,
	Config *config, CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_APPEARANCE, IDS_OPTIONS_APPEARANCE_TITLE, parent, resourceInstance,
		config, coreInterface, settingChangedCallback, tooltipWindow)
{
}

std::unique_ptr<ResizableDialogHelper> AppearanceOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_ICON_SET), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_RESTART_FOOTNOTE_1),
		MovingType::Horizontal, SizingType::None);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_THEME), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_RESTART_FOOTNOTE_2),
		MovingType::Horizontal, SizingType::None);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_RESTART_NOTICE), MovingType::None,
		SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void AppearanceOptionsPage::InitializeControls()
{
	// Adding a tooltip to a static control works, provided that the SS_NOTIFY style is set.
	// However, the tooltip won't show up if the control is disabled.
	// Adding the tooltip based on the control rectangle, while leaving out the SS_NOTIFY style,
	// will work in both cases.
	AddTooltipForControl(m_tooltipWindow, GetDlgItem(GetDialog(), IDC_OPTIONS_THEME_LABEL),
		m_resourceInstance, IDS_OPTIONS_THEME_TOOLTIP, TooltipType::Rectangle);

	// These calls add a tooltip both to the combobox control and to the control rectangle. The
	// first tooltip will activate when the control is enabled, while the second will activate
	// when the control is disabled.
	AddTooltipForControl(m_tooltipWindow, GetDlgItem(GetDialog(), IDC_OPTIONS_THEME),
		m_resourceInstance, IDS_OPTIONS_THEME_TOOLTIP, TooltipType::Control);
	AddTooltipForControl(m_tooltipWindow, GetDlgItem(GetDialog(), IDC_OPTIONS_THEME),
		m_resourceInstance, IDS_OPTIONS_THEME_TOOLTIP, TooltipType::Rectangle);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeSupported())
	{
		EnableWindow(GetDlgItem(GetDialog(), IDC_OPTIONS_THEME_LABEL), false);
		EnableWindow(GetDlgItem(GetDialog(), IDC_OPTIONS_THEME), false);
	}

	std::vector<ComboBoxItem> iconSetItems;

	for (auto iconSet : IconSet::_values())
	{
		iconSetItems.emplace_back(iconSet, GetIconSetText(iconSet, m_resourceInstance));
	}

	AddItemsToComboBox(GetDlgItem(GetDialog(), IDC_OPTIONS_ICON_SET), iconSetItems,
		m_config->iconSet);

	std::vector<ComboBoxItem> themeItems;

	for (auto theme : Theme::_values())
	{
		themeItems.emplace_back(theme, GetThemeText(theme, m_resourceInstance));
	}

	AddItemsToComboBox(GetDlgItem(GetDialog(), IDC_OPTIONS_THEME), themeItems, m_config->theme);
}

std::wstring GetIconSetText(IconSet iconSet, HINSTANCE resourceInstance)
{
	UINT stringId;

	switch (iconSet)
	{
	case IconSet::Color:
		stringId = IDS_ICON_SET_COLOR;
		break;

	case IconSet::FluentUi:
		stringId = IDS_ICON_SET_FLUENT_UI;
		break;

	case IconSet::Windows10:
		stringId = IDS_ICON_SET_WINDOWS_10;
		break;

	default:
		throw std::runtime_error("IconSet value not found");
	}

	return ResourceHelper::LoadString(resourceInstance, stringId);
}

void AppearanceOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			m_settingChangedCallback();
			break;
		}
	}
}

void AppearanceOptionsPage::SaveSettings()
{
	int selectedIndex =
		static_cast<int>(SendDlgItemMessage(GetDialog(), IDC_OPTIONS_ICON_SET, CB_GETCURSEL, 0, 0));
	int iconSetItemData = static_cast<int>(
		SendDlgItemMessage(GetDialog(), IDC_OPTIONS_ICON_SET, CB_GETITEMDATA, selectedIndex, 0));
	m_config->iconSet = IconSet::_from_integral(iconSetItemData);

	selectedIndex =
		static_cast<int>(SendDlgItemMessage(GetDialog(), IDC_OPTIONS_THEME, CB_GETCURSEL, 0, 0));
	int themeItemData = static_cast<int>(
		SendDlgItemMessage(GetDialog(), IDC_OPTIONS_THEME, CB_GETITEMDATA, selectedIndex, 0));
	m_config->theme = Theme::_from_integral(themeItemData);
}
