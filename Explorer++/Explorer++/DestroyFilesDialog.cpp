/******************************************************************
 *
 * Project: Explorer++
 * File: DestroyFilesDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Destroy Files' dialog and associated messages.
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


typedef struct
{
	TCHAR szFullFileName[MAX_PATH];
} DestroyedFile_t;

list<DestroyedFile_t>	g_DestroyedFileList;
UINT					uOverwriteMethod;

INT_PTR CALLBACK DestroyFilesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

	return pContainer->DestroyFilesProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::DestroyFilesProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			OnDestroyFilesInit(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_DESTROYFILES_RADIO_ONEPASS:
					uOverwriteMethod = OVERWRITE_ONEPASS;
					break;

				case IDC_DESTROYFILES_RADIO_THREEPASS:
					uOverwriteMethod = OVERWRITE_THREEPASS;
					break;

				case IDC_DESTROYFILES_BUTTON_REMOVE:
					OnDestroyFilesRemove(hDlg);
					break;

				case IDOK:
					OnDestroyFilesOk(hDlg);
					break;

				case IDCANCEL:
					DestroyFilesSaveState(hDlg);
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			DestroyFilesSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnDestroyFilesInit(HWND hDlg)
{
	HWND			hListView;
	DestroyedFile_t	df;
	LVCOLUMN		lvColumn;
	LVITEM			lvItem;
	DWORD			ExtendedStyle;
	int				iItem = -1;
	int				i = 0;

	g_DestroyedFileList.clear();

	hListView = GetDlgItem(hDlg,IDC_DESTROYFILES_LISTVIEW);

	ListView_SetGridlines(hListView,TRUE);

	ExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	/* Turn on full row select for report (details) mode. */
	ListView_SetExtendedListViewStyle(hListView,ExtendedStyle | LVS_EX_FULLROWSELECT);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Files");

	ListView_InsertColumn(hListView,0,&lvColumn);

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		m_pActiveShellBrowser->QueryFullItemName(iItem,df.szFullFileName);

		g_DestroyedFileList.push_back(df);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= df.szFullFileName;
		ListView_InsertItem(hListView,&lvItem);

		i++;
	}

	ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE);

	uOverwriteMethod = OVERWRITE_ONEPASS;

	CheckDlgButton(hDlg,IDC_DESTROYFILES_RADIO_ONEPASS,BST_CHECKED);

	if(m_bDestroyFilesDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptDestroyFiles.x,
			m_ptDestroyFiles.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}
}

void CContainer::OnDestroyFilesOk(HWND hDlg)
{
	int	iButtonPressed;

	if(g_DestroyedFileList.empty() == TRUE)
	{
		/* No files to delete. */
		EndDialog(hDlg,0);
		return;
	}

	/* The default button in this message box will be the second
	button (i.e. the no button). */
	iButtonPressed = MessageBox(hDlg,
	_T("Files that are destroyed will be \
permanently deleted, and will NOT be recoverable.\n\n\
Are you sure you want to continue?"),
	WINDOW_NAME,
	MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNO | MB_DEFBUTTON2);

	DestroyFilesSaveState(hDlg);

	switch(iButtonPressed)
	{
		case IDNO:
			/* Do nothing. */
			EndDialog(hDlg,0);
			break;

		case IDYES:
			OnDestroyFilesConfirmDelete(hDlg);
			break;

		default:
			/* Do nothing. */
			EndDialog(hDlg,0);
			break;
	}
}

void CContainer::OnDestroyFilesConfirmDelete(HWND hDlg)
{
	list<DestroyedFile_t>::iterator	itr;

	for(itr = g_DestroyedFileList.begin();itr != g_DestroyedFileList.end();itr++)
	{
		DeleteFileSecurely(itr->szFullFileName,uOverwriteMethod);
	}

	EndDialog(hDlg,1);
}

void CContainer::OnDestroyFilesRemove(HWND hDlg)
{
	HWND							hListView;
	list<DestroyedFile_t>::iterator	itr;
	int								iSelected;
	int								i = 0;

	hListView = GetDlgItem(hDlg,IDC_DESTROYFILES_LISTVIEW);

	/* Find which item is selected. */
	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	itr = g_DestroyedFileList.begin();

	if(iSelected != -1)
	{
		while(i < iSelected && itr != g_DestroyedFileList.end())
		{
			i++;

			itr++;
		}

		if(itr != g_DestroyedFileList.end())
		{
			g_DestroyedFileList.erase(itr);

			ListView_DeleteItem(hListView,iSelected);

			SetFocus(hListView);

			if(iSelected > ListView_GetItemCount(hListView))
				ListView_SelectItem(hListView,iSelected - 1,TRUE);
			else
				ListView_SelectItem(hListView,iSelected,TRUE);
		}
	}
}

void CContainer::DestroyFilesSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptDestroyFiles.x = rcTemp.left;
	m_ptDestroyFiles.y = rcTemp.top;

	m_bDestroyFilesDlgStateSaved = TRUE;
}