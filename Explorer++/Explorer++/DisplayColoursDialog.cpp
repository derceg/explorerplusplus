// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DisplayColoursDialog.h"
#include "MainResource.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/Macros.h"
#include <list>


const TCHAR CDisplayColoursDialogPersistentSettings::SETTINGS_KEY[] = _T("DisplayColors");

CDisplayColoursDialog::CDisplayColoursDialog(HINSTANCE hInstance,int iResource,
	HWND hParent,HWND hDisplayWindow,COLORREF DefaultCenterColor,COLORREF DefaultSurroundingColor) :
	CBaseDialog(hInstance, iResource, hParent, false),
	m_hDisplayWindow(hDisplayWindow),
	m_DefaultCenterColor(DefaultCenterColor),
	m_DefaultSurroundingColor(DefaultSurroundingColor)
{
	m_pdcdps = &CDisplayColoursDialogPersistentSettings::GetInstance();
}

INT_PTR CDisplayColoursDialog::OnInitDialog()
{
	InitializeColorGroups();
	InitializePreviewWindow();

	m_pdcdps->RestoreDialogPosition(m_hDlg,false);

	return 0;
}

void CDisplayColoursDialog::InitializeColorGroups()
{
	m_CenterGroup[0].SliderId	= IDC_SLIDER_CENTRE_RED;
	m_CenterGroup[0].EditId		= IDC_EDIT_CENTRE_RED;
	m_CenterGroup[0].Color		= COLOR_RED;

	m_CenterGroup[1].SliderId	= IDC_SLIDER_CENTRE_GREEN;
	m_CenterGroup[1].EditId		= IDC_EDIT_CENTRE_GREEN;
	m_CenterGroup[1].Color		= COLOR_GREEN;

	m_CenterGroup[2].SliderId	= IDC_SLIDER_CENTRE_BLUE;
	m_CenterGroup[2].EditId		= IDC_EDIT_CENTRE_BLUE;
	m_CenterGroup[2].Color		= COLOR_BLUE;

	InitializeColorGroupControls(m_CenterGroup);

	m_SurroundingGroup[0].SliderId	= IDC_SLIDER_SURROUND_RED;
	m_SurroundingGroup[0].EditId	= IDC_EDIT_SURROUND_RED;
	m_SurroundingGroup[0].Color	= COLOR_RED;

	m_SurroundingGroup[1].SliderId	= IDC_SLIDER_SURROUND_GREEN;
	m_SurroundingGroup[1].EditId	= IDC_EDIT_SURROUND_GREEN;
	m_SurroundingGroup[1].Color	= COLOR_GREEN;

	m_SurroundingGroup[2].SliderId	= IDC_SLIDER_SURROUND_BLUE;
	m_SurroundingGroup[2].EditId	= IDC_EDIT_SURROUND_BLUE;
	m_SurroundingGroup[2].Color	= COLOR_BLUE;

	InitializeColorGroupControls(m_SurroundingGroup);
}

void CDisplayColoursDialog::InitializeColorGroupControls(ColorGroup_t ColorGroup[NUM_COLORS])
{
	for(int i = 0;i < NUM_COLORS;i++)
	{
		SendDlgItemMessage(m_hDlg,ColorGroup[i].SliderId,TBM_SETTICFREQ,TICK_REQUENCY,0);
		SendDlgItemMessage(m_hDlg,ColorGroup[i].SliderId,TBM_SETRANGE,TRUE,MAKELONG(0,255));
		SendDlgItemMessage(m_hDlg,ColorGroup[i].EditId,EM_SETLIMITTEXT,3,0);
	}
}

void CDisplayColoursDialog::SetColorGroupValues(ColorGroup_t ColorGroup[NUM_COLORS],COLORREF Color)
{
	for(int i = 0;i < NUM_COLORS;i++)
	{
		UINT ColorComponent = 0;

		switch(ColorGroup[i].Color)
		{
		case COLOR_RED:
			ColorComponent = GetRValue(Color);
			break;

		case COLOR_GREEN:
			ColorComponent = GetGValue(Color);
			break;

		case COLOR_BLUE:
			ColorComponent = GetBValue(Color);
			break;

		default:
			assert(false);
			break;
		}

		SetDlgItemInt(m_hDlg,ColorGroup[i].EditId,ColorComponent,FALSE);
		SendDlgItemMessage(m_hDlg,ColorGroup[i].SliderId,TBM_SETPOS,TRUE,ColorComponent);
	}
}

void CDisplayColoursDialog::InitializePreviewWindow()
{
	COLORREF CentreColor = static_cast<COLORREF>(SendMessage(m_hDisplayWindow,DWM_GETCENTRECOLOR,0,0));
	COLORREF SurroundColor = static_cast<COLORREF>(SendMessage(m_hDisplayWindow,DWM_GETSURROUNDCOLOR,0,0));

	DisplayWindow_GetFont(m_hDisplayWindow,reinterpret_cast<WPARAM>(&m_hDisplayFont));
	m_TextColor = DisplayWindow_GetTextColor(m_hDisplayWindow);

	m_hDisplayWindowIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW),IMAGE_ICON,0,0,LR_CREATEDIBSECTION));

	DWInitialSettings_t InitialSettings;
	InitialSettings.CentreColor		= CentreColor;
	InitialSettings.SurroundColor	= SurroundColor;
	InitialSettings.TextColor		= m_TextColor;
	InitialSettings.hFont			= m_hDisplayFont;
	InitialSettings.hIcon			= m_hDisplayWindowIcon;

	HWND hStatic = GetDlgItem(m_hDlg,IDC_STATIC_PREVIEWDISPLAY);
	m_hPreviewDisplayWindow = CreateDisplayWindow(hStatic,&InitialSettings);

	SendMessage(m_hPreviewDisplayWindow,DWM_SETSURROUNDCOLOR,SurroundColor,0);
	SendMessage(m_hPreviewDisplayWindow,DWM_SETCENTRECOLOR,CentreColor,0);
	DisplayWindow_SetFont(m_hPreviewDisplayWindow,reinterpret_cast<WPARAM>(m_hDisplayFont));
	DisplayWindow_SetTextColor(m_hPreviewDisplayWindow,m_TextColor);

	DisplayWindow_ClearTextBuffer(m_hPreviewDisplayWindow);

	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_DISPLAYCOLORS_FILENAME,szTemp,SIZEOF_ARRAY(szTemp));
	DisplayWindow_BufferText(m_hPreviewDisplayWindow,szTemp);

	LoadString(GetInstance(),IDS_DISPLAYCOLORS_FILE_TYPE,szTemp,SIZEOF_ARRAY(szTemp));
	DisplayWindow_BufferText(m_hPreviewDisplayWindow,szTemp);

	LoadString(GetInstance(),IDS_DISPLAYCOLORS_MODIFICATION_DATE,szTemp,SIZEOF_ARRAY(szTemp));
	DisplayWindow_BufferText(m_hPreviewDisplayWindow,szTemp);

	RECT rc;
	GetWindowRect(hStatic,&rc);
	SetWindowPos(m_hPreviewDisplayWindow,NULL,0,0,rc.right,rc.bottom,SWP_SHOWWINDOW|SWP_NOZORDER);

	SetColorGroupValues(m_CenterGroup,CentreColor);
	SetColorGroupValues(m_SurroundingGroup,SurroundColor);
}

INT_PTR CDisplayColoursDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if(HIWORD(wParam) != 0)
	{
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			OnEnChange(LOWORD(wParam));
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
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

void CDisplayColoursDialog::OnRestoreDefaults()
{
	/* TODO: Need to delete old display window fonts. */
	//DeleteFont(m_DisplayWindowFont);

	/* TODO: This should be defined externally. */
	m_hDisplayFont	= CreateFont(-13,0,0,0,FW_MEDIUM,FALSE,
		FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FIXED_PITCH|FF_MODERN,
		_T("Segoe UI"));

	/* TODO: Default text color. */
	m_TextColor = RGB(0,0,0);

	SendMessage(m_hPreviewDisplayWindow,DWM_SETCENTRECOLOR,m_DefaultCenterColor,0);
	SendMessage(m_hPreviewDisplayWindow,DWM_SETSURROUNDCOLOR,m_DefaultSurroundingColor,0);
	DisplayWindow_SetFont(m_hPreviewDisplayWindow,reinterpret_cast<WPARAM>(m_hDisplayFont));
	DisplayWindow_SetTextColor(m_hPreviewDisplayWindow,m_TextColor);

	SetColorGroupValues(m_CenterGroup,m_DefaultCenterColor);
	SetColorGroupValues(m_SurroundingGroup,m_DefaultSurroundingColor);
}

void CDisplayColoursDialog::OnChooseFont()
{
	HFONT hFont;
	DisplayWindow_GetFont(m_hPreviewDisplayWindow,reinterpret_cast<WPARAM>(&hFont));

	LOGFONT lf;
	GetObject(hFont,sizeof(lf),reinterpret_cast<LPVOID>(&lf));

	CHOOSEFONT cf;
	TCHAR szStyle[512];
	cf.lStructSize	= sizeof(cf);
	cf.hwndOwner	= m_hDlg;
	cf.Flags		= CF_FORCEFONTEXIST|CF_SCREENFONTS|CF_EFFECTS|CF_INITTOLOGFONTSTRUCT;
	cf.lpLogFont	= &lf;
	cf.rgbColors	= DisplayWindow_GetTextColor(m_hPreviewDisplayWindow);
	cf.lCustData	= NULL;
	cf.lpszStyle	= szStyle;
	BOOL res = ChooseFont(&cf);

	if(res)
	{
		/* TODO: This font must be freed. */
		m_hDisplayFont = CreateFontIndirect(cf.lpLogFont);
		m_TextColor = cf.rgbColors;

		DisplayWindow_SetFont(m_hPreviewDisplayWindow,reinterpret_cast<WPARAM>(m_hDisplayFont));
		DisplayWindow_SetTextColor(m_hPreviewDisplayWindow,m_TextColor);
	}
}

INT_PTR CDisplayColoursDialog::OnHScroll(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);

	UpdateEditControlsFromSlider(m_CenterGroup);
	UpdateEditControlsFromSlider(m_SurroundingGroup);

	return 0;
}

void CDisplayColoursDialog::UpdateEditControlsFromSlider(ColorGroup_t ColorGroup[NUM_COLORS])
{
	for(int i = 0;i < NUM_COLORS;i++)
	{
		UINT ColorComponent = static_cast<UINT>(SendDlgItemMessage(m_hDlg,ColorGroup[i].SliderId,TBM_GETPOS,0,0));

		if(GetDlgItemInt(m_hDlg,ColorGroup[i].EditId,NULL,FALSE) != ColorComponent)
		{
			SetDlgItemInt(m_hDlg,ColorGroup[i].EditId,ColorComponent,FALSE);
		}
	}
}

COLORREF CDisplayColoursDialog::GetColorFromSliderGroup(ColorGroup_t ColorGroup[NUM_COLORS])
{
	UINT r = 0;
	UINT g = 0;
	UINT b = 0;

	for(int i = 0;i < NUM_COLORS;i++)
	{
		UINT ColorComponent = static_cast<UINT>(SendDlgItemMessage(m_hDlg,ColorGroup[i].SliderId,TBM_GETPOS,0,0));

		switch(ColorGroup[i].Color)
		{
		case COLOR_RED:
			r = ColorComponent;
			break;

		case COLOR_GREEN:
			g = ColorComponent;
			break;

		case COLOR_BLUE:
			b = ColorComponent;
			break;

		default:
			assert(false);
			break;
		}
	}

	return RGB(r,g,b);
}

void CDisplayColoursDialog::OnEnChange(UINT ControlID)
{
	BOOL Translated;
	UINT Value = GetDlgItemInt(m_hDlg,ControlID,&Translated,FALSE);

	if(!Translated)
	{
		return;
	}

	HWND hTrackBar = NULL;

	switch(ControlID)
	{
	case IDC_EDIT_CENTRE_RED:
		hTrackBar = GetDlgItem(m_hDlg,IDC_SLIDER_CENTRE_RED);
		break;

	case IDC_EDIT_CENTRE_GREEN:
		hTrackBar = GetDlgItem(m_hDlg,IDC_SLIDER_CENTRE_GREEN);
		break;

	case IDC_EDIT_CENTRE_BLUE:
		hTrackBar = GetDlgItem(m_hDlg,IDC_SLIDER_CENTRE_BLUE);
		break;

	case IDC_EDIT_SURROUND_RED:
		hTrackBar = GetDlgItem(m_hDlg,IDC_SLIDER_SURROUND_RED);
		break;

	case IDC_EDIT_SURROUND_GREEN:
		hTrackBar = GetDlgItem(m_hDlg,IDC_SLIDER_SURROUND_GREEN);
		break;

	case IDC_EDIT_SURROUND_BLUE:
		hTrackBar = GetDlgItem(m_hDlg,IDC_SLIDER_SURROUND_BLUE);
		break;
	}

	SendMessage(hTrackBar,TBM_SETPOS,TRUE,Value);

	if(ControlID == IDC_EDIT_CENTRE_RED ||
		ControlID == IDC_EDIT_CENTRE_GREEN ||
		ControlID == IDC_EDIT_CENTRE_BLUE)
	{
		COLORREF Color = GetColorFromSliderGroup(m_CenterGroup);
		SendMessage(m_hPreviewDisplayWindow,DWM_SETCENTRECOLOR,Color,0);
	}
	else if(ControlID == IDC_EDIT_SURROUND_RED ||
		ControlID == IDC_EDIT_SURROUND_GREEN ||
		ControlID == IDC_EDIT_SURROUND_BLUE)
	{
		COLORREF Color = GetColorFromSliderGroup(m_SurroundingGroup);
		SendMessage(m_hPreviewDisplayWindow,DWM_SETSURROUNDCOLOR,Color,0);
	}
}

void CDisplayColoursDialog::OnOk()
{
	COLORREF CenterColor = GetColorFromSliderGroup(m_CenterGroup);
	SendMessage(m_hDisplayWindow,DWM_SETCENTRECOLOR,CenterColor,0);

	COLORREF SurroundingColor = GetColorFromSliderGroup(m_SurroundingGroup);
	SendMessage(m_hDisplayWindow,DWM_SETSURROUNDCOLOR,SurroundingColor,0);

	DisplayWindow_SetFont(m_hDisplayWindow,reinterpret_cast<WPARAM>(m_hDisplayFont));
	DisplayWindow_SetTextColor(m_hDisplayWindow,m_TextColor);

	EndDialog(m_hDlg,1);
}

void CDisplayColoursDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

INT_PTR CDisplayColoursDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

INT_PTR CDisplayColoursDialog::OnDestroy()
{
	DestroyIcon(m_hDisplayWindowIcon);
	return 0;
}

void CDisplayColoursDialog::SaveState()
{
	m_pdcdps->SaveDialogPosition(m_hDlg);

	m_pdcdps->m_bStateSaved = TRUE;
}

CDisplayColoursDialogPersistentSettings::CDisplayColoursDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	
}

CDisplayColoursDialogPersistentSettings& CDisplayColoursDialogPersistentSettings::GetInstance()
{
	static CDisplayColoursDialogPersistentSettings dcdps;
	return dcdps;
}