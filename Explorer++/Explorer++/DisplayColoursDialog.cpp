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
	HWND hDisplayWindow, COLORREF DefaultCenterColor, COLORREF DefaultSurroundingColor) :
	ThemedDialog(resourceInstance, IDD_DISPLAYCOLOURS, hParent, DialogSizingType::None),
	m_hDisplayWindow(hDisplayWindow),
	m_DefaultCenterColor(DefaultCenterColor),
	m_DefaultSurroundingColor(DefaultSurroundingColor)
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
	m_CenterGroup[0].sliderId = IDC_SLIDER_CENTRE_RED;
	m_CenterGroup[0].editId = IDC_EDIT_CENTRE_RED;
	m_CenterGroup[0].color = Color::Red;

	m_CenterGroup[1].sliderId = IDC_SLIDER_CENTRE_GREEN;
	m_CenterGroup[1].editId = IDC_EDIT_CENTRE_GREEN;
	m_CenterGroup[1].color = Color::Green;

	m_CenterGroup[2].sliderId = IDC_SLIDER_CENTRE_BLUE;
	m_CenterGroup[2].editId = IDC_EDIT_CENTRE_BLUE;
	m_CenterGroup[2].color = Color::Blue;

	InitializeColorGroupControls(m_CenterGroup);

	m_SurroundingGroup[0].sliderId = IDC_SLIDER_SURROUND_RED;
	m_SurroundingGroup[0].editId = IDC_EDIT_SURROUND_RED;
	m_SurroundingGroup[0].color = Color::Red;

	m_SurroundingGroup[1].sliderId = IDC_SLIDER_SURROUND_GREEN;
	m_SurroundingGroup[1].editId = IDC_EDIT_SURROUND_GREEN;
	m_SurroundingGroup[1].color = Color::Green;

	m_SurroundingGroup[2].sliderId = IDC_SLIDER_SURROUND_BLUE;
	m_SurroundingGroup[2].editId = IDC_EDIT_SURROUND_BLUE;
	m_SurroundingGroup[2].color = Color::Blue;

	InitializeColorGroupControls(m_SurroundingGroup);
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
	auto centreColor =
		static_cast<COLORREF>(SendMessage(m_hDisplayWindow, DWM_GETCENTRECOLOR, 0, 0));
	auto surroundColor =
		static_cast<COLORREF>(SendMessage(m_hDisplayWindow, DWM_GETSURROUNDCOLOR, 0, 0));

	DisplayWindow_GetFont(m_hDisplayWindow, reinterpret_cast<WPARAM>(&m_hDisplayFont));
	m_TextColor = DisplayWindow_GetTextColor(m_hDisplayWindow);

	m_hDisplayWindowIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(nullptr),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW), IMAGE_ICON, 0, 0, LR_CREATEDIBSECTION));

	DWInitialSettings_t initialSettings;
	initialSettings.CentreColor = centreColor;
	initialSettings.SurroundColor = surroundColor;
	initialSettings.TextColor = m_TextColor;
	initialSettings.hFont = m_hDisplayFont;
	initialSettings.hIcon = m_hDisplayWindowIcon;

	HWND hStatic = GetDlgItem(m_hDlg, IDC_STATIC_PREVIEWDISPLAY);
	m_previewDisplayWindow = DisplayWindow::Create(hStatic, &initialSettings);

	SendMessage(m_previewDisplayWindow->GetHWND(), DWM_SETSURROUNDCOLOR, surroundColor, 0);
	SendMessage(m_previewDisplayWindow->GetHWND(), DWM_SETCENTRECOLOR, centreColor, 0);
	DisplayWindow_SetFont(m_previewDisplayWindow->GetHWND(),
		reinterpret_cast<WPARAM>(m_hDisplayFont));
	DisplayWindow_SetTextColor(m_previewDisplayWindow->GetHWND(), m_TextColor);

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

	SetColorGroupValues(m_CenterGroup, centreColor);
	SetColorGroupValues(m_SurroundingGroup, surroundColor);
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
	/* TODO: Need to delete old display window fonts. */
	// DeleteFont(m_DisplayWindowFont);

	/* TODO: This should be defined externally. */
	m_hDisplayFont = CreateFont(-13, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN,
		_T("Segoe UI"));

	/* TODO: Default text color. */
	m_TextColor = RGB(0, 0, 0);

	SendMessage(m_previewDisplayWindow->GetHWND(), DWM_SETCENTRECOLOR, m_DefaultCenterColor, 0);
	SendMessage(m_previewDisplayWindow->GetHWND(), DWM_SETSURROUNDCOLOR, m_DefaultSurroundingColor,
		0);
	DisplayWindow_SetFont(m_previewDisplayWindow->GetHWND(),
		reinterpret_cast<WPARAM>(m_hDisplayFont));
	DisplayWindow_SetTextColor(m_previewDisplayWindow->GetHWND(), m_TextColor);

	SetColorGroupValues(m_CenterGroup, m_DefaultCenterColor);
	SetColorGroupValues(m_SurroundingGroup, m_DefaultSurroundingColor);
}

void DisplayColoursDialog::OnChooseFont()
{
	HFONT hFont;
	DisplayWindow_GetFont(m_previewDisplayWindow->GetHWND(), reinterpret_cast<WPARAM>(&hFont));

	LOGFONT lf;
	GetObject(hFont, sizeof(lf), reinterpret_cast<LPVOID>(&lf));

	CHOOSEFONT cf;
	TCHAR szStyle[512];
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = m_hDlg;
	cf.Flags = CF_FORCEFONTEXIST | CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
	cf.lpLogFont = &lf;
	cf.rgbColors = DisplayWindow_GetTextColor(m_previewDisplayWindow->GetHWND());
	cf.lCustData = NULL;
	cf.lpszStyle = szStyle;
	BOOL res = ChooseFont(&cf);

	if (res)
	{
		/* TODO: This font must be freed. */
		m_hDisplayFont = CreateFontIndirect(cf.lpLogFont);
		m_TextColor = cf.rgbColors;

		DisplayWindow_SetFont(m_previewDisplayWindow->GetHWND(),
			reinterpret_cast<WPARAM>(m_hDisplayFont));
		DisplayWindow_SetTextColor(m_previewDisplayWindow->GetHWND(), m_TextColor);
	}
}

INT_PTR DisplayColoursDialog::OnHScroll(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);

	UpdateEditControlsFromSlider(m_CenterGroup);
	UpdateEditControlsFromSlider(m_SurroundingGroup);

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
		COLORREF color = GetColorFromSliderGroup(m_CenterGroup);
		SendMessage(m_previewDisplayWindow->GetHWND(), DWM_SETCENTRECOLOR, color, 0);
	}
	else if (ControlID == IDC_EDIT_SURROUND_RED || ControlID == IDC_EDIT_SURROUND_GREEN
		|| ControlID == IDC_EDIT_SURROUND_BLUE)
	{
		COLORREF color = GetColorFromSliderGroup(m_SurroundingGroup);
		SendMessage(m_previewDisplayWindow->GetHWND(), DWM_SETSURROUNDCOLOR, color, 0);
	}
}

void DisplayColoursDialog::OnOk()
{
	COLORREF centerColor = GetColorFromSliderGroup(m_CenterGroup);
	SendMessage(m_hDisplayWindow, DWM_SETCENTRECOLOR, centerColor, 0);

	COLORREF surroundingColor = GetColorFromSliderGroup(m_SurroundingGroup);
	SendMessage(m_hDisplayWindow, DWM_SETSURROUNDCOLOR, surroundingColor, 0);

	DisplayWindow_SetFont(m_hDisplayWindow, reinterpret_cast<WPARAM>(m_hDisplayFont));
	DisplayWindow_SetTextColor(m_hDisplayWindow, m_TextColor);

	EndDialog(m_hDlg, 1);
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

INT_PTR DisplayColoursDialog::OnDestroy()
{
	DestroyIcon(m_hDisplayWindowIcon);
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
