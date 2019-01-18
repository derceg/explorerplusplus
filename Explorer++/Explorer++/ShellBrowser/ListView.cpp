/******************************************************************
*
* Project: Explorer++
* File: ListView.cpp
* License: GPL - See LICENSE in the top level directory
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

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
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}