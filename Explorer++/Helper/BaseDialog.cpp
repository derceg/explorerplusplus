/******************************************************************
 *
 * Project: Helper
 * File: BaseDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides a degree of abstraction off a standard dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <unordered_map>
#include "BaseDialog.h"
#include "Helper.h"


namespace NBaseDialog
{
	INT_PTR CALLBACK	BaseDialogProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	std::tr1::unordered_map<HWND,CBaseDialog *>	g_WindowMap;
}

INT_PTR CALLBACK NBaseDialog::BaseDialogProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
			NBaseDialog::g_WindowMap.insert(std::tr1::unordered_map<HWND,CBaseDialog *>::
				value_type(hDlg,reinterpret_cast<CBaseDialog *>(lParam)));
		}
		break;
	}

	auto itr = NBaseDialog::g_WindowMap.find(hDlg);

	if(itr != g_WindowMap.end())
	{
		return itr->second->BaseDialogProc(hDlg,uMsg,wParam,lParam);
	}

	return 0;
}

INT_PTR CALLBACK CBaseDialog::BaseDialogProc(HWND hDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam)
{
	/* Private message? */
	if(uMsg > WM_APP && uMsg < 0xBFFF)
	{
		OnPrivateMessage(uMsg,wParam,lParam);
		return 0;
	}

	switch(uMsg)
	{
		case WM_INITDIALOG:
			m_hDlg = hDlg;

			if(m_bResizable)
			{
				RECT rcMain;
				GetWindowRect(m_hDlg,&rcMain);

				/* Assume that the current width and height of
				the dialog are the minimum width and height.
				Note that at this point, the dialog has NOT
				been initialized in any way, so it will not
				have had a chance to be resized yet. */
				m_iMinWidth = GetRectWidth(&rcMain);
				m_iMinHeight = GetRectHeight(&rcMain);

				std::list<CResizableDialog::Control_t> ControlList;
				m_dsc = DIALOG_SIZE_CONSTRAINT_NONE;
				GetResizableControlInformation(m_dsc,ControlList);

				m_prd = new CResizableDialog(m_hDlg,ControlList);
			}

			return OnInitDialog();
			break;

		case WM_CTLCOLORSTATIC:
			return OnCtlColorStatic(reinterpret_cast<HWND>(lParam),
				reinterpret_cast<HDC>(wParam));
			break;

		case WM_TIMER:
			return OnTimer(static_cast<int>(wParam));
			break;

		case WM_COMMAND:
			return OnCommand(wParam,lParam);
			break;

		case WM_NOTIFY:
			return OnNotify(reinterpret_cast<LPNMHDR>(lParam));
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

			return OnGetMinMaxInfo(reinterpret_cast<LPMINMAXINFO>(lParam));
			break;

		case WM_SIZE:
			if(m_bResizable)
			{
				m_prd->UpdateControls(LOWORD(lParam),HIWORD(lParam));
				return 0;
			}

			return OnSize(static_cast<int>(wParam),
				LOWORD(lParam),HIWORD(lParam));
			break;

		case WM_CLOSE:
			return OnClose();
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
					}
				}

				BOOL bRes = OnDestroy();

				/* Within WM_DESTROY, all child windows
				still exist. */
				SaveState();

				return bRes;
			}
			break;

		case WM_NCDESTROY:
			NBaseDialog::g_WindowMap.erase(
				NBaseDialog::g_WindowMap.find(hDlg));
			return OnNcDestroy();
			break;
	}

	return 0;
}

CBaseDialog::CBaseDialog(HINSTANCE hInstance,int iResource,
	HWND hParent,bool bResizable)
{
	m_hInstance = hInstance;
	m_iResource = iResource;
	m_hParent = hParent;
	m_bResizable = bResizable;

	m_prd = NULL;

	m_bShowingModelessDialog = FALSE;
}

CBaseDialog::~CBaseDialog()
{
	delete m_prd;
}

HINSTANCE CBaseDialog::GetInstance()
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
		m_hParent,NBaseDialog::BaseDialogProcStub,reinterpret_cast<LPARAM>(this));
}

HWND CBaseDialog::ShowModelessDialog(IModelessDialogNotification *pmdn)
{
	if(m_bShowingModelessDialog)
	{
		return NULL;
	}

	HWND hDlg = CreateDialogParam(m_hInstance,
		MAKEINTRESOURCE(m_iResource),m_hParent,
		NBaseDialog::BaseDialogProcStub,
		reinterpret_cast<LPARAM>(this));

	if(hDlg != NULL)
	{
		m_bShowingModelessDialog = TRUE;
	}

	m_pmdn = pmdn;

	return hDlg;
}

BOOL CBaseDialog::OnInitDialog()
{
	return TRUE;
}

BOOL CBaseDialog::OnTimer(int iTimerID)
{
	return 0;
}

INT_PTR CBaseDialog::OnCtlColorStatic(HWND hwnd,HDC hdc)
{
	return 0;
}

BOOL CBaseDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	return 1;
}

BOOL CBaseDialog::OnNotify(NMHDR *pnmhdr)
{
	return 0;
}

BOOL CBaseDialog::OnGetMinMaxInfo(LPMINMAXINFO pmmi)
{
	return 0;
}

BOOL CBaseDialog::OnSize(int iType,int iWidth,int iHeight)
{
	return 0;
}

BOOL CBaseDialog::OnClose()
{
	return 0;
}

BOOL CBaseDialog::OnDestroy()
{
	return 0;
}

BOOL CBaseDialog::OnNcDestroy()
{
	return 0;
}

void CBaseDialog::SaveState()
{

}

void CBaseDialog::OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{

}

void CBaseDialog::GetResizableControlInformation(DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{

}