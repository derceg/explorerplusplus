/******************************************************************
 *
 * Project: Explorer++
 * File: MergeFiles.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides support for merging files.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Misc.h"

#define WM_USER_MERGINGFINISHED	WM_USER

typedef struct
{
	HWND hOwner;
	HWND hProgressBar;
	list<MergedFile_t> MergedFileList;
	TCHAR szOutputFileName[MAX_PATH];
} MergeFilesThreadInfo_t;

DWORD WINAPI MergeFilesThreadProc(LPVOID lpParameter);

list<MergedFile_t>	g_MergedFileList;
TCHAR				g_szMerge[32];
BOOL				g_bContinueMerging;
BOOL				g_bMergingFiles;

INT_PTR CALLBACK MergeFilesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

	return pContainer->MergeFilesProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::MergeFilesProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnMergeFilesInit(hDlg);
			break;

		case WM_USER_MERGINGFINISHED:
			OnMergingFinished(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_MERGE_BUTTON_OUTPUT:
					OnMergeFilesChangeOutputDirectory(hDlg);
					break;

				case IDC_MERGE_BUTTON_MOVEUP:
					OnMergeFilesMove(hDlg,TRUE);
					break;

				case IDC_MERGE_BUTTON_MOVEDOWN:
					OnMergeFilesMove(hDlg,FALSE);
					break;

				case IDC_MERGE_BUTTON_REMOVE:
					OnMergeFilesRemove(hDlg);
					break;

				case IDOK:
					OnMergeFilesOk(hDlg);
					break;

				case IDCANCEL:
					OnMergeFilesCancel(hDlg);
					break;
			}
			break;

		case WM_CLOSE:
			MergeFilesSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnMergeFilesInit(HWND hDlg)
{
	HWND			hListView;
	TCHAR			szCurrentDirectory[MAX_PATH];
	TCHAR			szOutputFile[MAX_PATH];
	TCHAR			szMergeText[32];
	HIMAGELIST		himlSmall;
	SHFILEINFO		shfi;
	MergedFile_t	mf;
	LVCOLUMN		lvColumn;
	LVITEM			lvItem;
	DWORD			ExtendedStyle;
	int				iItem = -1;
	int				i = 0;

	g_MergedFileList.clear();

	m_pActiveShellBrowser->QueryCurrentDirectory(MAX_PATH,szCurrentDirectory);

	StringCchPrintf(szOutputFile,SIZEOF_ARRAY(szOutputFile),
	_T("%s\\output"),szCurrentDirectory);
	SetDlgItemText(hDlg,IDC_MERGE_EDIT_FILENAME,szOutputFile);

	hListView = GetDlgItem(hDlg,IDC_MERGE_LISTVIEW);

	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	ListView_SetGridlines(hListView,TRUE);

	ExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	/* Turn on full row select for report (details) mode. */
	ListView_SetExtendedListViewStyle(hListView,ExtendedStyle | LVS_EX_FULLROWSELECT);

	LoadString(g_hLanguageModule,IDS_MERGE_COLUMN_TEXT,szMergeText,SIZEOF_ARRAY(szMergeText));

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= szMergeText;

	ListView_InsertColumn(hListView,0,&lvColumn);
	ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE_USEHEADER);

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		m_pActiveShellBrowser->QueryFullItemName(iItem,mf.szFullFileName);

		SHGetFileInfo(mf.szFullFileName,0,&shfi,
			sizeof(SHFILEINFO),SHGFI_SYSICONINDEX);

		lvItem.mask		= LVIF_TEXT | LVIF_IMAGE;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= mf.szFullFileName;
		lvItem.iImage	= shfi.iIcon;
		ListView_InsertItem(hListView,&lvItem);

		g_MergedFileList.push_back(mf);

		i++;
	}

	SendMessage(GetDlgItem(hDlg,IDC_MERGE_EDIT_FILENAME),EM_SETSEL,0,-1);
	SetFocus(GetDlgItem(hDlg,IDC_MERGE_EDIT_FILENAME));

	g_bMergingFiles = FALSE;

	if(m_bMergeFilesDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptMergeFiles.x,
			m_ptMergeFiles.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}
}

void CContainer::OnMergeFilesOk(HWND hDlg)
{
	HWND							hEditOutputFileName;
	HWND							hProgressBar;
	static MergeFilesThreadInfo_t	mfti;
	TCHAR							szOutputFileName[MAX_PATH];
	TCHAR							szStringTemp[64];
	DWORD							dwThreadId;
	int								cchOutput;

	hEditOutputFileName = GetDlgItem(hDlg,IDC_MERGE_EDIT_FILENAME);
	hProgressBar = GetDlgItem(hDlg,IDC_MERGE_PROGRESS);

	if(g_bMergingFiles)
	{
		g_bContinueMerging = FALSE;

		SetDlgItemText(hDlg,IDOK,g_szMerge);
	}
	else
	{
		if(g_MergedFileList.empty() == TRUE)
		{
			LoadString(g_hLanguageModule,IDS_MERGE_NOINPUT,
			szStringTemp,SIZEOF_ARRAY(szStringTemp));

			MessageBox(hDlg,szStringTemp,
			WINDOW_NAME,MB_ICONWARNING | MB_OK);
			return;
		}

		cchOutput = GetWindowTextLength(hEditOutputFileName);

		if(cchOutput == 0)
		{
			LoadString(g_hLanguageModule,IDS_MERGE_OUTPUTINVALID,
			szStringTemp,SIZEOF_ARRAY(szStringTemp));

			MessageBox(hDlg,szStringTemp,
			WINDOW_NAME,MB_ICONWARNING | MB_OK);
			return;
		}

		GetWindowText(hEditOutputFileName,szOutputFileName,MAX_PATH);

		mfti.hOwner			= hDlg;
		mfti.hProgressBar	= hProgressBar;
		mfti.MergedFileList	= g_MergedFileList;
		StringCchCopy(mfti.szOutputFileName,MAX_PATH,szOutputFileName);

		g_bContinueMerging = TRUE;

		CreateThread(NULL,0,MergeFilesThreadProc,(LPVOID)&mfti,0,&dwThreadId);

		GetDlgItemText(hDlg,IDOK,g_szMerge,SIZEOF_ARRAY(g_szMerge));

		LoadString(g_hLanguageModule,IDS_CANCEL,
			szStringTemp,SIZEOF_ARRAY(szStringTemp));
		SetDlgItemText(hDlg,IDOK,szStringTemp);

		g_bMergingFiles = TRUE;
	}
}

void CContainer::OnMergeFilesCancel(HWND hDlg)
{
	if(g_bMergingFiles)
	{
		g_bContinueMerging = FALSE;

		SetDlgItemText(hDlg,IDOK,g_szMerge);
	}

	MergeFilesSaveState(hDlg);

	EndDialog(hDlg,0);
}

void CContainer::OnMergeFilesChangeOutputDirectory(HWND hDlg)
{
	TCHAR	szOutputDirectory[MAX_PATH];
	TCHAR	szTitle[64];
	BOOL	bSucceeded;

	LoadString(g_hLanguageModule,IDS_MERGE_SELECTDESTINATION,
	szTitle,SIZEOF_ARRAY(szTitle));

	bSucceeded = CreateBrowseDialog(hDlg,szTitle,szOutputDirectory,MAX_PATH);

	if(bSucceeded)
	{
		SetDlgItemText(hDlg,IDC_MERGE_EDIT_FILENAME,szOutputDirectory);
	}
}

void CContainer::OnMergingFinished(HWND hDlg)
{
	TCHAR	szStringTemp[32];

	LoadString(g_hLanguageModule,IDS_OK,
	szStringTemp,SIZEOF_ARRAY(szStringTemp));

	SetDlgItemText(hDlg,IDOK,szStringTemp);

	g_bMergingFiles = FALSE;
}

void CContainer::OnMergeFilesRemove(HWND hDlg)
{
	HWND							hListView;
	list<MergedFile_t>::iterator	itr;
	int								iSelected;
	int								i = 0;

	hListView = GetDlgItem(hDlg,IDC_MERGE_LISTVIEW);

	/* Find which item is selected. */
	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	itr = g_MergedFileList.begin();

	if(iSelected != -1)
	{
		while(i < iSelected && itr != g_MergedFileList.end())
		{
			i++;

			itr++;
		}

		if(itr != g_MergedFileList.end())
		{
			g_MergedFileList.erase(itr);

			ListView_DeleteItem(hListView,iSelected);

			SetFocus(hListView);

			if(iSelected > ListView_GetItemCount(hListView))
				ListView_SelectItem(hListView,iSelected - 1,TRUE);
			else
				ListView_SelectItem(hListView,iSelected,TRUE);
		}
	}
}

void CContainer::OnMergeFilesMove(HWND hDlg,BOOL bUp)
{
	HWND							hListView;
	list<MergedFile_t>::iterator	itrSelected;
	list<MergedFile_t>::iterator	itrSwap;
	MergedFile_t					mf;
	int								iSelected;
	int								iSwap;
	int								i = 0;

	hListView = GetDlgItem(hDlg,IDC_MERGE_LISTVIEW);

	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		if((bUp && (iSelected == 0)) || (!bUp && (iSelected == ListView_GetItemCount(hListView) - 1)))
			return;

		if(bUp)
			iSwap = iSelected - 1;
		else
			iSwap = iSelected + 1;

		ListView_SwapItems(hListView,iSelected,iSwap);

		i = 0;
		itrSelected = g_MergedFileList.begin();

		while(itrSelected != g_MergedFileList.end() && i < iSelected)
		{
			itrSelected++;

			i++;
		}

		i = 0;
		itrSwap = g_MergedFileList.begin();

		while(itrSwap != g_MergedFileList.end() && i < iSwap)
		{
			itrSwap++;

			i++;
		}

		if(bUp)
		{
			mf = *itrSelected;

			g_MergedFileList.erase(itrSelected);

			g_MergedFileList.insert(itrSwap,mf);
		}
		else
		{
			mf = *itrSwap;

			g_MergedFileList.erase(itrSwap);

			g_MergedFileList.insert(itrSelected,mf);
		}

		SetFocus(hListView);
	}
}

DWORD WINAPI MergeFilesThreadProc(LPVOID lpParameter)
{
	HANDLE							hOutputFile;
	HANDLE							hMergeFile;
	MergeFilesThreadInfo_t			*pmfti;
	list<MergedFile_t>::iterator	itr;
	TCHAR							*pszMergeBuffer = NULL;
	LARGE_INTEGER					lMergeFileSize;
	DWORD							dwNumberOfBytesRead;
	DWORD							dwNumberOfBytesWritten;
	int								nSplitsMade = 1;

	pmfti = (MergeFilesThreadInfo_t *)lpParameter;

	hOutputFile = CreateFile(pmfti->szOutputFileName,GENERIC_WRITE,
	0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);

	SendMessage(pmfti->hProgressBar,PBM_SETRANGE32,0,pmfti->MergedFileList.size());

	for(itr = pmfti->MergedFileList.begin();itr != pmfti->MergedFileList.end();itr++)
	{
		if(!g_bContinueMerging)
		{
			/* The operation was cancelled. Set the position of the progress
			bar to the end. */
			SendMessage(pmfti->hProgressBar,PBM_SETPOS,pmfti->MergedFileList.size(),0);

			break;
		}

		hMergeFile = CreateFile(itr->szFullFileName,GENERIC_READ,FILE_SHARE_READ,
		NULL,OPEN_EXISTING,0,NULL);

		if(hMergeFile != INVALID_HANDLE_VALUE)
		{
			GetFileSizeEx(hMergeFile,&lMergeFileSize);

			if(lMergeFileSize.QuadPart != 0)
			{
				pszMergeBuffer = (TCHAR *)realloc(pszMergeBuffer,(size_t)(lMergeFileSize.QuadPart * sizeof(TCHAR)));

				if(pszMergeBuffer != NULL)
				{
					ReadFile(hMergeFile,(LPVOID)pszMergeBuffer,(DWORD)(lMergeFileSize.QuadPart),
						&dwNumberOfBytesRead,NULL);

					WriteFile(hOutputFile,(LPCVOID)pszMergeBuffer,dwNumberOfBytesRead,
						&dwNumberOfBytesWritten,NULL);
				}
			}

			CloseHandle(hMergeFile);

			SendMessage(pmfti->hProgressBar,PBM_SETPOS,nSplitsMade,0);

			nSplitsMade++;
		}
	}

	free(pszMergeBuffer);

	CloseHandle(hOutputFile);

	SendMessage(pmfti->hOwner,WM_USER_MERGINGFINISHED,0,0);

	return 1;
}

void CContainer::MergeFilesSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptMergeFiles.x = rcTemp.left;
	m_ptMergeFiles.y = rcTemp.top;

	m_bMergeFilesDlgStateSaved = TRUE;
}