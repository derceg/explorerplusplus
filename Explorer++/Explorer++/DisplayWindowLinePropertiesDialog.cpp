/******************************************************************
 *
 * Project: Explorer++
 * File: DisplayWindowLinePropertiesDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles all messages associated with the 'Display Window Line
 * Properties' dialog box.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "../Helper/Helper.h"
#include "MainResource.h"


DWRule_t			*g_pDWRule = NULL;
list<DWLine_t>		g_DWLines;
list<DWCommand_t>	g_DWCommands;

HICON	g_hCommandsArrowIcon;

INT_PTR CALLBACK DWLinePropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

	return pContainer->DWLinePropertiesProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::DWLinePropertiesProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	//switch(Msg)
	//{
	//	case WM_INITDIALOG:
	//		{
	//			HWND hListView;
	//			LVCOLUMN lvColumn;
	//			LVITEM lvItem;
	//			DWLine_t DWLine;
	//			DWORD dwStyleEx;
	//			list<DWFileType_t>::iterator itrTypes;
	//			TCHAR szFileTypes[256];
	//			int nFileTypes;
	//			int iIndex = 0;
	//			int iItem = 0;

	//			hListView = GetDlgItem(hDlg,IDC_LISTVIEW_LINES);

	//			SetDlgItemText(hDlg,IDC_EDIT_DESCRIPTION,g_pDWRule->szDescription);

	//			iIndex = 0;
	//			nFileTypes = g_pDWRule->FileTypes.size();
	//			StringCchCopy(szFileTypes,SIZEOF_ARRAY(szFileTypes),EMPTY_STRING);

	//			for(itrTypes = g_pDWRule->FileTypes.begin();itrTypes != g_pDWRule->FileTypes.end();itrTypes++)
	//			{
	//				StringCchCat(szFileTypes,SIZEOF_ARRAY(szFileTypes),itrTypes->szType);

	//				if(iIndex < (nFileTypes - 1))
	//					StringCchCat(szFileTypes,SIZEOF_ARRAY(szFileTypes),_T(", "));

	//				iIndex++;
	//			}

	//			SetDlgItemText(hDlg,IDC_EDIT_PATTERN,szFileTypes);

	//			list<DWLine_t>::iterator itrLines;

	//			for(itrLines = g_pDWRule->Lines.begin();itrLines != g_pDWRule->Lines.end();itrLines++)
	//			{
	//				lvItem.mask		= LVIF_TEXT;
	//				lvItem.pszText	= itrLines->szText;
	//				lvItem.iItem	= iItem++;
	//				lvItem.iSubItem	= 0;
	//				ListView_InsertItem(hListView,&lvItem);
	//			}

	//			HIMAGELIST himl;
	//			HBITMAP hBitmap;

	//			himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

	//			/* Contains all images used on the menus. */
	//			hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

	//			ImageList_Add(himl,hBitmap,NULL);

	//			g_hCommandsArrowIcon = ImageList_GetIcon(himl,SHELLIMAGES_RIGHTARROW,ILD_NORMAL);
	//			SendDlgItemMessage(hDlg,IDC_BUTTON_COMMANDS,BM_SETIMAGE,IMAGE_ICON,(LPARAM)g_hCommandsArrowIcon);

	//			DeleteObject(hBitmap);
	//			ImageList_Destroy(himl);

	//			dwStyleEx = ListView_GetExtendedListViewStyle(hListView);
	//			ListView_SetExtendedListViewStyle(hListView,dwStyleEx|LVS_EX_FULLROWSELECT);

	//			SetFocus(hListView);



	//			lvColumn.mask		= LVCF_TEXT;
	//			lvColumn.pszText	= _T("Lines");
	//			ListView_InsertColumn(hListView,0,&lvColumn);

	//			ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE_USEHEADER);

	//			DWCommand_t DWCommand;

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{name}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("Filename"));
	//			g_DWCommands.push_back(DWCommand);

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{type}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("File type"));
	//			g_DWCommands.push_back(DWCommand);

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{date_modified}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("Date Modified"));
	//			g_DWCommands.push_back(DWCommand);

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{width}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("Width"));
	//			g_DWCommands.push_back(DWCommand);

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{height}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("Height"));
	//			g_DWCommands.push_back(DWCommand);

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{bit_depth}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("Bit Depth"));
	//			g_DWCommands.push_back(DWCommand);

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{horizontal_resolution}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("Horizontal resolution"));
	//			g_DWCommands.push_back(DWCommand);

	//			StringCchCopy(DWCommand.szCommand,SIZEOF_ARRAY(DWCommand.szCommand),
	//				_T("{vertical_resolution}"));
	//			StringCchCopy(DWCommand.szDescription,SIZEOF_ARRAY(DWCommand.szDescription),
	//				_T("Vertical resolution"));
	//			g_DWCommands.push_back(DWCommand);

	//			/* Default details shown... */
	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("{name}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Filename"));
	//			g_DWLines.push_back(DWLine);

	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("{type}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Type"));
	//			g_DWLines.push_back(DWLine);

	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("Date Modified: {date_modified}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Date Modified"));
	//			g_DWLines.push_back(DWLine);

	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("Width: {width}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Width"));
	//			g_DWLines.push_back(DWLine);

	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("Height: {height}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Height"));
	//			g_DWLines.push_back(DWLine);

	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("Bit depth: {bit_depth}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Bit Depth"));
	//			g_DWLines.push_back(DWLine);

	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("Horizontal resolution: {horizontal_resolution}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Horizontal Resolution"));
	//			g_DWLines.push_back(DWLine);

	//			StringCchCopy(DWLine.szText,SIZEOF_ARRAY(DWLine.szText),
	//				_T("Vertical resolution: {vertical_resolution}"));
	//			StringCchCopy(DWLine.szDescription,SIZEOF_ARRAY(DWLine.szDescription),
	//				_T("Vertical Resolution"));
	//			g_DWLines.push_back(DWLine);
	//		}
	//		break;

	//	case WM_COMMAND:
	//		switch(LOWORD(wParam))
	//		{
	//		case IDC_BUTTON_COMMANDS:
	//			{
	//				HMENU hMenu;
	//				list<DWCommand_t>::iterator itr;
	//				TCHAR szDisplay[256];
	//				RECT rc;

	//				UINT uID = 70000;

	//				//hMenu = GetSubMenu(LoadMenu(GetModuleHandle(0),MAKEINTRESOURCE(IDR_MASSRENAME_MENU)),0);
	//				hMenu = CreatePopupMenu();

	//				for(itr = g_DWCommands.begin();itr != g_DWCommands.end();itr++)
	//				{
	//					StringCchPrintf(szDisplay,SIZEOF_ARRAY(szDisplay),_T("%s %s"),
	//						itr->szCommand,itr->szDescription);
	//					InsertMenu(hMenu,0,MF_BYPOSITION|MF_STRING,uID++,szDisplay);
	//				}

	//				GetWindowRect(GetDlgItem(hDlg,IDC_BUTTON_COMMANDS),&rc);

	//				TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_VERTICAL|TPM_RETURNCMD,
	//					rc.right,rc.top,0,hDlg,NULL);
	//			}
	//			break;

	//		case IDOK:
	//			EndDialog(hDlg,1);
	//			break;

	//		case IDCANCEL:
	//			EndDialog(hDlg,0);
	//			break;
	//		}
	//		break;

	//	case WM_NOTIFY:
	//		switch(((LPNMHDR)lParam)->code)
	//		{
	//		case LVN_ITEMCHANGED:
	//			{
	//				HWND hEdit;
	//				NMLISTVIEW *pnmlv = NULL;
	//				list<DWLine_t>::iterator itr;
	//				int  i = 0;

	//				pnmlv = (NMLISTVIEW *)lParam;

	//				hEdit = GetDlgItem(hDlg,IDC_EDIT_LINE);

	//				if(pnmlv->uNewState & LVIS_SELECTED)
	//				{
	//					if(pnmlv->iItem >= 0
	//						&& (unsigned int)pnmlv->iItem < g_DWLines.size())
	//					{
	//						itr = g_DWLines.begin();

	//						while(i < pnmlv->iItem)
	//						{
	//							itr++;
	//							i++;
	//						}

	//						SetWindowText(hEdit,itr->szText);
	//					}
	//				}
	//			}
	//			break;
	//		}
	//		break;

	//	case WM_CLOSE:
	//		EndDialog(hDlg,0);
	//		break;

	//	case WM_DESTROY:
	//		DestroyIcon(g_hCommandsArrowIcon);
	//		break;
	//}

	return 0;
}