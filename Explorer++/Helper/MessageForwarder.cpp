// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MessageForwarder.h"

INT_PTR MessageForwarder::ForwardMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	m_hwnd = hwnd;
	m_uMsg = uMsg;
	m_wParam = wParam;
	m_lParam = lParam;

	/* Private message? */
	if (uMsg > WM_APP && uMsg < 0xBFFF)
	{
		return OnPrivateMessage(uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInitDialog();
		break;

	case WM_CTLCOLORDLG:
		return OnCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));
		break;

	case WM_CTLCOLORSTATIC:
		return OnCtlColorStatic(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));
		break;

	case WM_CTLCOLOREDIT:
		return OnCtlColorEdit(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));
		break;

	case WM_CTLCOLORLISTBOX:
		return OnCtlColorListBox(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));
		break;

	case WM_HSCROLL:
		return OnHScroll(reinterpret_cast<HWND>(lParam));
		break;

	case WM_APPCOMMAND:
		return OnAppCommand(reinterpret_cast<HWND>(wParam), GET_APPCOMMAND_LPARAM(lParam),
			GET_DEVICE_LPARAM(lParam), GET_KEYSTATE_LPARAM(lParam));
		break;

	case WM_TIMER:
		return OnTimer(static_cast<int>(wParam));
		break;

	case WM_MBUTTONUP:
		return OnMButtonUp(&MAKEPOINTS(lParam), static_cast<UINT>(wParam));
		break;

	case WM_COMMAND:
		return OnCommand(wParam, lParam);
		break;

	case WM_NOTIFY:
		return OnNotify(reinterpret_cast<LPNMHDR>(lParam));
		break;

	case WM_GETMINMAXINFO:
		return OnGetMinMaxInfo(reinterpret_cast<LPMINMAXINFO>(lParam));
		break;

	case WM_SIZE:
		return OnSize(static_cast<int>(wParam), LOWORD(lParam), HIWORD(lParam));
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

	return GetDefaultReturnValue(hwnd, uMsg, wParam, lParam);
}

INT_PTR MessageForwarder::OnInitDialog()
{
	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnTimer(int iTimerID)
{
	UNREFERENCED_PARAMETER(iTimerID);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnCtlColorDlg(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnCtlColorStatic(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnCtlColorEdit(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnCtlColorListBox(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnHScroll(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnMButtonUp(const POINTS *pts, UINT keysDown)
{
	UNREFERENCED_PARAMETER(pts);
	UNREFERENCED_PARAMETER(keysDown);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnAppCommand(HWND hwnd, UINT uCmd, UINT uDevice, DWORD dwKeys)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uCmd);
	UNREFERENCED_PARAMETER(uDevice);
	UNREFERENCED_PARAMETER(dwKeys);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnNotify(NMHDR *pnmhdr)
{
	UNREFERENCED_PARAMETER(pnmhdr);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnGetMinMaxInfo(LPMINMAXINFO pmmi)
{
	UNREFERENCED_PARAMETER(pmmi);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnSize(int iType, int iWidth, int iHeight)
{
	UNREFERENCED_PARAMETER(iType);
	UNREFERENCED_PARAMETER(iWidth);
	UNREFERENCED_PARAMETER(iHeight);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnClose()
{
	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnDestroy()
{
	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnNcDestroy()
{
	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}

INT_PTR MessageForwarder::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return GetDefaultReturnValue(m_hwnd, m_uMsg, m_wParam, m_lParam);
}
