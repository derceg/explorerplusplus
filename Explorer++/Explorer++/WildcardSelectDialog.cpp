/******************************************************************
 *
 * Project: Explorer++
 * File: WildcardSelectDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Allows items to be selected/deselected based
 * on a wildcard filter.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"

using namespace std;


INT_PTR CALLBACK WildcardSelectProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->WildcardSelectProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::WildcardSelectProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnWildcardSelectInit(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					OnWildcardSelectOk(hDlg);
					break;

				case IDCANCEL:
					WildcardSelectSaveState(hDlg);
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			WildcardSelectSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnWildcardSelectInit(HWND hDlg)
{
	HWND hComboBox;
	list<WildcardSelectInfo_t>::iterator itr;

	hComboBox = GetDlgItem(hDlg,IDC_SELECTGROUP_COMBOBOX);

	if(!m_wsiList.empty())
	{
		for(itr = m_wsiList.begin();itr != m_wsiList.end();itr++)
		{
			ComboBox_InsertString(hComboBox,-1,itr->szPattern);
		}
	}

	ComboBox_SetText(hComboBox,m_szwsiText);

	/* If the position of the dialog has previously
	been saved this session, restore it. */
	if(m_bWildcardDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptWildcardSelect.x,
			m_ptWildcardSelect.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}

	if(!m_bWildcardSelect)
	{
		TCHAR szTemp[64];

		LoadString(g_hLanguageModule,IDS_WILDCARDDESELECTION,
			szTemp,SIZEOF_ARRAY(szTemp));
		SetWindowText(hDlg,szTemp);
	}

	SetFocus(hComboBox);
}

void CContainer::OnWildcardSelectOk(HWND hDlg)
{
	WildcardSelectInfo_t	wsi;
	TCHAR	FullFileName[MAX_PATH];
	TCHAR	szPattern[512];
	int		nItems;
	int		i = 0;

	nItems = ListView_GetItemCount(m_hActiveListView);

	GetDlgItemText(hDlg,IDC_SELECTGROUP_COMBOBOX,
		szPattern,SIZEOF_ARRAY(szPattern));

	for(i = 0;i < nItems;i++)
	{
		m_pActiveShellBrowser->QueryName(i,FullFileName);

		if(CheckWildcardMatch(szPattern,FullFileName,FALSE) == 1)
		{
			ListView_SelectItem(m_hActiveListView,i,m_bWildcardSelect);
		}
	}

	StringCchCopy(wsi.szPattern,SIZEOF_ARRAY(wsi.szPattern),
		szPattern);
	m_wsiList.push_front(wsi);

	WildcardSelectSaveState(hDlg);
	EndDialog(hDlg,1);
}

void CContainer::WildcardSelectSaveState(HWND hDlg)
{
	HWND hComboBox;
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptWildcardSelect.x = rcTemp.left;
	m_ptWildcardSelect.y = rcTemp.top;

	hComboBox = GetDlgItem(hDlg,IDC_SELECTGROUP_COMBOBOX);
	ComboBox_GetText(hComboBox,m_szwsiText,SIZEOF_ARRAY(m_szwsiText));

	m_bWildcardDlgStateSaved = TRUE;
}