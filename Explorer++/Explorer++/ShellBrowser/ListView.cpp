// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "iShellView.h"

LRESULT CALLBACK CShellBrowser::ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CShellBrowser *shellBrowser = reinterpret_cast<CShellBrowser *>(dwRefData);
	return shellBrowser->ListViewProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CShellBrowser::ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_APP_COLUMN_RESULT_READY:
		ProcessColumnResult(static_cast<int>(wParam));
		break;

	case WM_APP_THUMBNAIL_RESULT_READY:
		ProcessThumbnailResult(static_cast<int>(wParam));
		break;

	case WM_APP_ICON_RESULT_READY:
		ProcessIconResult(static_cast<int>(wParam));
		break;

	case WM_APP_SHELL_NOTIFY:
		OnShellNotify(wParam, lParam);
		return 0;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CShellBrowser::ListViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CShellBrowser *shellBrowser = reinterpret_cast<CShellBrowser *>(dwRefData);
	return shellBrowser->ListViewParentProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CShellBrowser::ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hListView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case LVN_GETDISPINFO:
				OnListViewGetDisplayInfo(lParam);
				break;

			case LVN_COLUMNCLICK:
				ColumnClicked(reinterpret_cast<NMLISTVIEW *>(lParam)->iSubItem);
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}