/******************************************************************
 *
 * Project: Explorer++
 * File: DisplayColoursDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Change Display Colors' dialog and associated messages.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++.h"

#define NUM_CONTROLS	3

typedef struct
{
	UINT uSliderId;
	UINT uEditId;

	BYTE (*GetColorValue)(DWORD rgb);
} Colors_t;

HWND g_hPreviewDisplay;

Colors_t	CentreColors[3];
Colors_t	SurroundColors[3];
HFONT		g_hDisplayFont;
COLORREF	g_TextColor;
HICON		g_hDisplayWindowIcon;

INT_PTR CALLBACK ChangeDisplayColoursStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (Explorerplusplus *)lParam;
		}
		break;
	}

	return pContainer->ChangeDisplayColours(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::ChangeDisplayColours(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnInitializeDisplayColorsDlg(hDlg);
			break;

		case WM_HSCROLL:
			OnDisplayColorsHScroll(hDlg);
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
			case EN_CHANGE:
				OnDisplayColorsEnChange(hDlg,lParam);
				break;
			}

			switch(LOWORD(wParam))
			{
			case IDC_BUTTON_RESTOREDEFAULTS:
				{
					COLORREF SurroundColor;
					COLORREF CentreColor;

					m_DisplayWindowSurroundColor = DEFAULT_DISPLAYWINDOW_SURROUND_COLOR;
					m_DisplayWindowCentreColor = DEFAULT_DISPLAYWINDOW_CENTRE_COLOR;

					SurroundColor = m_DisplayWindowSurroundColor.ToCOLORREF();
					CentreColor = m_DisplayWindowCentreColor.ToCOLORREF();

					/* TODO: Need to delete old display window fonts. */
					//DeleteFont(m_DisplayWindowFont);
					m_DisplayWindowFont	= CreateFont(-13,0,0,0,FW_MEDIUM,FALSE,
						FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
						CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FIXED_PITCH|FF_MODERN,
						_T("Segoe UI"));
					g_hDisplayFont = m_DisplayWindowFont;

					SendMessage(g_hPreviewDisplay,DWM_SETSURROUNDCOLOR,SurroundColor,0);
					SendMessage(g_hPreviewDisplay,DWM_SETCENTRECOLOR,CentreColor,0);
					DisplayWindow_SetFont(g_hPreviewDisplay,(WPARAM)g_hDisplayFont);
					DisplayWindow_SetTextColor(g_hPreviewDisplay,g_TextColor);

					SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_RED,GetRValue(SurroundColor),FALSE);
					SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_GREEN,GetGValue(SurroundColor),FALSE);
					SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_BLUE,GetBValue(SurroundColor),FALSE);

					SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_RED,TBM_SETPOS,TRUE,
						GetRValue(SurroundColor));
					SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_GREEN,TBM_SETPOS,TRUE,
						GetGValue(SurroundColor));
					SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_BLUE,TBM_SETPOS,TRUE,
						GetBValue(SurroundColor));

					SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_RED,GetRValue(CentreColor),FALSE);
					SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_GREEN,GetGValue(CentreColor),FALSE);
					SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_BLUE,GetBValue(CentreColor),FALSE);

					SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_RED,TBM_SETPOS,TRUE,
						GetRValue(CentreColor));
					SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_GREEN,TBM_SETPOS,TRUE,
						GetGValue(CentreColor));
					SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_BLUE,TBM_SETPOS,TRUE,
						GetBValue(CentreColor));

					SendMessage(g_hPreviewDisplay,DWM_SETSURROUNDCOLOR,SurroundColor,0);
					SendMessage(g_hPreviewDisplay,DWM_SETCENTRECOLOR,CentreColor,0);
					DisplayWindow_SetFont(g_hPreviewDisplay,(WPARAM)g_hDisplayFont);
					DisplayWindow_SetTextColor(g_hPreviewDisplay,g_TextColor);
				}
				break;

			case IDC_BUTTON_DISPLAY_FONT:
				OnDisplayColorsChooseFont(hDlg);
				break;

			case IDOK:
				OnDisplayColorsDlgOk(hDlg);
				break;

			case IDCANCEL:
				DisplayColorsSaveState(hDlg);
				EndDialog(hDlg,0);
				break;
			}
			break;

		case WM_CLOSE:
			DisplayColorsSaveState(hDlg);
			EndDialog(hDlg,0);
			break;

		case WM_DESTROY:
			DestroyIcon(g_hDisplayWindowIcon);
			break;
	}

	return 0;
}

void Explorerplusplus::OnInitializeDisplayColorsDlg(HWND hDlg)
{
	COLORREF CentreColor;
	COLORREF SurroundColor;
	WORD wFreq;
	int i = 0;

	CentreColors[0].uSliderId	= IDC_SLIDER_CENTRE_RED;
	CentreColors[0].uEditId		= IDC_EDIT_CENTRE_RED;

	CentreColors[1].uSliderId	= IDC_SLIDER_CENTRE_GREEN;
	CentreColors[1].uEditId		= IDC_EDIT_CENTRE_GREEN;

	CentreColors[2].uSliderId	= IDC_SLIDER_CENTRE_BLUE;
	CentreColors[2].uEditId		= IDC_EDIT_CENTRE_BLUE;

	SurroundColors[0].uSliderId	= IDC_SLIDER_SURROUND_RED;
	SurroundColors[0].uEditId	= IDC_EDIT_SURROUND_RED;

	SurroundColors[1].uSliderId	= IDC_SLIDER_SURROUND_GREEN;
	SurroundColors[1].uEditId	= IDC_EDIT_SURROUND_GREEN;

	SurroundColors[2].uSliderId	= IDC_SLIDER_SURROUND_BLUE;
	SurroundColors[2].uEditId	= IDC_EDIT_SURROUND_BLUE;

	wFreq = 10;

	for(i = 0;i < NUM_CONTROLS;i++)
	{
		SendDlgItemMessage(hDlg,SurroundColors[i].uSliderId,
			TBM_SETTICFREQ,wFreq,0);

		SendDlgItemMessage(hDlg,SurroundColors[i].uSliderId,
			TBM_SETRANGE,TRUE,MAKELONG(0,255));

		SendDlgItemMessage(hDlg,SurroundColors[i].uEditId,
			EM_SETLIMITTEXT,3,0);
	}

	for(i = 0;i < NUM_CONTROLS;i++)
	{
		SendDlgItemMessage(hDlg,CentreColors[i].uSliderId,
		TBM_SETTICFREQ,wFreq,0);

		SendDlgItemMessage(hDlg,CentreColors[i].uSliderId,
		TBM_SETRANGE,TRUE,MAKELONG(0,255));

		SendDlgItemMessage(hDlg,CentreColors[i].uEditId,
			EM_SETLIMITTEXT,3,0);
	}

	SurroundColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETSURROUNDCOLOR,0,0);
	CentreColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETCENTRECOLOR,0,0);

	DisplayWindow_GetFont(m_hDisplayWindow,(WPARAM)&g_hDisplayFont);
	g_TextColor = DisplayWindow_GetTextColor(m_hDisplayWindow);

	HWND hStatic;
	RECT rc;

	hStatic = GetDlgItem(hDlg,IDC_STATIC_PREVIEWDISPLAY);

	DWInitialSettings_t InitialSettings;

	g_hDisplayWindowIcon = (HICON)LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW),IMAGE_ICON,
		0,0,LR_CREATEDIBSECTION);

	InitialSettings.CentreColor		= m_DisplayWindowCentreColor;
	InitialSettings.SurroundColor	= m_DisplayWindowSurroundColor;
	InitialSettings.TextColor		= m_DisplayWindowTextColor;
	InitialSettings.hFont			= m_DisplayWindowFont;
	InitialSettings.hIcon			= g_hDisplayWindowIcon;

	g_hPreviewDisplay = CreateDisplayWindow(hStatic,&m_pDisplayMain,&InitialSettings);




	SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_RED,GetRValue(SurroundColor),FALSE);
	SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_GREEN,GetGValue(SurroundColor),FALSE);
	SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_BLUE,GetBValue(SurroundColor),FALSE);

	SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_RED,TBM_SETPOS,TRUE,
	GetRValue(SurroundColor));
	SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_GREEN,TBM_SETPOS,TRUE,
	GetGValue(SurroundColor));
	SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_BLUE,TBM_SETPOS,TRUE,
	GetBValue(SurroundColor));

	SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_RED,GetRValue(CentreColor),FALSE);
	SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_GREEN,GetGValue(CentreColor),FALSE);
	SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_BLUE,GetBValue(CentreColor),FALSE);

	SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_RED,TBM_SETPOS,TRUE,
	GetRValue(CentreColor));
	SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_GREEN,TBM_SETPOS,TRUE,
	GetGValue(CentreColor));
	SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_BLUE,TBM_SETPOS,TRUE,
	GetBValue(CentreColor));

	SendMessage(g_hPreviewDisplay,DWM_SETSURROUNDCOLOR,SurroundColor,0);
	SendMessage(g_hPreviewDisplay,DWM_SETCENTRECOLOR,CentreColor,0);
	DisplayWindow_SetFont(g_hPreviewDisplay,(WPARAM)g_hDisplayFont);
	DisplayWindow_SetTextColor(g_hPreviewDisplay,g_TextColor);




	DisplayWindow_ClearTextBuffer(g_hPreviewDisplay);
	DisplayWindow_BufferText(g_hPreviewDisplay,_T("Filename"));
	DisplayWindow_BufferText(g_hPreviewDisplay,_T("File Type"));
	DisplayWindow_BufferText(g_hPreviewDisplay,_T("Modification Date"));

	GetWindowRect(hStatic,&rc);

	SetWindowPos(g_hPreviewDisplay,NULL,0,0,rc.right,rc.bottom,SWP_SHOWWINDOW|SWP_NOZORDER);

	if(m_bDisplayColorsDlgStateSaved)
	{		
		SetWindowPos(hDlg,NULL,m_ptDisplayColors.x,m_ptDisplayColors.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}
}

void Explorerplusplus::OnDisplayColorsDlgOk(HWND hDlg)
{
	UINT r;
	UINT g;
	UINT b;
	COLORREF rgb;

	r = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_RED,TBM_GETPOS,0,0);
	g = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_GREEN,TBM_GETPOS,0,0);
	b = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_BLUE,TBM_GETPOS,0,0);

	rgb = RGB(r,g,b);

	SendMessage(m_hDisplayWindow,DWM_SETSURROUNDCOLOR,rgb,0);

	r = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_RED,TBM_GETPOS,0,0);
	g = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_GREEN,TBM_GETPOS,0,0);
	b = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_BLUE,TBM_GETPOS,0,0);

	rgb = RGB(r,g,b);

	SendMessage(m_hDisplayWindow,DWM_SETCENTRECOLOR,rgb,0);

	DisplayWindow_SetFont(m_hDisplayWindow,(WPARAM)g_hDisplayFont);
	DisplayWindow_SetTextColor(m_hDisplayWindow,g_TextColor);

	DisplayColorsSaveState(hDlg);
	EndDialog(hDlg,1);
}

void Explorerplusplus::OnDisplayColorsChooseFont(HWND hDlg)
{
	CHOOSEFONT cf;
	LOGFONT LogFont;
	HFONT hFont;
	TCHAR szStyle[512];

	DisplayWindow_GetFont(g_hPreviewDisplay,(WPARAM)&hFont);

	GetObject(hFont,sizeof(LogFont),(LPVOID)&LogFont);

	cf.lStructSize	= sizeof(cf);
	cf.hwndOwner	= hDlg;
	cf.Flags		= CF_FORCEFONTEXIST|CF_SCREENFONTS|CF_EFFECTS|CF_INITTOLOGFONTSTRUCT;
	cf.lpLogFont	= &LogFont;
	cf.rgbColors	= DisplayWindow_GetTextColor(g_hPreviewDisplay);
	cf.lCustData	= NULL;
	cf.lpszStyle	= szStyle;

	ChooseFont(&cf);

	g_hDisplayFont = CreateFontIndirect(cf.lpLogFont);
	g_TextColor = cf.rgbColors;

	DisplayWindow_SetFont(g_hPreviewDisplay,(WPARAM)g_hDisplayFont);
	DisplayWindow_SetTextColor(g_hPreviewDisplay,g_TextColor);
}

void Explorerplusplus::OnDisplayColorsHScroll(HWND hDlg)
{
	UINT r;
	UINT g;
	UINT b;

	r = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_RED,TBM_GETPOS,0,0);
	g = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_GREEN,TBM_GETPOS,0,0);
	b = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_BLUE,TBM_GETPOS,0,0);

	if(GetDlgItemInt(hDlg,IDC_EDIT_SURROUND_RED,NULL,FALSE) != r)
	{
		SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_RED,r,FALSE);
	}

	if(GetDlgItemInt(hDlg,IDC_EDIT_SURROUND_GREEN,NULL,FALSE) != g)
	{
		SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_GREEN,g,FALSE);
	}

	if(GetDlgItemInt(hDlg,IDC_EDIT_SURROUND_BLUE,NULL,FALSE) != b)
	{
		SetDlgItemInt(hDlg,IDC_EDIT_SURROUND_BLUE,b,FALSE);
	}

	r = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_RED,TBM_GETPOS,0,0);
	g = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_GREEN,TBM_GETPOS,0,0);
	b = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_BLUE,TBM_GETPOS,0,0);

	if(GetDlgItemInt(hDlg,IDC_EDIT_CENTRE_RED,NULL,FALSE) != r)
	{
		SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_RED,r,FALSE);
	}

	if(GetDlgItemInt(hDlg,IDC_EDIT_CENTRE_GREEN,NULL,FALSE) != g)
	{
		SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_GREEN,g,FALSE);
	}

	if(GetDlgItemInt(hDlg,IDC_EDIT_CENTRE_BLUE,NULL,FALSE) != b)
	{
		SetDlgItemInt(hDlg,IDC_EDIT_CENTRE_BLUE,b,FALSE);
	}
}

void Explorerplusplus::OnDisplayColorsEnChange(HWND hDlg,LPARAM lParam)
{
	HWND hEdit;
	HWND hTrackBar = NULL;
	COLORREF rgb;
	TCHAR szValue[32];
	int iValue;

	hEdit = (HWND)lParam;

	GetWindowText(hEdit,szValue,SIZEOF_ARRAY(szValue));

	#ifdef UNICODE
		iValue = _wtoi(szValue);
	#else
		iValue = atoi(szValue);
	#endif

	if(hEdit == GetDlgItem(hDlg,IDC_EDIT_SURROUND_RED))
		hTrackBar = GetDlgItem(hDlg,IDC_SLIDER_SURROUND_RED);
	else if(hEdit == GetDlgItem(hDlg,IDC_EDIT_SURROUND_GREEN))
		hTrackBar = GetDlgItem(hDlg,IDC_SLIDER_SURROUND_GREEN);
	else if(hEdit == GetDlgItem(hDlg,IDC_EDIT_SURROUND_BLUE))
		hTrackBar = GetDlgItem(hDlg,IDC_SLIDER_SURROUND_BLUE);
	else if(hEdit == GetDlgItem(hDlg,IDC_EDIT_CENTRE_RED))
		hTrackBar = GetDlgItem(hDlg,IDC_SLIDER_CENTRE_RED);
	else if(hEdit == GetDlgItem(hDlg,IDC_EDIT_CENTRE_GREEN))
		hTrackBar = GetDlgItem(hDlg,IDC_SLIDER_CENTRE_GREEN);
	else if(hEdit == GetDlgItem(hDlg,IDC_EDIT_CENTRE_BLUE))
		hTrackBar = GetDlgItem(hDlg,IDC_SLIDER_CENTRE_BLUE);

	SendMessage(hTrackBar,TBM_SETPOS,TRUE,iValue);

	UINT r;
	UINT g;
	UINT b;

	r = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_RED,TBM_GETPOS,0,0);
	g = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_GREEN,TBM_GETPOS,0,0);
	b = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_SURROUND_BLUE,TBM_GETPOS,0,0);

	rgb = RGB(r,g,b);

	SendMessage(g_hPreviewDisplay,DWM_SETSURROUNDCOLOR,rgb,0);

	r = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_RED,TBM_GETPOS,0,0);
	g = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_GREEN,TBM_GETPOS,0,0);
	b = (UINT)SendDlgItemMessage(hDlg,IDC_SLIDER_CENTRE_BLUE,TBM_GETPOS,0,0);

	rgb = RGB(r,g,b);

	SendMessage(g_hPreviewDisplay,DWM_SETCENTRECOLOR,rgb,0);
}

void Explorerplusplus::DisplayColorsSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptDisplayColors.x = rcTemp.left;
	m_ptDisplayColors.y = rcTemp.top;

	m_bDisplayColorsDlgStateSaved = TRUE;
}