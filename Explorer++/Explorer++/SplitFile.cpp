/******************************************************************
 *
 * Project: Explorer++
 * File: SplitFile.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides the ability to split a file into several
 * pieces.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Misc.h"


#define WM_USER_SPLITFINISHED	WM_USER

typedef struct
{
	HWND hOwner;
	HWND hProgressBar;
	TCHAR szFullFileName[MAX_PATH];
	TCHAR szOutputDirectory[MAX_PATH];
	UINT uSplitSize;
} SplitFileInfo_t;

DWORD WINAPI	SplitFileThreadProc(LPVOID lpParameter);

TCHAR	g_szSplit[32];
BOOL	g_bContinueSplitting;

INT_PTR CALLBACK SplitFileProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

	return pContainer->SplitFileProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::SplitFileProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnSplitFileInit(hDlg);
			break;

		case WM_USER_SPLITFINISHED:
			OnSplitFinished(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_SPLIT_BUTTON_OUTPUT:
					OnSplitFileChangeOutputDirectory(hDlg);
					break;

				case IDOK:
					OnSplitFileOk(hDlg);
					break;

				case IDCANCEL:
					OnSplitFileCancel(hDlg);
					break;
			}
			break;

		case WM_CLOSE:
			SplitFileSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnSplitFileInit(HWND hDlg)
{
	HWND	hEditSize;
	HWND	hComboBox;
	TCHAR	szOutputDirectory[MAX_PATH];
	int		iSelected;

	iSelected = ListView_GetNextItem(m_hActiveListView,-1,LVNI_SELECTED);

	m_pActiveShellBrowser->QueryFullItemName(iSelected,m_SplitFileName);

	hEditSize = GetDlgItem(hDlg,IDC_SPLIT_EDIT_SIZE);

	SetWindowText(hEditSize,_T("10"));

	StringCchCopy(szOutputDirectory,MAX_PATH,m_SplitFileName);
	PathRemoveFileSpec(szOutputDirectory);

	SetDlgItemText(hDlg,IDC_SPLIT_EDIT_OUTPUT,szOutputDirectory);

	hComboBox = GetDlgItem(hDlg,IDC_SPLIT_COMBOBOX_SIZES);

	SendMessage(hComboBox,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)_T("Bytes"));
	SendMessage(hComboBox,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)_T("KB"));
	SendMessage(hComboBox,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)_T("MB"));

	SendMessage(hComboBox,CB_SELECTSTRING,(WPARAM)-1,(LPARAM)_T("KB"));

	m_bSplittingFile = FALSE;

	/* Select all the text in the size edit control. */
	SendMessage(hEditSize,EM_SETSEL,0,-1);

	/* Focus on the size edit control. */
	SetFocus(hEditSize);

	if(m_bSplitFileDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptSplitFile.x,
			m_ptSplitFile.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}
}

void CContainer::OnSplitFileOk(HWND hDlg)
{
	HWND	hEditSize;
	HWND	hEditOutputDirectory;
	HWND	hProgressBar;
	HWND	hComboBox;
	HANDLE	hOutputDirectory;
	TCHAR	szOutputDirectory[MAX_PATH];
	TCHAR	szStringTemp[64];
	UINT	uSplitSize;
	int		cchOutput;
	int		iCurSel;
	BOOL	bTranslated;

	hEditSize = GetDlgItem(hDlg,IDC_SPLIT_EDIT_SIZE);
	hEditOutputDirectory = GetDlgItem(hDlg,IDC_SPLIT_EDIT_OUTPUT);
	hProgressBar = GetDlgItem(hDlg,IDC_SPLIT_PROGRESS);

	if(m_bSplittingFile)
	{
		g_bContinueSplitting = FALSE;

		SetDlgItemText(hDlg,IDOK,g_szSplit);
	}
	else
	{
		cchOutput = GetWindowTextLength(hEditOutputDirectory);
		uSplitSize = GetDlgItemInt(hDlg,IDC_SPLIT_EDIT_SIZE,&bTranslated,FALSE);

		GetWindowText(hEditOutputDirectory,szOutputDirectory,MAX_PATH);

		if(!bTranslated)
		{
			MessageBox(hDlg,_T("Please enter a size for the split files"),
			WINDOW_NAME,MB_ICONWARNING | MB_OK);

			return;
		}

		/* Check if the specified directory actually exists. */
		hOutputDirectory = CreateFile(szOutputDirectory,0,0,NULL,
		OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);

		if(cchOutput == 0 || hOutputDirectory == INVALID_HANDLE_VALUE)
		{
			MessageBox(hDlg,_T("The output directory is invalid"),
			WINDOW_NAME,MB_ICONWARNING | MB_OK);

			return;
		}

		CloseHandle(hOutputDirectory);

		hComboBox = GetDlgItem(hDlg,IDC_SPLIT_COMBOBOX_SIZES);

		iCurSel = (int)SendMessage(hComboBox,CB_GETCURSEL,0,0);

		switch(iCurSel)
		{
			case 0:
				/* Nothing needs to be done, as the selection
				is in bytes. */
				break;

			case 1:
				/* KB. */
				uSplitSize *= 1024;
				break;

			case 2:
				/* MB. */
				uSplitSize *= (1024 * 1024);
				break;
		}

		if(bTranslated && cchOutput != 0)
		{
			static SplitFileInfo_t SplitFileInfo;
			DWORD ThreadId;

			SplitFileInfo.hOwner		= hDlg;
			SplitFileInfo.hProgressBar	= hProgressBar;
			SplitFileInfo.uSplitSize	= uSplitSize;
			StringCchCopy(SplitFileInfo.szFullFileName,MAX_PATH,m_SplitFileName);
			StringCchCopy(SplitFileInfo.szOutputDirectory,MAX_PATH,szOutputDirectory);

			g_bContinueSplitting = TRUE;

			CreateThread(NULL,0,SplitFileThreadProc,(LPVOID)&SplitFileInfo,0,&ThreadId);
		}

		GetDlgItemText(hDlg,IDOK,g_szSplit,SIZEOF_ARRAY(g_szSplit));

		LoadString(g_hLanguageModule,IDS_CANCEL,
			szStringTemp,SIZEOF_ARRAY(szStringTemp));
		SetDlgItemText(hDlg,IDOK,szStringTemp);

		m_bSplittingFile = TRUE;
	}
}

void CContainer::OnSplitFileCancel(HWND hDlg)
{
	if(m_bSplittingFile)
	{
		g_bContinueSplitting = FALSE;

		SetDlgItemText(hDlg,IDOK,g_szSplit);
	}

	SplitFileSaveState(hDlg);

	EndDialog(hDlg,0);
}

void CContainer::OnSplitFinished(HWND hDlg)
{
	SetDlgItemText(hDlg,IDOK,g_szSplit);

	m_bSplittingFile = FALSE;
}

DWORD WINAPI SplitFileThreadProc(LPVOID lpParameter)
{
	SplitFileInfo_t	*psfi = NULL;
	HANDLE			hInputFile;
	HANDLE			hOutputFile;
	LARGE_INTEGER	lFileSize;
	LARGE_INTEGER	lRunningSplitSize;
	TCHAR			szFileName[MAX_PATH];
	TCHAR			szOutputFile[MAX_PATH];
	TCHAR			*pszBuffer = NULL;
	DWORD			dwNumberOfBytesRead;
	DWORD			dwNumberOfBytesWritten;
	LONGLONG		nSplits;
	int				nSplitsMade = 1;

	psfi = (SplitFileInfo_t *)lpParameter;

	hInputFile = CreateFile(psfi->szFullFileName,GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
		0,NULL);

	if(hInputFile == INVALID_HANDLE_VALUE)
		return 0;

	GetFileSizeEx(hInputFile,&lFileSize);

	nSplits = lFileSize.QuadPart / psfi->uSplitSize;

	if((lFileSize.QuadPart % psfi->uSplitSize) != 0)
		nSplits++;

	SendMessage(psfi->hProgressBar,PBM_SETRANGE32,0,(LPARAM)nSplits);

	lRunningSplitSize.QuadPart = 0;

	pszBuffer = (TCHAR *)malloc(psfi->uSplitSize * sizeof(TCHAR));

	StringCchCopy(szFileName,MAX_PATH,psfi->szFullFileName);
	PathStripPath(szFileName);

	while(lRunningSplitSize.QuadPart < lFileSize.QuadPart)
	{
		if(!g_bContinueSplitting)
		{
			/* The operation was canceled. Set the position of the progress
			bar to the end. */
			SendMessage(psfi->hProgressBar,PBM_SETPOS,(WPARAM)nSplits,0);

			break;
		}

		ReadFile(hInputFile,(LPVOID)pszBuffer,psfi->uSplitSize,&dwNumberOfBytesRead,NULL);

		StringCchPrintf(szOutputFile,MAX_PATH,_T("%s\\%s.%d"),psfi->szOutputDirectory,szFileName,nSplitsMade);

		hOutputFile = CreateFile(szOutputFile,GENERIC_WRITE,0,NULL,CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,NULL);

		if(hOutputFile != INVALID_HANDLE_VALUE)
		{
			WriteFile(hOutputFile,(LPCVOID)pszBuffer,dwNumberOfBytesRead,
				&dwNumberOfBytesWritten,NULL);

			CloseHandle(hOutputFile);
		}

		SendMessage(psfi->hProgressBar,PBM_SETPOS,nSplitsMade,0);

		lRunningSplitSize.QuadPart += dwNumberOfBytesRead;

		nSplitsMade++;
	}

	CloseHandle(hInputFile);

	SendMessage(psfi->hOwner,WM_USER_SPLITFINISHED,0,0);

	return 1;
}

void CContainer::OnSplitFileChangeOutputDirectory(HWND hDlg)
{
	TCHAR	szOutputDirectory[MAX_PATH];
	TCHAR	szTitle[] = _T("Select a destination folder");
	BOOL	bSucceeded;

	bSucceeded = CreateBrowseDialog(hDlg,szTitle,szOutputDirectory,MAX_PATH);

	if(bSucceeded)
	{
		SetDlgItemText(hDlg,IDC_SPLIT_EDIT_OUTPUT,szOutputDirectory);
	}
}

void CContainer::SplitFileSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptSplitFile.x = rcTemp.left;
	m_ptSplitFile.y = rcTemp.top;

	m_bSplitFileDlgStateSaved = TRUE;
}