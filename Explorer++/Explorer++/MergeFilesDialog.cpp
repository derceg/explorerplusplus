// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MergeFilesDialog.h"
#include "CoreInterface.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include <boost/scope_exit.hpp>
#include <regex>

#pragma warning(                                                                                   \
	disable : 4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

namespace NMergeFilesDialog
{
const int WM_APP_SETTOTALMERGECOUNT = WM_APP + 1;
const int WM_APP_SETCURRENTMERGECOUNT = WM_APP + 2;
const int WM_APP_MERGINGFINISHED = WM_APP + 3;
const int WM_APP_OUTPUTFILEINVALID = WM_APP + 4;

DWORD WINAPI MergeFilesThread(LPVOID pParam);
}

const TCHAR MergeFilesDialogPersistentSettings::SETTINGS_KEY[] = _T("MergeFiles");

bool CompareFilenames(const std::wstring &strFirst, const std::wstring &strSecond);

MergeFilesDialog::MergeFilesDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
	const std::wstring &strOutputDirectory, const std::list<std::wstring> &FullFilenameList,
	BOOL bShowFriendlyDates) :
	BaseDialog(hInstance, IDD_MERGEFILES, hParent, true),
	m_expp(expp),
	m_strOutputDirectory(strOutputDirectory),
	m_FullFilenameList(FullFilenameList),
	m_bShowFriendlyDates(bShowFriendlyDates),
	m_bMergingFiles(false),
	m_bStopMerging(false),
	m_pMergeFiles(nullptr)
{
	m_persistentSettings = &MergeFilesDialogPersistentSettings::GetInstance();
}

MergeFilesDialog::~MergeFilesDialog()
{
	if (m_pMergeFiles != nullptr)
	{
		m_pMergeFiles->StopMerging();
		m_pMergeFiles->Release();
	}
}

bool CompareFilenames(const std::wstring &strFirst, const std::wstring &strSecond)
{
	return (StrCmpLogicalW(strFirst.c_str(), strSecond.c_str()) <= 0);
}

INT_PTR MergeFilesDialog::OnInitDialog()
{
	std::wregex rxPattern;
	bool bAllMatchPattern = true;

	rxPattern.assign(_T(".*[\\.]?part[0-9]+"), std::regex_constants::icase);

	/* If the files all match the pattern .*[\\.]?part[0-9]+
	(e.g. document.txt.part1), order them alphabetically. */
	for (const auto &strFullFilename : m_FullFilenameList)
	{
		if (!std::regex_match(strFullFilename, rxPattern))
		{
			bAllMatchPattern = false;
			break;
		}
	}

	std::wstring strOutputFilename;

	if (bAllMatchPattern)
	{
		m_FullFilenameList.sort(CompareFilenames);

		/* Since the filenames all match the
		pattern, construct the output filename
		from the first files name. */
		rxPattern.assign(_T("[\\.]?part[0-9]+"), std::regex_constants::icase);
		strOutputFilename =
			std::regex_replace(m_FullFilenameList.front(), rxPattern, std::wstring(_T("")));
	}
	else
	{
		/* TODO: Improve output name. */
		strOutputFilename = _T("output");
	}

	TCHAR szOutputFile[MAX_PATH];
	PathCombine(szOutputFile, m_strOutputDirectory.c_str(), strOutputFilename.c_str());
	SetDlgItemText(m_hDlg, IDC_MERGE_EDIT_FILENAME, szOutputFile);

	HWND hListView = GetDlgItem(m_hDlg, IDC_MERGE_LISTVIEW);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(nullptr, &himlSmall);
	ListView_SetImageList(hListView, himlSmall, LVSIL_SMALL);

	SetWindowTheme(hListView, L"Explorer", nullptr);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	LVCOLUMN lvColumn;
	TCHAR szTemp[32];

	LoadString(GetInstance(), IDS_MERGE_FILES_COLUMN_FILE, szTemp, SIZEOF_ARRAY(szTemp));
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = szTemp;
	ListView_InsertColumn(hListView, 0, &lvColumn);

	LoadString(GetInstance(), IDS_MERGE_FILES_COLUMN_TYPE, szTemp, SIZEOF_ARRAY(szTemp));
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = szTemp;
	ListView_InsertColumn(hListView, 1, &lvColumn);

	LoadString(GetInstance(), IDS_MERGE_FILES_COLUMN_SIZE, szTemp, SIZEOF_ARRAY(szTemp));
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = szTemp;
	ListView_InsertColumn(hListView, 2, &lvColumn);

	LoadString(GetInstance(), IDS_MERGE_FILES_COLUMN_DATE_MODIFIED, szTemp, SIZEOF_ARRAY(szTemp));
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = szTemp;
	ListView_InsertColumn(hListView, 3, &lvColumn);

	int iItem = 0;

	for (const auto &strFullFilename : m_FullFilenameList)
	{
		TCHAR szFullFilename[MAX_PATH];

		StringCchCopy(szFullFilename, SIZEOF_ARRAY(szFullFilename), strFullFilename.c_str());

		/* TODO: Perform in background thread. */
		SHFILEINFO shfi;
		SHGetFileInfo(
			szFullFilename, 0, &shfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_TYPENAME);

		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = szFullFilename;
		lvItem.iImage = shfi.iIcon;
		ListView_InsertItem(hListView, &lvItem);

		ListView_SetItemText(hListView, iItem, 1, shfi.szTypeName);

		WIN32_FILE_ATTRIBUTE_DATA wfad;
		GetFileAttributesEx(szFullFilename, GetFileExInfoStandard, &wfad);

		TCHAR szFileSize[32];
		ULARGE_INTEGER lFileSize = { wfad.nFileSizeLow, wfad.nFileSizeHigh };
		FormatSizeString(lFileSize, szFileSize, SIZEOF_ARRAY(szFileSize));
		ListView_SetItemText(hListView, iItem, 2, szFileSize);

		TCHAR szDateModified[32];
		CreateFileTimeString(&wfad.ftLastWriteTime, szDateModified, SIZEOF_ARRAY(szDateModified),
			m_bShowFriendlyDates);
		ListView_SetItemText(hListView, iItem, 3, szDateModified);

		iItem++;
	}

	ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView, 1, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView, 2, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hListView, 3, LVSCW_AUTOSIZE_USEHEADER);

	SendMessage(GetDlgItem(m_hDlg, IDC_MERGE_EDIT_FILENAME), EM_SETSEL, 0, -1);
	SetFocus(GetDlgItem(m_hDlg, IDC_MERGE_EDIT_FILENAME));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

wil::unique_hicon MergeFilesDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_expp->GetIconResourceLoader()->LoadIconFromPNGAndScale(
		Icon::MergeFiles, iconWidth, iconHeight);
}

void MergeFilesDialog::GetResizableControlInformation(
	BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &ControlList)
{
	dsc = BaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	ResizableDialog::Control_t control;

	control.iID = IDC_MERGE_LISTVIEW;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_BUTTON_MOVEUP;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_BUTTON_MOVEDOWN;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_STATIC_OUTPUT;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_EDIT_FILENAME;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_EDIT_FILENAME;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_BUTTON_OUTPUT;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_PROGRESS;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_PROGRESS;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_STATIC_ETCHED;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(control);

	control.iID = IDC_MERGE_STATIC_ETCHED;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(control);

	control.iID = IDOK;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);

	control.iID = IDCANCEL;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);

	control.iID = IDC_GRIPPER;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(control);
}

INT_PTR MergeFilesDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
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

INT_PTR MergeFilesDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

INT_PTR MergeFilesDialog::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (uMsg)
	{
	case NMergeFilesDialog::WM_APP_SETTOTALMERGECOUNT:
		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETRANGE32, 0, wParam);
		break;

	case NMergeFilesDialog::WM_APP_SETCURRENTMERGECOUNT:
		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETPOS, wParam, 0);
		break;

	case NMergeFilesDialog::WM_APP_MERGINGFINISHED:
		OnFinished();
		break;

	case NMergeFilesDialog::WM_APP_OUTPUTFILEINVALID:
	{
		TCHAR szTemp[64];
		LoadString(GetInstance(), IDS_MERGE_FILES_OUTPUTFILEINVALID, szTemp, SIZEOF_ARRAY(szTemp));
		MessageBox(m_hDlg, szTemp, NExplorerplusplus::APP_NAME, MB_ICONWARNING | MB_OK);

		assert(m_pMergeFiles != nullptr);

		m_pMergeFiles->Release();
		m_pMergeFiles = nullptr;

		m_bMergingFiles = false;
		m_bStopMerging = false;

		SetDlgItemText(m_hDlg, IDOK, m_szOk);
	}
	break;
	}

	return 0;
}

void MergeFilesDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);
	m_persistentSettings->m_bStateSaved = TRUE;
}

void MergeFilesDialog::OnOk()
{
	if (!m_bMergingFiles)
	{
		HWND hOutputFileName = GetDlgItem(m_hDlg, IDC_MERGE_EDIT_FILENAME);

		if (GetWindowTextLength(hOutputFileName) == 0)
		{
			TCHAR szTemp[64];
			LoadString(GetInstance(), IDS_MERGE_OUTPUTINVALID, szTemp, SIZEOF_ARRAY(szTemp));

			MessageBox(m_hDlg, szTemp, NExplorerplusplus::APP_NAME, MB_ICONWARNING | MB_OK);
			return;
		}

		TCHAR szOutputFileName[MAX_PATH];
		GetWindowText(hOutputFileName, szOutputFileName, SIZEOF_ARRAY(szOutputFileName));

		m_pMergeFiles = new MergeFiles(m_hDlg, szOutputFileName, m_FullFilenameList);

		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETPOS, 0, 0);

		GetDlgItemText(m_hDlg, IDOK, m_szOk, SIZEOF_ARRAY(m_szOk));

		TCHAR szTemp[64];
		LoadString(GetInstance(), IDS_CANCEL, szTemp, SIZEOF_ARRAY(szTemp));
		SetDlgItemText(m_hDlg, IDOK, szTemp);

		m_bMergingFiles = true;

		HANDLE hThread = CreateThread(nullptr, 0, NMergeFilesDialog::MergeFilesThread,
			reinterpret_cast<LPVOID>(m_pMergeFiles), 0, nullptr);
		SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
		CloseHandle(hThread);
	}
	else
	{
		m_bStopMerging = true;

		if (m_pMergeFiles != nullptr)
		{
			m_pMergeFiles->StopMerging();
		}
	}
}

void MergeFilesDialog::OnCancel()
{
	if (m_bMergingFiles)
	{
		m_bStopMerging = true;
	}
	else
	{
		EndDialog(m_hDlg, 0);
	}
}

void MergeFilesDialog::OnChangeOutputDirectory()
{
	TCHAR szTitle[128];
	LoadString(GetInstance(), IDS_MERGE_SELECTDESTINATION, szTitle, SIZEOF_ARRAY(szTitle));

	PIDLIST_ABSOLUTE pidl;
	BOOL bSucceeded = NFileOperations::CreateBrowseDialog(m_hDlg, szTitle, &pidl);

	if (!bSucceeded)
	{
		return;
	}

	BOOST_SCOPE_EXIT(pidl)
	{
		CoTaskMemFree(pidl);
	}
	BOOST_SCOPE_EXIT_END

	TCHAR parsingName[MAX_PATH];
	HRESULT hr = GetDisplayName(pidl, parsingName, SIZEOF_ARRAY(parsingName), SHGDN_FORPARSING);

	if (FAILED(hr))
	{
		return;
	}

	SetDlgItemText(m_hDlg, IDC_MERGE_EDIT_FILENAME, parsingName);
}

void MergeFilesDialog::OnMove(bool bUp)
{
	HWND hListView = GetDlgItem(m_hDlg, IDC_MERGE_LISTVIEW);
	int iSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

	if (iSelected != -1)
	{
		int iSwap;

		if (bUp)
		{
			if (iSelected == 0)
			{
				return;
			}

			iSwap = iSelected - 1;
		}
		else
		{
			if (iSelected == static_cast<int>((m_FullFilenameList.size() - 1)))
			{
				return;
			}

			iSwap = iSelected + 1;
		}

		auto itrSelected = m_FullFilenameList.begin();
		std::advance(itrSelected, iSelected);

		auto itrSwap = m_FullFilenameList.begin();
		std::advance(itrSelected, iSwap);

		std::iter_swap(itrSelected, itrSwap);

		ListViewHelper::SwapItems(hListView, iSelected, iSwap, FALSE);
	}
}

void MergeFilesDialog::OnFinished()
{
	assert(m_pMergeFiles != nullptr);

	m_pMergeFiles->Release();
	m_pMergeFiles = nullptr;

	m_bMergingFiles = false;
	m_bStopMerging = false;

	/* Set the progress bar position to the end. */
	int iHighLimit =
		static_cast<int>(SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_GETRANGE, FALSE, 0));
	SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETPOS, iHighLimit, 0);

	SetDlgItemText(m_hDlg, IDOK, m_szOk);
}

DWORD WINAPI NMergeFilesDialog::MergeFilesThread(LPVOID pParam)
{
	assert(pParam != nullptr);

	auto *pMergeFiles = reinterpret_cast<MergeFiles *>(pParam);
	pMergeFiles->StartMerging();

	return 0;
}

MergeFiles::MergeFiles(HWND hDlg, const std::wstring &strOutputFilename,
	const std::list<std::wstring> &FullFilenameList)
{
	m_hDlg = hDlg;
	m_strOutputFilename = strOutputFilename;
	m_FullFilenameList = FullFilenameList;

	m_bstopMerging = false;

	InitializeCriticalSection(&m_csStop);
}

MergeFiles::~MergeFiles()
{
	DeleteCriticalSection(&m_csStop);
}

void MergeFiles::StartMerging()
{
	LARGE_INTEGER lMergeFileSize;
	int nFilesMerged = 1;
	bool bStop = false;

	HANDLE hOutputFile = CreateFile(m_strOutputFilename.c_str(), GENERIC_WRITE, 0, nullptr,
		CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hOutputFile == INVALID_HANDLE_VALUE)
	{
		PostMessage(m_hDlg, NMergeFilesDialog::WM_APP_OUTPUTFILEINVALID, 0, 0);
		return;
	}

	PostMessage(m_hDlg, NMergeFilesDialog::WM_APP_SETTOTALMERGECOUNT,
		static_cast<WPARAM>(m_FullFilenameList.size()), 0);

	for (const auto &strFullFilename : m_FullFilenameList)
	{
		if (bStop)
		{
			break;
		}

		HANDLE hInputFile = CreateFile(strFullFilename.c_str(), GENERIC_READ, FILE_SHARE_READ,
			nullptr, OPEN_EXISTING, 0, nullptr);

		if (hInputFile != INVALID_HANDLE_VALUE)
		{
			GetFileSizeEx(hInputFile, &lMergeFileSize);

			if (lMergeFileSize.QuadPart != 0)
			{
				auto buffer = std::make_unique<char[]>(lMergeFileSize.LowPart);

				DWORD dwNumberOfBytesRead;
				ReadFile(hInputFile, reinterpret_cast<LPVOID>(buffer.get()),
					static_cast<DWORD>(lMergeFileSize.LowPart), &dwNumberOfBytesRead, nullptr);

				DWORD dwNumberOfBytesWritten;
				WriteFile(hOutputFile, reinterpret_cast<LPCVOID>(buffer.get()), dwNumberOfBytesRead,
					&dwNumberOfBytesWritten, nullptr);
			}

			CloseHandle(hInputFile);

			PostMessage(m_hDlg, NMergeFilesDialog::WM_APP_SETCURRENTMERGECOUNT, nFilesMerged, 0);

			nFilesMerged++;

			EnterCriticalSection(&m_csStop);
			if (m_bstopMerging)
			{
				bStop = true;
			}
			LeaveCriticalSection(&m_csStop);
		}
	}

	CloseHandle(hOutputFile);

	SendMessage(m_hDlg, NMergeFilesDialog::WM_APP_MERGINGFINISHED, 0, 0);
}

void MergeFiles::StopMerging()
{
	EnterCriticalSection(&m_csStop);
	m_bstopMerging = true;
	LeaveCriticalSection(&m_csStop);
}

MergeFilesDialogPersistentSettings::MergeFilesDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
}

MergeFilesDialogPersistentSettings &MergeFilesDialogPersistentSettings::GetInstance()
{
	static MergeFilesDialogPersistentSettings mfdps;
	return mfdps;
}