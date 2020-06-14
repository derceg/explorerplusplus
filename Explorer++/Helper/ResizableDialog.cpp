// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Updates the size/position of a set of controls within
 * a resizable dialog.
 */

#include "stdafx.h"
#include "ResizableDialog.h"

ResizableDialog::ResizableDialog(HWND hDlg, const std::list<Control_t> &controlList) : m_hDlg(hDlg)
{
	ControlInternal_t controlInternal;
	HWND hwnd;
	RECT rcDlg;
	RECT rc;

	GetClientRect(m_hDlg, &rcDlg);

	/* Loop through each of the controls and
	find the delta's. */
	for (const auto &control : controlList)
	{
		controlInternal.iID = control.iID;
		controlInternal.Type = control.Type;
		controlInternal.Constraint = control.Constraint;

		hwnd = GetDlgItem(m_hDlg, control.iID);
		GetWindowRect(hwnd, &rc);
		MapWindowPoints(
			HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&rc), sizeof(RECT) / sizeof(POINT));

		switch (control.Type)
		{
		case ControlType::Move:
			controlInternal.iWidthDelta = rcDlg.right - rc.left;
			controlInternal.iHeightDelta = rcDlg.bottom - rc.top;
			break;

		case ControlType::Resize:
			controlInternal.iWidthDelta = rcDlg.right - rc.right;
			controlInternal.iHeightDelta = rcDlg.bottom - rc.bottom;
			break;
		}

		m_ControlList.push_back(controlInternal);
	}
}

void ResizableDialog::UpdateControls(int iWidth, int iHeight)
{
	HWND hCtrl;
	RECT rc;

	for (const auto &control : m_ControlList)
	{
		hCtrl = GetDlgItem(m_hDlg, control.iID);
		GetWindowRect(hCtrl, &rc);
		MapWindowPoints(
			HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&rc), sizeof(RECT) / sizeof(POINT));

		/* Update the size/position of each of the controls.
		Both resizes and movements rely on the fact that two
		of the edges on a control will always be at a fixed
		distance from the right/bottom edges of the dialog. */
		switch (control.Type)
		{
		case ControlType::Move:
			switch (control.Constraint)
			{
			case ControlConstraint::None:
				SetWindowPos(hCtrl, NULL, iWidth - control.iWidthDelta,
					iHeight - control.iHeightDelta, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				break;

			case ControlConstraint::X:
				SetWindowPos(hCtrl, NULL, iWidth - control.iWidthDelta, rc.top, 0, 0,
					SWP_NOSIZE | SWP_NOZORDER);
				break;

			case ControlConstraint::Y:
				SetWindowPos(hCtrl, NULL, rc.left, iHeight - control.iHeightDelta, 0, 0,
					SWP_NOSIZE | SWP_NOZORDER);
				break;
			}
			break;

		case ControlType::Resize:
			switch (control.Constraint)
			{
			case ControlConstraint::None:
				SetWindowPos(hCtrl, NULL, 0, 0, iWidth - control.iWidthDelta - rc.left,
					iHeight - control.iHeightDelta - rc.top, SWP_NOMOVE | SWP_NOZORDER);
				break;

			case ControlConstraint::X:
				SetWindowPos(hCtrl, NULL, 0, 0, iWidth - control.iWidthDelta - rc.left,
					rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
				break;

			case ControlConstraint::Y:
				SetWindowPos(hCtrl, NULL, 0, 0, rc.right - rc.left,
					iHeight - control.iHeightDelta - rc.top, SWP_NOMOVE | SWP_NOZORDER);
				break;
			}
			break;
		}

		/* Force group boxes to redraw. */
		if (GetWindowStyle(hCtrl) & BS_GROUPBOX)
		{
			InvalidateRect(hCtrl, NULL, TRUE);
		}
	}
}