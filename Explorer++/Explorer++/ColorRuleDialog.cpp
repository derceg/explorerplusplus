/******************************************************************
 *
 * Project: Explorer++
 * File: ColorRuleDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the new/edit color rule dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"


LRESULT CALLBACK StaticColorProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

COLORREF g_rgbColor;
BOOL g_bEditing;

INT_PTR CALLBACK ColorRuleProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->ColorRuleProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::ColorRuleProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			{
				HWND hStaticColor;

				SetDlgItemText(hDlg,IDC_EDIT_DESCRIPTION,m_pColoringItem->szDescription);
				SetDlgItemText(hDlg,IDC_EDIT_FILENAMEPATTERN,m_pColoringItem->szFilterPattern);

				g_rgbColor = m_pColoringItem->rgbColour;

				if(m_pColoringItem->dwFilterAttributes & FILE_ATTRIBUTE_COMPRESSED)
					CheckDlgButton(hDlg,IDC_CHECK_COMPRESSED,BST_CHECKED);

				if(m_pColoringItem->dwFilterAttributes & FILE_ATTRIBUTE_ENCRYPTED)
					CheckDlgButton(hDlg,IDC_CHECK_ENCRYPTED,BST_CHECKED);

				if(m_pColoringItem->dwFilterAttributes & FILE_ATTRIBUTE_ARCHIVE)
					CheckDlgButton(hDlg,IDC_CHECK_ARCHIVE,BST_CHECKED);

				if(m_pColoringItem->dwFilterAttributes & FILE_ATTRIBUTE_HIDDEN)
					CheckDlgButton(hDlg,IDC_CHECK_HIDDEN,BST_CHECKED);

				if(m_pColoringItem->dwFilterAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
					CheckDlgButton(hDlg,IDC_CHECK_INDEXED,BST_CHECKED);

				if(m_pColoringItem->dwFilterAttributes & FILE_ATTRIBUTE_READONLY)
					CheckDlgButton(hDlg,IDC_CHECK_READONLY,BST_CHECKED);

				if(m_pColoringItem->dwFilterAttributes & FILE_ATTRIBUTE_SYSTEM)
					CheckDlgButton(hDlg,IDC_CHECK_SYSTEM,BST_CHECKED);

				hStaticColor = GetDlgItem(hDlg,IDC_STATIC_COLOR);

				SetWindowSubclass(hStaticColor,StaticColorProcStub,0,(DWORD_PTR)this);

				if(g_bEditing)
				{
					TCHAR szTemp[64];

					LoadString(g_hLanguageModule,IDS_EDITCOLORRULE,
						szTemp,SIZEOF_ARRAY(szTemp));
					SetWindowText(hDlg,szTemp);
				}

				SetFocus(GetDlgItem(hDlg,IDC_EDIT_DESCRIPTION));

				CenterWindow(m_hContainer,hDlg);
			}
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
				case STN_DBLCLK:
					ColorRuleChooseNewColor(hDlg);
					break;
			}

			switch(LOWORD(wParam))
			{
			case IDC_BUTTON_CHANGECOLOR:
				ColorRuleChooseNewColor(hDlg);
				break;

			case IDOK:
				{
					GetWindowText(GetDlgItem(hDlg,IDC_EDIT_DESCRIPTION),
						m_pColoringItem->szDescription,
						SIZEOF_ARRAY(m_pColoringItem->szDescription));

					GetWindowText(GetDlgItem(hDlg,IDC_EDIT_FILENAMEFILTER),
						m_pColoringItem->szFilterPattern,
						SIZEOF_ARRAY(m_pColoringItem->szFilterPattern));

					m_pColoringItem->rgbColour = g_rgbColor;

					if(IsDlgButtonChecked(hDlg,IDC_CHECK_COMPRESSED) == BST_CHECKED)
						m_pColoringItem->dwFilterAttributes |= FILE_ATTRIBUTE_COMPRESSED;

					if(IsDlgButtonChecked(hDlg,IDC_CHECK_ENCRYPTED) == BST_CHECKED)
						m_pColoringItem->dwFilterAttributes |= FILE_ATTRIBUTE_ENCRYPTED;

					if(IsDlgButtonChecked(hDlg,IDC_CHECK_ARCHIVE) == BST_CHECKED)
						m_pColoringItem->dwFilterAttributes |= FILE_ATTRIBUTE_ARCHIVE;

					if(IsDlgButtonChecked(hDlg,IDC_CHECK_HIDDEN) == BST_CHECKED)
						m_pColoringItem->dwFilterAttributes |= FILE_ATTRIBUTE_HIDDEN;

					if(IsDlgButtonChecked(hDlg,IDC_CHECK_READONLY) == BST_CHECKED)
						m_pColoringItem->dwFilterAttributes |= FILE_ATTRIBUTE_READONLY;

					if(IsDlgButtonChecked(hDlg,IDC_CHECK_SYSTEM) == BST_CHECKED)
						m_pColoringItem->dwFilterAttributes |= FILE_ATTRIBUTE_SYSTEM;

					EndDialog(hDlg,1);
				}
				break;

			case IDCANCEL:
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::ColorRuleChooseNewColor(HWND hDlg)
{
	CHOOSECOLOR cc;
	BOOL bRet;

	cc.lStructSize	= sizeof(cc);
	cc.hwndOwner	= hDlg;
	cc.rgbResult	= g_rgbColor;
	cc.lpCustColors	= m_ccCustomColors;
	cc.Flags		= CC_RGBINIT;

	bRet = ChooseColor(&cc);

	if(bRet)
	{
		g_rgbColor = cc.rgbResult;

		/* If this is a new item been created, store the color
		regardless of whether the item is actually created or
		not. */
		if(!g_bEditing)
			m_pColoringItem->rgbColour = cc.rgbResult;

		InvalidateRect(GetDlgItem(hDlg,IDC_STATIC_COLOR),NULL,TRUE);
	}
}

LRESULT CALLBACK StaticColorProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CContainer *pContainer = (CContainer *)dwRefData;

	return pContainer->StaticColorProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CContainer::StaticColorProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_ERASEBKGND:
		{
			HDC hdc;
			RECT rc;
			HBRUSH hBrush;

			hdc = (HDC)wParam;

			GetClientRect(hwnd,&rc);

			hBrush = CreateSolidBrush(g_rgbColor);

			FillRect(hdc,&rc,hBrush);

			DeleteObject(hBrush);

			return 1;
		}
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}