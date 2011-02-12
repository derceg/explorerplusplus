/******************************************************************
 *
 * Project: Helper
 * File: ResizableDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Updates the size/position of a set of controls within
 * a resizable dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "ResizableDialog.h"


CResizableDialog::CResizableDialog(HWND hDlg,
	const std::list<Control_t> &ControlList)
{
	m_hDlg = hDlg;

	ControlInternal_t ControlInternal;
	HWND hwnd;
	RECT rcDlg;
	RECT rc;

	GetClientRect(m_hDlg,&rcDlg);

	/* Loop through each of the controls and
	find the delta's. */
	for each(auto Control in ControlList)
	{
		ControlInternal.iID			= Control.iID;
		ControlInternal.Type		= Control.Type;
		ControlInternal.Constraint	= Control.Constraint;

		hwnd = GetDlgItem(m_hDlg,Control.iID);
		GetWindowRect(hwnd,&rc);
		MapWindowPoints(HWND_DESKTOP,m_hDlg,reinterpret_cast<LPPOINT>(&rc),sizeof(RECT) / sizeof(POINT));

		switch(Control.Type)
		{
		case TYPE_MOVE:
			ControlInternal.iWidthDelta = rcDlg.right - rc.left;
			ControlInternal.iHeightDelta = rcDlg.bottom - rc.top;
			break;

		case TYPE_RESIZE:
			ControlInternal.iWidthDelta = rcDlg.right - rc.right;
			ControlInternal.iHeightDelta = rcDlg.bottom - rc.bottom;
			break;
		}

		m_ControlList.push_back(ControlInternal);
	}
}

CResizableDialog::~CResizableDialog()
{

}

void CResizableDialog::UpdateControls(int iWidth,int iHeight)
{
	HWND hCtrl;
	RECT rc;

	for each(auto Control in m_ControlList)
	{
		hCtrl = GetDlgItem(m_hDlg,Control.iID);
		GetWindowRect(hCtrl,&rc);
		MapWindowPoints(HWND_DESKTOP,m_hDlg,reinterpret_cast<LPPOINT>(&rc),sizeof(RECT) / sizeof(POINT));

		/* Update the size/position of each of the controls.
		Both resizes and movements rely on the fact that two
		of the edges on a control will always be at a fixed
		distance from the right/bottom edges of the dialog. */
		switch(Control.Type)
		{
		case TYPE_MOVE:
			switch(Control.Constraint)
			{
			case CONSTRAINT_NONE:
				SetWindowPos(hCtrl,NULL,iWidth - Control.iWidthDelta,iHeight - Control.iHeightDelta,
					0,0,SWP_NOSIZE|SWP_NOZORDER);
				break;

			case CONSTRAINT_X:
				SetWindowPos(hCtrl,NULL,iWidth - Control.iWidthDelta,rc.top,
					0,0,SWP_NOSIZE|SWP_NOZORDER);
				break;

			case CONSTRAINT_Y:
				SetWindowPos(hCtrl,NULL,rc.left,iHeight - Control.iHeightDelta,
					0,0,SWP_NOSIZE|SWP_NOZORDER);
				break;
			}
			break;

		case TYPE_RESIZE:
			switch(Control.Constraint)
			{
			case CONSTRAINT_NONE:
				SetWindowPos(hCtrl,NULL,0,0,iWidth - Control.iWidthDelta - rc.left,
					iHeight - Control.iHeightDelta - rc.top,SWP_NOMOVE|SWP_NOZORDER);
				break;

			case CONSTRAINT_X:
				SetWindowPos(hCtrl,NULL,0,0,iWidth - Control.iWidthDelta - rc.left,
					rc.bottom - rc.top,SWP_NOMOVE|SWP_NOZORDER);
				break;

			case CONSTRAINT_Y:
				SetWindowPos(hCtrl,NULL,0,0,rc.right - rc.left,
					iHeight - Control.iHeightDelta - rc.top,SWP_NOMOVE|SWP_NOZORDER);
				break;
			}
			break;
		}

		/* Force group boxes to redraw. */
		if(GetWindowStyle(hCtrl) & BS_GROUPBOX)
		{
			InvalidateRect(hCtrl,NULL,TRUE);
		}
	}
}