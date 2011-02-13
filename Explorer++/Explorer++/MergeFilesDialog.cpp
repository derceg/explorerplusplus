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
#include <regex>
#include "Explorer++_internal.h"
#include "MergeFilesDialog.h"
#include "../Helper/Helper.h"


namespace NMergeFilesDialog
{
	const int WM_APP_SETTOTALMERGECOUNT		= WM_APP + 1;
	const int WM_APP_SETCURRENTMERGECOUNT	= WM_APP + 2;
	const int WM_APP_MERGINGFINISHED		= WM_APP + 3;
	const int WM_APP_OUTPUTFILEINVALID		= WM_APP + 4;

	DWORD WINAPI	MergeFilesThread(LPVOID pParam);
}

const TCHAR CMergeFilesDialogPersistentSettings::SETTINGS_KEY[] = _T("MergeFiles");

CMergeFilesDialog::CMergeFilesDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,std::wstring strOutputDirectory,
	std::list<std::wstring> FullFilenameList) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_strOutputDirectory	= strOutputDirectory;
	m_FullFilenameList		= FullFilenameList;
	m_bMergingFiles			= false;
	m_bStopMerging			= false;

	m_pmfdps = &CMergeFilesDialogPersistentSettings::GetInstance();
}

CMergeFilesDialog::~CMergeFilesDialog()
{

}

bool CompareFilenames(std::wstring strFirst,std::wstring strSecond)
{
	return (StrCmpLogicalW(strFirst.c_str(),strSecond.c_str()) <= 0);
}

BOOL CMergeFilesDialog::OnInitDialog()
{
	std::wregex rxPattern;
	bool bAllMatchPattern = true;

	rxPattern.assign(_T(".*[\\.]?part[0-9]+"),regex_constants::icase);

	/* If the files all match the pattern .*[\\.]?part[0-9]+
	(e.g. document.txt.part1), order them alphabetically. */
	for each(auto strFullFilename in m_FullFilenameList)
	{
		if(!std::tr1::regex_match(strFullFilename,rxPattern))
		{
			bAllMatchPattern = false;
			break;
		}
	}

	std::wstring strOutputFilename;

	if(bAllMatchPattern)
	{
		m_FullFilenameList.sort(CompareFilenames);

		/* Since the filenames all match the
		pattern, construct the output filename
		from the first files name. */
		rxPattern.assign(_T("[\\.]?part[0-9]+"),regex_constants::icase);
		strOutputFilename = std::tr1::regex_replace(m_FullFilenameList.front(),
			rxPattern,std::wstring(_T("")));
	}
	else
	{
		/* TODO: Improve output name. */
		strOutputFilename = _T("output");
	}

	TCHAR szOutputFile[MAX_PATH];
	PathCombine(szOutputFile,m_strOutputDirectory.c_str(),strOutputFilename.c_str());
	SetDlgItemText(m_hDlg,IDC_MERGE_EDIT_FILENAME,szOutputFile);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MERGE_LISTVIEW);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	ListView_SetImageList(hListView,himlSmall,LVSIL_SMALL);

	SetWindowTheme(hListView,L"Explorer",NULL);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	TCHAR szTemp[32];
	LoadString(g_hLanguageModule,IDS_MERGE_COLUMN_TEXT,szTemp,SIZEOF_ARRAY(szTemp));

	LVCOLUMN lvColumn;
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= szTemp;
	ListView_InsertColumn(hListView,0,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Type");
	ListView_InsertColumn(hListView,1,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Size");
	ListView_InsertColumn(hListView,2,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Date Modified");
	ListView_InsertColumn(hListView,3,&lvColumn);

	int iItem = 0;

	for each(auto strFullFilename in m_FullFilenameList)
	{
		TCHAR szFullFilename[MAX_PATH];

		StringCchCopy(szFullFilename,SIZEOF_ARRAY(szFullFilename),
			strFullFilename.c_str());

		/* TODO: Perform in background thread. */
		SHFILEINFO shfi;
		SHGetFileInfo(szFullFilename,0,&shfi,sizeof(SHFILEINFO),
			SHGFI_SYSICONINDEX|SHGFI_TYPENAME);

		LVITEM lvItem;
		lvItem.mask		= LVIF_TEXT|LVIF_IMAGE;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szFullFilename;
		lvItem.iImage	= shfi.iIcon;
		ListView_InsertItem(hListView,&lvItem);

		ListView_SetItemText(hListView,iItem,1,shfi.szTypeName);

		WIN32_FILE_ATTRIBUTE_DATA wfad;
		GetFileAttributesEx(szFullFilename,GetFileExInfoStandard,&wfad);

		TCHAR szFileSize[32];
		ULARGE_INTEGER lFileSize = {wfad.nFileSizeLow,wfad.nFileSizeHigh};
		FormatSizeString(lFileSize,szFileSize,SIZEOF_ARRAY(szFileSize));
		ListView_SetItemText(hListView,iItem,2,szFileSize);

		/* TODO: Friendly dates global. */
		TCHAR szDateModified[32];
		CreateFileTimeString(&wfad.ftLastWriteTime,szDateModified,
			SIZEOF_ARRAY(szDateModified),TRUE);
		ListView_SetItemText(hListView,iItem,3,szDateModified);

		iItem++;
	}

	ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,1,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,2,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView,3,LVSCW_AUTOSIZE_USEHEADER);

	SendMessage(GetDlgItem(m_hDlg,IDC_MERGE_EDIT_FILENAME),EM_SETSEL,0,-1);
	SetFocus(GetDlgItem(m_hDlg,IDC_MERGE_EDIT_FILENAME));

	m_pmfdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CMergeFilesDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_MERGE_LISTVIEW;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_BUTTON_MOVEUP;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_BUTTON_MOVEDOWN;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_STATIC_OUTPUT;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_EDIT_FILENAME;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_EDIT_FILENAME;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_BUTTON_OUTPUT;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_PROGRESS;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_PROGRESS;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_STATIC_ETCHED;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_MERGE_STATIC_ETCHED;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDOK;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDCANCEL;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
}

BOOL CMergeFilesDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDC_MERGE_BUTTON_OUTPUT:
		OnChangeOutputDirectory();
		break;

	case IDC_MERGE_BUTTON_MOVEUP:
		OnMove(true);
		break;

	case IDC_MERGE_BUTTON_MOVEDOWN:
		OnMove(false);
		break;

	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

BOOL CMergeFilesDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CMergeFilesDialog::OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case NMergeFilesDialog::WM_APP_SETTOTALMERGECOUNT:
		SendDlgItemMessage(m_hDlg,IDC_MERGE_PROGRESS,PBM_SETRANGE32,0,wParam);
		break;

	case NMergeFilesDialog::WM_APP_SETCURRENTMERGECOUNT:
		SendDlgItemMessage(m_hDlg,IDC_MERGE_PROGRESS,PBM_SETPOS,wParam,0);
		break;

	case NMergeFilesDialog::WM_APP_MERGINGFINISHED:
		OnFinished();
		break;
	}
}

void CMergeFilesDialog::SaveState()
{
	m_pmfdps->SaveDialogPosition(m_hDlg);
	m_pmfdps->m_bStateSaved = TRUE;
}

void CMergeFilesDialog::OnOk()
{
	if(!m_bMergingFiles)
	{
		HWND hOutputFileName = GetDlgItem(m_hDlg,IDC_MERGE_EDIT_FILENAME);

		if(GetWindowTextLength(hOutputFileName) == 0)
		{
			TCHAR szTemp[64];
			LoadString(g_hLanguageModule,IDS_MERGE_OUTPUTINVALID,
				szTemp,SIZEOF_ARRAY(szTemp));

			MessageBox(m_hDlg,szTemp,NExplorerplusplus::WINDOW_NAME,
				MB_ICONWARNING|MB_OK);
			return;
		}

		TCHAR szOutputFileName[MAX_PATH];
		GetWindowText(hOutputFileName,szOutputFileName,SIZEOF_ARRAY(szOutputFileName));

		m_pMergeFiles = new CMergeFiles(m_hDlg,szOutputFileName,m_FullFilenameList);

		GetDlgItemText(m_hDlg,IDOK,m_szOk,SIZEOF_ARRAY(m_szOk));

		TCHAR szTemp[64];
		LoadString(g_hLanguageModule,IDS_CANCEL,szTemp,SIZEOF_ARRAY(szTemp));
		SetDlgItemText(m_hDlg,IDOK,szTemp);

		m_bMergingFiles = true;

		HANDLE hThread = CreateThread(NULL,0,NMergeFilesDialog::MergeFilesThread,
			reinterpret_cast<LPVOID>(m_pMergeFiles),0,NULL);
		SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
	}
	else
	{
		m_bStopMerging = true;
	}
}

void CMergeFilesDialog::OnCancel()
{
	if(m_bMergingFiles)
	{
		m_bStopMerging = true;
	}
	else
	{
		EndDialog(m_hDlg,0);
	}
}

void CMergeFilesDialog::OnChangeOutputDirectory()
{
	TCHAR szTitle[64];
	LoadString(g_hLanguageModule,IDS_MERGE_SELECTDESTINATION,
		szTitle,SIZEOF_ARRAY(szTitle));

	TCHAR szOutputDirectory[MAX_PATH];
	BOOL bRes = CreateBrowseDialog(m_hDlg,szTitle,szOutputDirectory,MAX_PATH);

	if(bRes)
	{
		SetDlgItemText(m_hDlg,IDC_MERGE_EDIT_FILENAME,szOutputDirectory);
	}
}

void CMergeFilesDialog::OnMove(bool bUp)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MERGE_LISTVIEW);
	int iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		int iSwap;

		if(bUp)
		{
			if(iSelected == 0)
				return;

			iSwap = iSelected - 1;
		}
		else
		{
			if(iSelected == static_cast<int>((m_FullFilenameList.size() - 1)))
				return;

			iSwap = iSelected + 1;
		}

		auto itrSelected = m_FullFilenameList.begin();
		std::advance(itrSelected,iSelected);

		auto itrSwap = m_FullFilenameList.begin();
		std::advance(itrSelected,iSwap);

		std::iter_swap(itrSelected,itrSwap);

		ListView_SwapItemsNolParam(hListView,iSelected,iSwap);
	}
}

void CMergeFilesDialog::OnFinished()
{
	m_bMergingFiles = false;
}

DWORD WINAPI NMergeFilesDialog::MergeFilesThread(LPVOID pParam)
{
	assert(pParam != NULL);

	CMergeFiles *pMergeFiles = reinterpret_cast<CMergeFiles *>(pParam);
	pMergeFiles->StartMerging();

	return 0;
}

CMergeFiles::CMergeFiles(HWND hDlg,std::wstring strOutputFilename,std::list<std::wstring> FullFilenameList)
{
	m_hDlg				= hDlg;
	m_strOutputFilename	= strOutputFilename;
	m_FullFilenameList	= FullFilenameList;

	m_bstopMerging		= false;

	InitializeCriticalSection(&m_csStop);
}

CMergeFiles::~CMergeFiles()
{
	DeleteCriticalSection(&m_csStop);
}

void CMergeFiles::StartMerging()
{
	LARGE_INTEGER lMergeFileSize;
	int nFilesMerged = 1;
	bool bStop = false;

	HANDLE hOutputFile = CreateFile(m_strOutputFilename.c_str(),GENERIC_WRITE,
		0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hOutputFile == INVALID_HANDLE_VALUE)
	{
		PostMessage(m_hDlg,NMergeFilesDialog::WM_APP_OUTPUTFILEINVALID,0,0);
		return;
	}

	PostMessage(m_hDlg,NMergeFilesDialog::WM_APP_SETTOTALMERGECOUNT,
		static_cast<WPARAM>(m_FullFilenameList.size()),0);

	for each(auto strFullFilename in m_FullFilenameList)
	{
		if(bStop)
		{
			break;
		}

		HANDLE hInputFile = CreateFile(strFullFilename.c_str(),GENERIC_READ,FILE_SHARE_READ,
			NULL,OPEN_EXISTING,0,NULL);

		if(hInputFile != INVALID_HANDLE_VALUE)
		{
			GetFileSizeEx(hInputFile,&lMergeFileSize);

			if(lMergeFileSize.QuadPart != 0)
			{
				char *pBuffer = new char[lMergeFileSize.LowPart];

				DWORD dwNumberOfBytesRead;
				ReadFile(hInputFile,reinterpret_cast<LPVOID>(pBuffer),static_cast<DWORD>(lMergeFileSize.LowPart),
					&dwNumberOfBytesRead,NULL);

				DWORD dwNumberOfBytesWritten;
				WriteFile(hOutputFile,reinterpret_cast<LPCVOID>(pBuffer),dwNumberOfBytesRead,
					&dwNumberOfBytesWritten,NULL);

				delete[] pBuffer;
			}

			CloseHandle(hInputFile);

			PostMessage(m_hDlg,NMergeFilesDialog::WM_APP_SETCURRENTMERGECOUNT,nFilesMerged,0);

			nFilesMerged++;

			EnterCriticalSection(&m_csStop);
			if(m_bstopMerging)
			{
				bStop = true;
			}
			LeaveCriticalSection(&m_csStop);
		}
	}

	CloseHandle(hOutputFile);

	SendMessage(m_hDlg,NMergeFilesDialog::WM_APP_MERGINGFINISHED,0,0);
}

CMergeFilesDialogPersistentSettings::CMergeFilesDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CMergeFilesDialogPersistentSettings::~CMergeFilesDialogPersistentSettings()
{
	
}

CMergeFilesDialogPersistentSettings& CMergeFilesDialogPersistentSettings::GetInstance()
{
	static CMergeFilesDialogPersistentSettings mfdps;
	return mfdps;
}