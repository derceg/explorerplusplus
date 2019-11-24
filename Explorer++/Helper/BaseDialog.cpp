// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include <unordered_map>
#include "WindowHelper.h"
#include "BaseDialog.h"
#include "Helper.h"


namespace
{
	std::unordered_map<HWND,CBaseDialog *>	g_WindowMap;
}

INT_PTR CALLBACK BaseDialogProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			/* Store a mapping between window handles
			and objects. This must be done, as each
			dialog is managed by a separate object,
			but all window calls come through this
			function.
			Since two or more dialogs may be
			shown at once (as a dialog can be
			modeless), this function needs to be able
			to send the specified messages to the
			correct object.
			May also use thunks - see
			http://www.hackcraft.net/cpp/windowsThunk/ */
			g_WindowMap.insert(std::unordered_map<HWND,CBaseDialog *>::
				value_type(hDlg,reinterpret_cast<CBaseDialog *>(lParam)));
		}
		break;
	}

	auto itr = g_WindowMap.find(hDlg);

	if(itr != g_WindowMap.end())
	{
		return itr->second->BaseDialogProc(hDlg,uMsg,wParam,lParam);
	}

	return 0;
}

INT_PTR CALLBACK CBaseDialog::BaseDialogProc(HWND hDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			m_hDlg = hDlg;

			if (m_bResizable)
			{
				RECT rcMain;
				GetWindowRect(m_hDlg, &rcMain);

				/* Assume that the current width and height of
				the dialog are the minimum width and height.
				Note that at this point, the dialog has NOT
				been initialized in any way, so it will not
				have had a chance to be resized yet. */
				m_iMinWidth = GetRectWidth(&rcMain);
				m_iMinHeight = GetRectHeight(&rcMain);

				std::list<CResizableDialog::Control_t> ControlList;
				m_dsc = DIALOG_SIZE_CONSTRAINT_NONE;
				GetResizableControlInformation(m_dsc, ControlList);

				m_prd = std::unique_ptr<CResizableDialog>(new CResizableDialog(m_hDlg, ControlList));
			}

			UINT dpi = m_dpiCompat.GetDpiForWindow(m_hDlg);
			int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
			int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
			m_icon = GetDialogIcon(iconWidth, iconHeight);

			if (m_icon)
			{
				SetClassLongPtr(m_hDlg, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_icon.get()));
			}
		}
		break;

	case WM_GETMINMAXINFO:
		if(m_bResizable)
		{
			LPMINMAXINFO pmmi = reinterpret_cast<LPMINMAXINFO>(lParam);

			pmmi->ptMinTrackSize.x = m_iMinWidth;
			pmmi->ptMinTrackSize.y = m_iMinHeight;

			if(m_dsc == DIALOG_SIZE_CONSTRAINT_X)
			{
				pmmi->ptMaxTrackSize.y = m_iMinHeight;
			}

			if(m_dsc == DIALOG_SIZE_CONSTRAINT_Y)
			{
				pmmi->ptMaxTrackSize.x = m_iMinWidth;
			}

			return 0;
		}
		break;

	case WM_SIZE:
		if(m_bResizable)
		{
			m_prd->UpdateControls(LOWORD(lParam),HIWORD(lParam));
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			/* If this is a modeless dialog, notify the
			caller that the dialog is been destroyed. */
			if(m_bShowingModelessDialog)
			{
				if(m_pmdn != NULL)
				{
					m_pmdn->OnModelessDialogDestroy(m_iResource);
					m_pmdn->Release();
				}
			}

			/* Within WM_DESTROY, all child windows
			still exist. */
			SaveState();
		}
		break;

	case WM_NCDESTROY:
		g_WindowMap.erase(g_WindowMap.find(hDlg));
		break;
	}

	return ForwardMessage(hDlg,uMsg,wParam,lParam);
}

wil::unique_hicon CBaseDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	UNREFERENCED_PARAMETER(iconWidth);
	UNREFERENCED_PARAMETER(iconHeight);

	return nullptr;
}

INT_PTR CBaseDialog::GetDefaultReturnValue(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return 0;
}

CBaseDialog::CBaseDialog(HINSTANCE hInstance,int iResource,
	HWND hParent,bool bResizable) :
CMessageForwarder(),
m_hInstance(hInstance),
m_iResource(iResource),
m_hParent(hParent),
m_bResizable(bResizable)
{
	m_prd = NULL;
	m_bShowingModelessDialog = FALSE;
}

HINSTANCE CBaseDialog::GetInstance() const
{
	return m_hInstance;
}

INT_PTR CBaseDialog::ShowModalDialog()
{
	/* Explicitly disallow the creation of another
	dialog from this object while a modeless dialog
	is been shown. */
	if(m_bShowingModelessDialog)
	{
		return -1;
	}

	return DialogBoxParam(m_hInstance,MAKEINTRESOURCE(m_iResource),
		m_hParent,BaseDialogProcStub,reinterpret_cast<LPARAM>(this));
}

HWND CBaseDialog::ShowModelessDialog(IModelessDialogNotification *pmdn)
{
	if(m_bShowingModelessDialog)
	{
		return NULL;
	}

	HWND hDlg = CreateDialogParam(m_hInstance,
		MAKEINTRESOURCE(m_iResource),m_hParent,
		BaseDialogProcStub,
		reinterpret_cast<LPARAM>(this));

	if(hDlg != NULL)
	{
		m_bShowingModelessDialog = TRUE;
	}

	m_pmdn = pmdn;

	return hDlg;
}

void CBaseDialog::GetResizableControlInformation(DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	UNREFERENCED_PARAMETER(dsc);
	UNREFERENCED_PARAMETER(ControlList);
}

void CBaseDialog::SaveState()
{

}