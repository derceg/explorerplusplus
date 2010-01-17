/******************************************************************
 *
 * Project: Explorer++
 * File: CustomizeColorsDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the customize colors dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"


extern BOOL g_bEditing;

INT_PTR CALLBACK ColorFilteringProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

	return pContainer->ColorFilteringProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::ColorFilteringProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			{
				HWND hListView;
				LVCOLUMN lvColumn;
				LVITEM lvItem;
				list<ListViewColouring_t>::iterator itr;
				int iItem = 0;

				hListView = GetDlgItem(hDlg,IDC_LISTVIEW_COLORRULES);

				lvColumn.mask		= LVCF_TEXT;
				lvColumn.pszText	= _T("Description");
				ListView_InsertColumn(hListView,0,&lvColumn);

				ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE_USEHEADER);

				if(!m_ColourFilter.empty())
				{
					for(itr = m_ColourFilter.begin();itr != m_ColourFilter.end();itr++)
					{
						lvItem.mask		= LVIF_TEXT;
						lvItem.pszText	= itr->szDescription;
						lvItem.iItem	= iItem++;
						lvItem.iSubItem	= 0;
						ListView_InsertItem(hListView,&lvItem);
					}
				}

				ListView_SetExtendedListViewStyleEx(hListView,
					LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);

				SetFocus(hListView);

				if(m_bCustomizeColorsDlgStateSaved)
				{
					SetWindowPos(hDlg,NULL,m_ptCustomizeColors.x,
						m_ptCustomizeColors.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
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
			case IDC_BUTTON_NEW:
				{
					HWND hListView;
					LVITEM lvItem;
					ListViewColouring_t lvc;
					INT_PTR iRet;

					hListView = GetDlgItem(hDlg,IDC_LISTVIEW_COLORRULES);

					StringCchCopy(lvc.szDescription,
						SIZEOF_ARRAY(lvc.szDescription),EMPTY_STRING);
					StringCchCopy(lvc.szFilterPattern,
						SIZEOF_ARRAY(lvc.szDescription),EMPTY_STRING);

					lvc.rgbColour			= m_crInitialColor;
					lvc.dwFilterAttributes	= 0;

					m_pColoringItem = &lvc;

					g_bEditing = FALSE;

					iRet = DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_NEWCOLORRULE),
						hDlg,ColorRuleProcStub,(LPARAM)this);

					if(iRet == 1)
					{
						m_ColourFilter.push_back(lvc);

						int nItems;

						nItems = ListView_GetItemCount(hListView);

						/* Insert the item into the listview. */
						lvItem.mask		= LVIF_TEXT;
						lvItem.pszText	= lvc.szDescription;
						lvItem.iItem	= nItems;
						lvItem.iSubItem	= 0;
						ListView_InsertItem(hListView,&lvItem);
					}

					/* Regardless of whether the user canelled the dialog
					or added the rule, save their color selection. */
					m_crInitialColor = lvc.rgbColour;

					SetFocus(hDlg);
				}
				break;

			case IDC_BUTTON_EDIT:
				{
					HWND hListView;
					int iSelected;

					hListView = GetDlgItem(hDlg,IDC_LISTVIEW_COLORRULES);

					iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

					OnEditColorRule(hDlg,iSelected);
				}
				break;

			case IDC_BUTTON_MOVEUP:
				CustomizeColorsMove(hDlg,TRUE);
				break;

			case IDC_BUTTON_MOVEDOWN:
				CustomizeColorsMove(hDlg,FALSE);
				break;

			case IDC_BUTTON_DELETE:
				CustomizeColorsDelete(hDlg);
				break;

			case IDOK:
				{
					/* Causes the active listview to redraw (therefore
					applying the updated color schemes). */
					InvalidateRect(m_hActiveListView,NULL,FALSE);
					CustomizeColorsSaveState(hDlg);
					EndDialog(hDlg,1);
				}
				break;
			}
			break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
			case NM_DBLCLK:
				{
					NMITEMACTIVATE	*pnmItem = NULL;

					pnmItem = (NMITEMACTIVATE *)lParam;

					if(pnmItem->iItem != -1)
					{
						OnEditColorRule(hDlg,pnmItem->iItem);
					}
				}
				break;
			}
			break;

		case WM_CLOSE:
			InvalidateRect(m_hActiveListView,NULL,FALSE);
			CustomizeColorsSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::CustomizeColorsMove(HWND hDlg,BOOL bUp)
{
	HWND		hListView;
	list<ListViewColouring_t>::iterator	itr;
	ListViewColouring_t	lvc;
	int			iSelected;
	int			iSwap;
	int			i = 0;

	hListView = GetDlgItem(hDlg,IDC_LISTVIEW_COLORRULES);

	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		if(bUp)
		{
			if(iSelected == 0)
				return;

			iSwap = iSelected - 1;
		}
		else
		{
			if(iSelected == (ListView_GetItemCount(hListView) - 1))
				return;

			iSwap = iSelected + 1;
		}

		itr = m_ColourFilter.begin();

		while(i < iSelected && itr != m_ColourFilter.end())
		{
			i++;
			itr++;
		}

		lvc = *itr;

		m_ColourFilter.erase(itr);

		itr = m_ColourFilter.begin();

		while(i < iSwap && itr != m_ColourFilter.end())
		{
			i++;
			itr++;
		}

		m_ColourFilter.insert(itr,lvc);

		ListView_SwapItemsNolParam(hListView,iSelected,iSwap);
	}
}

void CContainer::CustomizeColorsDelete(HWND hDlg)
{
	HWND		hListView;
	list<ListViewColouring_t>::iterator	itr;
	int			nItems;
	int			iSelected;
	int			i = 0;

	hListView = GetDlgItem(hDlg,IDC_LISTVIEW_COLORRULES);

	nItems = ListView_GetItemCount(hListView);
	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		itr = m_ColourFilter.begin();

		while(i < iSelected && itr != m_ColourFilter.end())
		{
			i++;
			itr++;
		}

		TCHAR szInfoMsg[128];
		int	iMessageBoxReturn;

		LoadString(g_hLanguageModule,IDS_COLORRULE_DELETE,
			szInfoMsg,SIZEOF_ARRAY(szInfoMsg));

		iMessageBoxReturn = MessageBox(hDlg,szInfoMsg,
			WINDOW_NAME,MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2);

		if(iMessageBoxReturn == IDYES)
		{
			m_ColourFilter.erase(itr);

			ListView_DeleteItem(hListView,iSelected);

			if(iSelected == (nItems - 1))
				iSelected--;

			ListView_SelectItem(hListView,iSelected,TRUE);
		}

		SetFocus(hListView);
	}
}

void CContainer::CustomizeColorsSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptCustomizeColors.x = rcTemp.left;
	m_ptCustomizeColors.y = rcTemp.top;

	m_bCustomizeColorsDlgStateSaved = TRUE;
}

void CContainer::OnEditColorRule(HWND hDlg,int iSelected)
{
	HWND hListView;
	list<ListViewColouring_t>::iterator itr;
	INT_PTR iRet;
	int i = 0;

	hListView = GetDlgItem(hDlg,IDC_LISTVIEW_COLORRULES);

	if(iSelected != -1)
	{
		itr = m_ColourFilter.begin();

		while(i < iSelected && itr != m_ColourFilter.end())
		{
			i++;
			itr++;
		}

		m_pColoringItem = &(*itr);

		g_bEditing = TRUE;

		iRet = DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_NEWCOLORRULE),
			hDlg,ColorRuleProcStub,(LPARAM)this);

		if(iRet == 1)
		{
			ListView_SetItemText(hListView,iSelected,0,itr->szDescription);
		}
	}

	SetFocus(hDlg);
}