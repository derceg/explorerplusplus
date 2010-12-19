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
#include "BaseDialog.h"


INT_PTR CALLBACK BaseDialogProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

INT_PTR CALLBACK BaseDialogProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CBaseDialog *pBaseDialog = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pBaseDialog = (CBaseDialog *)lParam;
		}
		break;
	}

	return pBaseDialog->BaseDialogProc(hDlg,uMsg,wParam,lParam);
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

			return OnInitDialog();
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
			return OnGetMinMaxInfo(reinterpret_cast<LPMINMAXINFO>(lParam));
			break;

		case WM_SIZE:
			return OnSize(static_cast<int>(wParam),
				LOWORD(lParam),HIWORD(lParam));
			break;

		case WM_CLOSE:
			return OnClose();
			break;

		case WM_DESTROY:
			return OnDestroy();
			break;

		case WM_NCDESTROY:
			return OnNcDestroy();
			break;
	}

	return 0;
}

CBaseDialog::CBaseDialog(HINSTANCE hInstance,int iResource,HWND hParent)
{
	m_hInstance = hInstance;
	m_iResource = iResource;
	m_hParent = hParent;
}

CBaseDialog::~CBaseDialog()
{

}

HINSTANCE CBaseDialog::GetInstance()
{
	return m_hInstance;
}

void CBaseDialog::ShowModalDialog()
{
	DialogBoxParam(m_hInstance,MAKEINTRESOURCE(m_iResource),
		m_hParent,BaseDialogProcStub,(LPARAM)this);
}

HWND CBaseDialog::ShowModelessDialog()
{
	return CreateDialogParam(m_hInstance,
		MAKEINTRESOURCE(m_iResource),m_hParent,
		BaseDialogProcStub,(LPARAM)this);
}

BOOL CBaseDialog::OnInitDialog()
{
	return TRUE;
}

BOOL CBaseDialog::OnTimer(int iTimerID)
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

void CBaseDialog::OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{

}