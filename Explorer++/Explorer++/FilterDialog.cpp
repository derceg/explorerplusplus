/******************************************************************
 *
 * Project: Explorer++
 * File: FilterDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Filter' dialog.
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


INT_PTR CALLBACK FilterProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->FilterProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::FilterProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				HWND						hComboBox;
				list<Filter_t>::iterator	itr;

				hComboBox = GetDlgItem(hDlg,IDC_FILTER_COMBOBOX);

				SetFocus(hComboBox);

				for(itr = m_FilterList.begin();itr != m_FilterList.end();itr++)
				{
					if(itr == m_FilterList.begin())
						SendMessage(hComboBox,WM_SETTEXT,0,(LPARAM)itr->pszFilterString);

					SendMessage(hComboBox,CB_ADDSTRING,(WPARAM)-1,(LPARAM)itr->pszFilterString);
				}

				SendMessage(hComboBox,CB_SETEDITSEL,0,MAKELPARAM(0,-1));

				if(m_bFilterDlgStateSaved)
				{
					SetWindowPos(hDlg,NULL,m_ptFilter.x,m_ptFilter.y,
						0,0,SWP_NOSIZE|SWP_NOZORDER);
				}
				else
				{
					CenterWindow(m_hContainer,hDlg);
				}
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					{
						HWND		hComboBox;
						Filter_t	Filter;
						int			iBufSize;

						hComboBox = GetDlgItem(hDlg,IDC_FILTER_COMBOBOX);

						iBufSize = GetWindowTextLength(hComboBox);

						Filter.pszFilterString = (TCHAR *)malloc((iBufSize + 1) * sizeof(TCHAR));

						SendMessage(hComboBox,WM_GETTEXT,iBufSize + 1,(LPARAM)Filter.pszFilterString);

						m_FilterList.push_front(Filter);

						m_pActiveShellBrowser->SetFilter(Filter.pszFilterString);

						if(!m_pActiveShellBrowser->GetFilterStatus())
							m_pActiveShellBrowser->SetFilterStatus(TRUE);

						FilterSaveState(hDlg);

						EndDialog(hDlg,0);
					}
					break;

				case IDCANCEL:
					FilterSaveState(hDlg);
					EndDialog(hDlg,1);
					break;
			}
			break;

		case WM_CLOSE:
			FilterSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::FilterSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptFilter.x = rcTemp.left;
	m_ptFilter.y = rcTemp.top;

	m_bFilterDlgStateSaved = TRUE;
}