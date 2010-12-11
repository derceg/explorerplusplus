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

CBaseDialog::CBaseDialog(HINSTANCE hInstance,int iResource,HWND hParent)
{
	m_hInstance = hInstance;
	m_iResource = iResource;
	m_hParent = hParent;
}

CBaseDialog::~CBaseDialog()
{

}

void CBaseDialog::ShowDialog()
{
	DialogBoxParam(m_hInstance,MAKEINTRESOURCE(m_iResource),
		m_hParent,BaseDialogProcStub,(LPARAM)this);
}

BOOL CBaseDialog::OnInitDialog()
{
	return TRUE;
}

BOOL CBaseDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	return 1;
}

BOOL CBaseDialog::OnNotify(NMHDR *pnmhdr)
{
	return 0;
}

BOOL CBaseDialog::OnClose()
{
	return 0;
}

INT_PTR CALLBACK CBaseDialog::BaseDialogProc(HWND hDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			m_hDlg = hDlg;

			return OnInitDialog();
			break;

		case WM_COMMAND:
			return OnCommand(wParam,lParam);
			break;

		case WM_NOTIFY:
			return OnNotify(reinterpret_cast<LPNMHDR>(lParam));
			break;

		case WM_CLOSE:
			return OnClose();
			break;
	}

	return 0;
}