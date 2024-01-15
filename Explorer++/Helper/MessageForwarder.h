// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class MessageForwarder
{
public:
	virtual ~MessageForwarder() = default;

protected:
	INT_PTR ForwardMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual INT_PTR OnInitDialog();
	virtual INT_PTR OnTimer(int iTimerID);
	virtual INT_PTR OnCtlColorDlg(HWND hwnd, HDC hdc);
	virtual INT_PTR OnCtlColorStatic(HWND hwnd, HDC hdc);
	virtual INT_PTR OnCtlColorEdit(HWND hwnd, HDC hdc);
	virtual INT_PTR OnCtlColorListBox(HWND hwnd, HDC hdc);
	virtual INT_PTR OnHScroll(HWND hwnd);
	virtual INT_PTR OnMButtonUp(const POINTS *pts, UINT keysDown);
	virtual INT_PTR OnAppCommand(HWND hwnd, UINT uCmd, UINT uDevice, DWORD dwKeys);
	virtual INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	virtual INT_PTR OnNotify(NMHDR *pnmhdr);
	virtual INT_PTR OnGetMinMaxInfo(LPMINMAXINFO pmmi);
	virtual INT_PTR OnSize(int iType, int iWidth, int iHeight);
	virtual INT_PTR OnClose();
	virtual INT_PTR OnDestroy();
	virtual INT_PTR OnNcDestroy();

	/* For private application messages in
	the range WM_APP (0x8000) - 0xBFFF. */
	virtual INT_PTR OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual INT_PTR GetDefaultReturnValue(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

private:
	/* Any derived class should not have to define each
	of the methods above. However, the default implementation
	within this class must return the default value for this
	window type (as it varies between a window and dialog).
	Therefore, these values will be saved on each call of
	the window procedure. They will then be used to return
	the default value (which will be retrieved from the derived
	class). */
	HWND m_hwnd;
	UINT m_uMsg;
	WPARAM m_wParam;
	LPARAM m_lParam;
};
