// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DisplayColoursDialog.h"
#include "DisplayWindow/DisplayWindow.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"

const TCHAR DisplayColoursDialogPersistentSettings::SETTINGS_KEY[] = _T("DisplayColors");

DisplayColoursDialog::DisplayColoursDialog(HINSTANCE resourceInstance, HWND hParent,
	Config *config) :
	ThemedDialog(resourceInstance, IDD_DISPLAYCOLOURS, hParent, DialogSizingType::None),
	m_config(config)
{
	m_pdcdps = &DisplayColoursDialogPersistentSettings::GetInstance();
}

INT_PTR DisplayColoursDialog::OnInitDialog()
{
	InitializeColorGroups();
	InitializePreviewWindow();

	m_pdcdps->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

void DisplayColoursDialog::InitializeColorGroups()
{
	m_centerGroup[0].sliderId = IDC_SLIDER_CENTRE_RED;
	m_centerGroup[0].editId = IDC_EDIT_CENTRE_RED;
	m_centerGroup[0].color = Color::Red;

	m_centerGroup[1].sliderId = IDC_SLIDER_CENTRE_GREEN;
	m_centerGroup[1].editId = IDC_EDIT_CENTRE_GREEN;
	m_centerGroup[1].color = Color::Green;

	m_centerGroup[2].sliderId = IDC_SLIDER_CENTRE_BLUE;
	m_centerGroup[2].editId = IDC_EDIT_CENTRE_BLUE;
	m_centerGroup[2].color = Color::Blue;

	InitializeColorGroupControls(m_centerGroup);

	m_surroundingGroup[0].sliderId = IDC_SLIDER_SURROUND_RED;
	m_surroundingGroup[0].editId = IDC_EDIT_SURROUND_RED;
	m_surroundingGroup[0].color = Color::Red;

	m_surroundingGroup[1].sliderId = IDC_SLIDER_SURROUND_GREEN;
	m_surroundingGroup[1].editId = IDC_EDIT_SURROUND_GREEN;
	m_surroundingGroup[1].color = Color::Green;

	m_surroundingGroup[2].sliderId = IDC_SLIDER_SURROUND_BLUE;
	m_surroundingGroup[2].editId = IDC_EDIT_SURROUND_BLUE;
	m_surroundingGroup[2].color = Color::Blue;

	InitializeColorGroupControls(m_surroundingGroup);
}

void DisplayColoursDialog::InitializeColorGroupControls(ColorGroup colorGroup[NUM_COLORS])
{
	for (int i = 0; i < NUM_COLORS; i++)
	{
		SendDlgItemMessage(m_hDlg, colorGroup[i].sliderId, TBM_SETTICFREQ, TICK_REQUENCY, 0);
		SendDlgItemMessage(m_hDlg, colorGroup[i].sliderId, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
		SendDlgItemMessage(m_hDlg, colorGroup[i].editId, EM_SETLIMITTEXT, 3, 0);
	}
}

void DisplayColoursDialog::SetColorGroupValues(ColorGroup colorGroup[NUM_COLORS], COLORREF color)
{
	for (int i = 0; i < NUM_COLORS; i++)
	{
		UINT colorComponent = 0;

		switch (colorGroup[i].color)
		{
		case Color::Red:
			colorComponent = GetRValue(color);
			break;

		case Color::Green:
			colorComponent = GetGValue(color);
			break;

		case Color::Blue:
			colorComponent = GetBValue(color);
			break;

		default:
			assert(false);
			break;
		}

		SetDlgItemInt(m_hDlg, colorGroup[i].editId, colorComponent, FALSE);
		SendDlgItemMessage(m_hDlg, colorGroup[i].sliderId, TBM_SETPOS, TRUE, colorComponent);
	}
}

void DisplayColoursDialog::InitializePreviewWindow()
{
	CopyDisplayConfigFields(*m_config, m_previewConfig);

	HWND hStatic = GetDlgItem(m_hDlg, IDC_STATIC_PREVIEWDISPLAY);
	m_previewDisplayWindow = DisplayWindow::Create(hStatic, &m_previewConfig);

	DisplayWindow_ClearTextBuffer(m_previewDisplayWindow->GetHWND());

	auto fileName = ResourceHelper::LoadString(GetResourceInstance(), IDS_DISPLAYCOLORS_FILENAME);
	DisplayWindow_BufferText(m_previewDisplayWindow->GetHWND(), fileName.c_str());

	auto fileType = ResourceHelper::LoadString(GetResourceInstance(), IDS_DISPLAYCOLORS_FILE_TYPE);
	DisplayWindow_BufferText(m_previewDisplayWindow->GetHWND(), fileType.c_str());

	auto modificationDate =
		ResourceHelper::LoadString(GetResourceInstance(), IDS_DISPLAYCOLORS_MODIFICATION_DATE);
	DisplayWindow_BufferText(m_previewDisplayWindow->GetHWND(), modificationDate.c_str());

	RECT rc;
	GetWindowRect(hStatic, &rc);
	SetWindowPos(m_previewDisplayWindow->GetHWND(), nullptr, 0, 0, rc.right, rc.bottom,
		SWP_SHOWWINDOW | SWP_NOZORDER);

	SetColorGroupValues(m_centerGroup, m_previewConfig.displayWindowCentreColor.get());
	SetColorGroupValues(m_surroundingGroup, m_previewConfig.displayWindowSurroundColor.get());
}

INT_PTR DisplayColoursDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
			OnEnChange(LOWORD(wParam));
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_RESTOREDEFAULTS:
			OnRestoreDefaults();
			break;

		case IDC_BUTTON_DISPLAY_FONT:
			OnChooseFont();
			break;

		case IDOK:
			OnOk();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

void DisplayColoursDialog::OnRestoreDefaults()
{
	m_previewConfig.displayWindowCentreColor = DisplayWindowDefaults::CENTRE_COLOR;
	m_previewConfig.displayWindowSurroundColor = DisplayWindowDefaults::SURROUND_COLOR;
	m_previewConfig.displayWindowFont = DisplayWindowDefaults::FONT;
	m_previewConfig.displayWindowTextColor = DisplayWindowDefaults::TEXT_COLOR;

	SetColorGroupValues(m_centerGroup, m_previewConfig.displayWindowCentreColor.get());
	SetColorGroupValues(m_surroundingGroup, m_previewConfig.displayWindowSurroundColor.get());
}

void DisplayColoursDialog::OnChooseFont()
{
	LOGFONT currentFont = m_previewConfig.displayWindowFont.get();

	CHOOSEFONT cf = {};
	TCHAR szStyle[512];
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = m_hDlg;
	cf.Flags = CF_FORCEFONTEXIST | CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
	cf.lpLogFont = &currentFont;
	cf.rgbColors = m_previewConfig.displayWindowTextColor.get();
	cf.lCustData = NULL;
	cf.lpszStyle = szStyle;
	BOOL res = ChooseFont(&cf);

	if (res)
	{
		m_previewConfig.displayWindowFont = *cf.lpLogFont;
		m_previewConfig.displayWindowTextColor = cf.rgbColors;
	}
}

INT_PTR DisplayColoursDialog::OnHScroll(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);

	UpdateEditControlsFromSlider(m_centerGroup);
	UpdateEditControlsFromSlider(m_surroundingGroup);

	return 0;
}

void DisplayColoursDialog::UpdateEditControlsFromSlider(ColorGroup colorGroup[NUM_COLORS])
{
	for (int i = 0; i < NUM_COLORS; i++)
	{
		UINT colorComponent =
			static_cast<UINT>(SendDlgItemMessage(m_hDlg, colorGroup[i].sliderId, TBM_GETPOS, 0, 0));

		if (GetDlgItemInt(m_hDlg, colorGroup[i].editId, nullptr, FALSE) != colorComponent)
		{
			SetDlgItemInt(m_hDlg, colorGroup[i].editId, colorComponent, FALSE);
		}
	}
}

COLORREF DisplayColoursDialog::GetColorFromSliderGroup(ColorGroup colorGroup[NUM_COLORS])
{
	UINT r = 0;
	UINT g = 0;
	UINT b = 0;

	for (int i = 0; i < NUM_COLORS; i++)
	{
		UINT colorComponent =
			static_cast<UINT>(SendDlgItemMessage(m_hDlg, colorGroup[i].sliderId, TBM_GETPOS, 0, 0));

		switch (colorGroup[i].color)
		{
		case Color::Red:
			r = colorComponent;
			break;

		case Color::Green:
			g = colorComponent;
			break;

		case Color::Blue:
			b = colorComponent;
			break;

		default:
			assert(false);
			break;
		}
	}

	return RGB(r, g, b);
}

void DisplayColoursDialog::OnEnChange(UINT ControlID)
{
	BOOL translated;
	UINT value = GetDlgItemInt(m_hDlg, ControlID, &translated, FALSE);

	if (!translated)
	{
		return;
	}

	HWND hTrackBar = nullptr;

	switch (ControlID)
	{
	case IDC_EDIT_CENTRE_RED:
		hTrackBar = GetDlgItem(m_hDlg, IDC_SLIDER_CENTRE_RED);
		break;

	case IDC_EDIT_CENTRE_GREEN:
		hTrackBar = GetDlgItem(m_hDlg, IDC_SLIDER_CENTRE_GREEN);
		break;

	case IDC_EDIT_CENTRE_BLUE:
		hTrackBar = GetDlgItem(m_hDlg, IDC_SLIDER_CENTRE_BLUE);
		break;

	case IDC_EDIT_SURROUND_RED:
		hTrackBar = GetDlgItem(m_hDlg, IDC_SLIDER_SURROUND_RED);
		break;

	case IDC_EDIT_SURROUND_GREEN:
		hTrackBar = GetDlgItem(m_hDlg, IDC_SLIDER_SURROUND_GREEN);
		break;

	case IDC_EDIT_SURROUND_BLUE:
		hTrackBar = GetDlgItem(m_hDlg, IDC_SLIDER_SURROUND_BLUE);
		break;
	}

	SendMessage(hTrackBar, TBM_SETPOS, TRUE, value);

	if (ControlID == IDC_EDIT_CENTRE_RED || ControlID == IDC_EDIT_CENTRE_GREEN
		|| ControlID == IDC_EDIT_CENTRE_BLUE)
	{
		COLORREF color = GetColorFromSliderGroup(m_centerGroup);
		m_previewConfig.displayWindowCentreColor = color;
	}
	else if (ControlID == IDC_EDIT_SURROUND_RED || ControlID == IDC_EDIT_SURROUND_GREEN
		|| ControlID == IDC_EDIT_SURROUND_BLUE)
	{
		COLORREF color = GetColorFromSliderGroup(m_surroundingGroup);
		m_previewConfig.displayWindowSurroundColor = color;
	}
}

void DisplayColoursDialog::OnOk()
{
	CopyDisplayConfigFields(m_previewConfig, *m_config);

	EndDialog(m_hDlg, 1);
}

void DisplayColoursDialog::CopyDisplayConfigFields(const Config &sourceConfig, Config &destConfig)
{
	destConfig.displayWindowCentreColor = sourceConfig.displayWindowCentreColor.get();
	destConfig.displayWindowSurroundColor = sourceConfig.displayWindowSurroundColor.get();
	destConfig.displayWindowFont = sourceConfig.displayWindowFont.get();
	destConfig.displayWindowTextColor = sourceConfig.displayWindowTextColor.get();
}

void DisplayColoursDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

INT_PTR DisplayColoursDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void DisplayColoursDialog::SaveState()
{
	m_pdcdps->SaveDialogPosition(m_hDlg);

	m_pdcdps->m_bStateSaved = TRUE;
}

DisplayColoursDialogPersistentSettings::DisplayColoursDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
}

DisplayColoursDialogPersistentSettings &DisplayColoursDialogPersistentSettings::GetInstance()
{
	static DisplayColoursDialogPersistentSettings dcdps;
	return dcdps;
}
