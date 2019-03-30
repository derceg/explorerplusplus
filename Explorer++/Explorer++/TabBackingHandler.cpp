// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Manages the 'tab backing' panel, which sits
 * behind the tab control.
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "../Helper/Macros.h"


LRESULT CALLBACK TabBackingProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

void Explorerplusplus::CreateTabBacking(void)
{
	m_hTabBacking = CreateWindow(WC_STATIC,EMPTY_STRING,
	WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SS_NOTIFY,
	0,0,0,0,m_hContainer,NULL,GetModuleHandle(0),NULL);

	SetWindowSubclass(m_hTabBacking,TabBackingProcStub,0,(DWORD_PTR)this);
}

LRESULT CALLBACK TabBackingProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TabBackingProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TabBackingProc(HWND hTabCtrl,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_LBUTTONDBLCLK:
			{
				HRESULT hr = CreateNewTab(m_DefaultTabDirectory, TabSettings(_selected = true));

				if (FAILED(hr))
				{
					CreateNewTab(m_DefaultTabDirectoryStatic, TabSettings(_selected = true));
				}
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case TABTOOLBAR_CLOSE:
					OnCloseTab();
					break;
			}
			break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
				case TCN_SELCHANGE:
					OnTabSelectionChanged();
					break;
			}
			break;
	}

	return DefSubclassProc(hTabCtrl,msg,wParam,lParam);
}