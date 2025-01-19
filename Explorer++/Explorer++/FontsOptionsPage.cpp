// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FontsOptionsPage.h"
#include "Config.h"
#include "FontHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SystemFontHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ResizableDialogHelper.h"
#include "../Helper/WindowHelper.h"

FontsOptionsPage::FontsOptionsPage(HWND parent, HINSTANCE resourceInstance, Config *config,
	CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_FONTS, IDS_OPTIONS_FONTS_TITLE, parent, resourceInstance, config,
		coreInterface, settingChangedCallback, tooltipWindow)
{
}

std::unique_ptr<ResizableDialogHelper> FontsOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_MAIN_FONT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_FONTS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SIZE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SAMPLE), MovingType::None,
		SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void FontsOptionsPage::InitializeControls()
{
	m_systemLogFont = GetDefaultSystemFontScaledToWindow(GetDialog());

	InitializeFontsControl();
	InitializeSizeControl();
	InitializeSampleControl();

	UpdateSampleWindow();
}

void FontsOptionsPage::InitializeFontsControl()
{
	HWND fontsControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONTS);

	auto automaticText =
		ResourceHelper::LoadString(m_resourceInstance, IDS_OPTIONS_FONTS_AUTOMATIC);
	m_defaultItemIndex = ComboBox_AddString(fontsControl, automaticText.c_str());
	assert(m_defaultItemIndex != CB_ERR);

	auto &mainFont = m_config->mainFont.get();

	if (!mainFont)
	{
		[[maybe_unused]] auto res = ComboBox_SetCurSel(fontsControl, m_defaultItemIndex);
		assert(res != CB_ERR);
	}

	auto fonts = EnumerateUniqueFonts();

	// The size of all strings isn't exact, but that doesn't matter, as there's no issue with
	// supplying an estimate (the control will allocate whatever memory needs to be allocated,
	// regardless).
	SendMessage(fontsControl, CB_INITSTORAGE, fonts.size(),
		fonts.size() * LF_FULLFACESIZE * sizeof(WCHAR));

	for (const auto &font : fonts)
	{
		int index = ComboBox_AddString(fontsControl, font.c_str());

		if (index == CB_ERR)
		{
			assert(false);
			continue;
		}

		if (mainFont && font == mainFont->GetName())
		{
			[[maybe_unused]] auto res = ComboBox_SetCurSel(fontsControl, index);
			assert(res != CB_ERR);
		}
	}
}

std::set<std::wstring> FontsOptionsPage::EnumerateUniqueFonts()
{
	std::set<std::wstring> fonts;
	auto hdc = wil::GetDC(GetDialog());

	LOGFONT logFont = {};
	logFont.lfCharSet = DEFAULT_CHARSET;

	EnumFontFamiliesEx(hdc.get(), &logFont, EnumFontFamiliesExCallback,
		reinterpret_cast<LPARAM>(&fonts), 0);

	return fonts;
}

int CALLBACK FontsOptionsPage::EnumFontFamiliesExCallback(const LOGFONT *logFont,
	const TEXTMETRIC *textMetric, DWORD fontType, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(textMetric);

	if (WI_IsFlagClear(fontType, TRUETYPE_FONTTYPE))
	{
		return 1;
	}

	if (logFont->lfCharSet == SYMBOL_CHARSET)
	{
		return 1;
	}

	if (logFont->lfFaceName[0] == '@')
	{
		// These are vertically-oriented fonts (see
		// https://devblogs.microsoft.com/oldnewthing/20120719-00/?p=7093). Since there's no
		// native support for vertical text in the application, these fonts aren't considered
		// useful in the current context.
		return 1;
	}

	auto *enumLogFont = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);

	auto *fonts = reinterpret_cast<std::set<std::wstring> *>(lParam);
	fonts->insert(enumLogFont->elfFullName);

	return 1;
}

void FontsOptionsPage::InitializeSizeControl()
{
	HWND sizeControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SIZE);
	int sizeToSelect;

	auto &mainFont = m_config->mainFont.get();

	if (mainFont)
	{
		sizeToSelect = mainFont->GetSize();
	}
	else
	{
		int systemFontSize = std::abs(
			DpiCompatibility::GetInstance().PixelsToPoints(GetDialog(), m_systemLogFont.lfHeight));
		sizeToSelect =
			std::clamp(systemFontSize, CustomFont::MINIMUM_SIZE, CustomFont::MAXIMUM_SIZE);
	}

	for (int i = CustomFont::MINIMUM_SIZE; i <= CustomFont::MAXIMUM_SIZE; i++)
	{
		auto sizeText = std::to_wstring(i);
		int index = ComboBox_AddString(sizeControl, sizeText.c_str());

		if (index == CB_ERR)
		{
			assert(false);
			continue;
		}

		if (i == sizeToSelect)
		{
			[[maybe_unused]] auto res = ComboBox_SetCurSel(sizeControl, index);
			assert(res != CB_ERR);
		}
	}

	if (!mainFont)
	{
		EnableWindow(GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SIZE), false);
	}
}

void FontsOptionsPage::InitializeSampleControl()
{
	HWND sampleControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SAMPLE);
	AddWindowStyles(sampleControl, WS_BORDER, true);

	// This will cause the border to be drawn.
	SetWindowPos(sampleControl, nullptr, 0, 0, 0, 0,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
}

void FontsOptionsPage::UpdateSampleWindow()
{
	wil::unique_hfont newFont;

	HWND fontsControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONTS);
	int selectedIndex = ComboBox_GetCurSel(fontsControl);
	assert(selectedIndex != CB_ERR);

	if (selectedIndex == m_defaultItemIndex)
	{
		newFont.reset(CreateFontIndirect(&m_systemLogFont));
		assert(newFont);
	}
	else
	{
		auto selectedFont = GetWindowString(fontsControl);

		// Note that the set of sizes shown is fixed, so although std::stoi() can throw an
		// exception, that's not something that should ever happen here, since the sizes passed to
		// the method are always known to be valid.
		HWND sizeControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SIZE);
		auto sizeText = GetWindowString(sizeControl);
		int size = std::stoi(sizeText);

		newFont = CreateFontFromNameAndSize(selectedFont, size, GetDialog());
	}

	if (!newFont)
	{
		return;
	}

	HWND sampleControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SAMPLE);
	SendMessage(sampleControl, WM_SETFONT, reinterpret_cast<WPARAM>(newFont.get()), true);

	m_sampleFont = std::move(newFont);
}

INT_PTR FontsOptionsPage::DialogProcExtra(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(dlg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	switch (msg)
	{
	case WM_DPICHANGED_BEFOREPARENT:
		OnDpiChanged();
		break;
	}

	return 0;
}

void FontsOptionsPage::OnDpiChanged()
{
	m_systemLogFont = GetDefaultSystemFontScaledToWindow(GetDialog());
	UpdateSampleWindow();
}

void FontsOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			if (LOWORD(wParam) == IDC_OPTIONS_FONTS)
			{
				OnFontSelectionChanged();
			}
			else if (LOWORD(wParam) == IDC_OPTIONS_FONT_SIZE)
			{
				UpdateSampleWindow();
			}

			m_settingChangedCallback();
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_OPTIONS_FONT_RESET_TO_DEFAULT:
			OnResetToDefault();
			break;
		}
	}
}

void FontsOptionsPage::OnFontSelectionChanged()
{
	HWND fontsControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONTS);
	int selectedIndex = ComboBox_GetCurSel(fontsControl);
	assert(selectedIndex != CB_ERR);
	EnableWindow(GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SIZE),
		selectedIndex != m_defaultItemIndex);

	UpdateSampleWindow();
}

void FontsOptionsPage::OnResetToDefault()
{
	HWND fontsControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONTS);
	int selectedIndex = ComboBox_GetCurSel(fontsControl);
	assert(selectedIndex != CB_ERR);

	if (selectedIndex == m_defaultItemIndex)
	{
		return;
	}

	[[maybe_unused]] auto res = ComboBox_SetCurSel(fontsControl, m_defaultItemIndex);
	assert(res != CB_ERR);

	OnFontSelectionChanged();

	int systemFontSize = std::abs(
		DpiCompatibility::GetInstance().PixelsToPoints(GetDialog(), m_systemLogFont.lfHeight));
	int sizeToSelect =
		std::clamp(systemFontSize, CustomFont::MINIMUM_SIZE, CustomFont::MAXIMUM_SIZE);

	HWND sizeControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SIZE);
	res = ComboBox_SetCurSel(sizeControl, sizeToSelect - CustomFont::MINIMUM_SIZE);
	assert(res != CB_ERR);

	m_settingChangedCallback();
}

void FontsOptionsPage::SaveSettings()
{
	HWND fontsControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONTS);
	int selectedIndex = ComboBox_GetCurSel(fontsControl);
	assert(selectedIndex != CB_ERR);

	if (selectedIndex == m_defaultItemIndex)
	{
		m_config->mainFont = std::nullopt;
	}
	else
	{
		auto selectedFont = GetWindowString(fontsControl);

		HWND sizeControl = GetDlgItem(GetDialog(), IDC_OPTIONS_FONT_SIZE);
		auto sizeText = GetWindowString(sizeControl);
		int size = std::stoi(sizeText);

		m_config->mainFont = CustomFont(selectedFont, size);
	}
}
