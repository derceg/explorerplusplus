/******************************************************************
 *
 * Project: Explorer++
 * File: TabBackingHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Manages the 'tab backing' panel, which sits
 * behind the tab control.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"



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
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TabBackingProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TabBackingProc(HWND hTabCtrl,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITMENU:
			SendMessage(m_hContainer,WM_INITMENU,wParam,lParam);
			break;

		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT	*pMeasureItem = NULL;

				pMeasureItem = (MEASUREITEMSTRUCT *)lParam;

				pMeasureItem->itemWidth = 20;
				pMeasureItem->itemHeight = 10;

			}
			break;

		case WM_LBUTTONDBLCLK:
			{
				HRESULT hr;

				hr = BrowseFolder(m_DefaultTabDirectory,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

				if(FAILED(hr))
					BrowseFolder(m_DefaultTabDirectoryStatic,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
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
					OnTabSelectionChange();
					break;

				case TTN_GETDISPINFO:
					OnTabCtrlGetDispInfo(lParam);
					break;
			}
			break;
	}

	return DefSubclassProc(hTabCtrl,msg,wParam,lParam);
}