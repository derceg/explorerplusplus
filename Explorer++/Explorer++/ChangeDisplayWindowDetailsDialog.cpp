/******************************************************************
 *
 * Project: Explorer++
 * File: ChangeDisplayWindowDetailsDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles all messages associated with the 'Change Display Window
 * details' dialog box.
 *
 * Format:
 *
 * e.g. For images:
 * Applies to: *.bmp, *.ico, *.gif, *.jpg, *.exf, *.png, *.tif,
 *             *.wmf, *.emf, *.tiff
 * Lines:
 * {name}
 * {type}
 * Date Modified: {date_modified}
 * Width: {width}
 * Height: {height}
 * Bit depth: {bit_depth}
 * Horizontal resolution: {horizontal_resolution}
 * Vertical resolution: {vertical_resolution}
 *
 * Show preview (checkbox)
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "../Helper/Helper.h"
#include "MainResource.h"


extern DWRule_t	*g_pDWRule;

INT_PTR CALLBACK DWChangeDetailsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (Explorerplusplus *)lParam;
		}
		break;
	}

	return pContainer->DWChangeDetailsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::DWChangeDetailsProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	/*switch(Msg)
	{
		case WM_INITDIALOG:
			{
				HWND hListView;
				LVCOLUMN lvColumn;
				LVITEM lvItem;
				DWORD dwStyleEx;
				int iItem = 0;

				hListView = GetDlgItem(hDlg,IDC_LISTVIEW_DISPLAYLINES);

				lvColumn.mask		= LVCF_TEXT;
				lvColumn.pszText	= _T("Description");
				ListView_InsertColumn(hListView,0,&lvColumn);

				lvColumn.mask		= LVCF_TEXT;
				lvColumn.pszText	= _T("Applies to");
				ListView_InsertColumn(hListView,1,&lvColumn);

				list<DWRule_t>::iterator itr;
				list<DWFileType_t>::iterator itrTypes;
				TCHAR szFileTypes[256];
				int nFileTypes;
				int iIndex = 0;

				if(!m_DWRules.empty())
				{
					for(itr = m_DWRules.begin();itr != m_DWRules.end();itr++)
					{
						lvItem.mask		= LVIF_TEXT;
						lvItem.pszText	= itr->szDescription;
						lvItem.iItem	= iItem;
						lvItem.iSubItem	= 0;
						ListView_InsertItem(hListView,&lvItem);

						if(!itr->FileTypes.empty())
						{
							iIndex = 0;
							nFileTypes = itr->FileTypes.size();
							StringCchCopy(szFileTypes,SIZEOF_ARRAY(szFileTypes),EMPTY_STRING);

							for(itrTypes = itr->FileTypes.begin();itrTypes != itr->FileTypes.end();itrTypes++)
							{
								StringCchCat(szFileTypes,SIZEOF_ARRAY(szFileTypes),itrTypes->szType);

								if(iIndex < (nFileTypes - 1))
									StringCchCat(szFileTypes,SIZEOF_ARRAY(szFileTypes),_T(", "));

								iIndex++;
							}

							ListView_SetItemText(hListView,iItem,1,szFileTypes);
						}

						iItem++;
					}
				}

				ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE);
				ListView_SetColumnWidth(hListView,1,LVSCW_AUTOSIZE_USEHEADER);

				dwStyleEx = ListView_GetExtendedListViewStyle(hListView);
				ListView_SetExtendedListViewStyle(hListView,dwStyleEx|LVS_EX_FULLROWSELECT);

				SetFocus(hListView);

				CenterWindow(m_hContainer,hDlg);
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDC_BUTTON_EDIT:
				{
					HWND hListView;
					list<DWRule_t>::iterator itr;
					int iSelected;
					int i = 0;

					hListView = GetDlgItem(hDlg,IDC_LISTVIEW_DISPLAYLINES);

					iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

					if(iSelected != -1)
					{
						itr = m_DWRules.begin();

						while(i < iSelected && itr != m_DWRules.end())
						{
							i++;
							itr++;
						}

						g_pDWRule = &(*itr);

						DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_DISPLAYWINDOWLINEPROPERTIES),
							hDlg,DWLinePropertiesProcStub,(LPARAM)this);
					}
				}
				break;

			case IDOK:
				EndDialog(hDlg,1);
				break;

			case IDCANCEL:
				EndDialog(hDlg,0);
				break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}*/

	return 0;
}