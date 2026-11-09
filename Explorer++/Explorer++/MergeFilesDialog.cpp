// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MergeFilesDialog.h"
#include "AppInfo.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowHelper.h"
#include <wil/resource.h>
#include <Shlwapi.h>
#include <algorithm>
#include <regex>

namespace NMergeFilesDialog
{
const int WM_APP_SETCURRENTMERGECOUNT = WM_APP + 2;
const int WM_APP_MERGINGFINISHED = WM_APP + 3;

}

const TCHAR MergeFilesDialogPersistentSettings::SETTINGS_KEY[] = _T("MergeFiles");

bool CompareFilenames(const std::wstring &strFirst, const std::wstring &strSecond);

MergeFilesDialog *MergeFilesDialog::Create(const ResourceLoader *resourceLoader, HWND hParent,
	const std::wstring &strOutputDirectory, const std::vector<std::wstring> &filePaths,
	BOOL bShowFriendlyDates)
{
	return new MergeFilesDialog(resourceLoader, hParent, strOutputDirectory, filePaths,
		bShowFriendlyDates);
}

MergeFilesDialog::MergeFilesDialog(const ResourceLoader *resourceLoader, HWND hParent,
	const std::wstring &strOutputDirectory, const std::vector<std::wstring> &filePaths,
	BOOL bShowFriendlyDates) :
	BaseDialog(resourceLoader, IDD_MERGEFILES, hParent, DialogSizingType::Both),
	m_strOutputDirectory(strOutputDirectory),
	m_filePaths(filePaths),
	m_bShowFriendlyDates(bShowFriendlyDates),
	m_bMergingFiles(false)
{
	m_persistentSettings = &MergeFilesDialogPersistentSettings::GetInstance();
}

MergeFilesDialog::~MergeFilesDialog()
{
	if (m_worker.joinable())
	{
		m_worker.request_stop();
		m_worker.join();
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
	for (const auto &strFullFilename : m_filePaths)
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
		std::ranges::sort(m_filePaths, CompareFilenames);

		/* Since the filenames all match the
		pattern, construct the output filename
		from the first files name. */
		rxPattern.assign(_T("[\\.]?part[0-9]+"), std::regex_constants::icase);
		strOutputFilename =
			std::regex_replace(m_filePaths.front(), rxPattern, std::wstring(_T("")));
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

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	LVCOLUMN lvColumn;

	auto fileText = m_resourceLoader->LoadString(IDS_MERGE_FILES_COLUMN_FILE);
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = fileText.data();
	ListView_InsertColumn(hListView, 0, &lvColumn);

	auto typeText = m_resourceLoader->LoadString(IDS_MERGE_FILES_COLUMN_TYPE);
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = typeText.data();
	ListView_InsertColumn(hListView, 1, &lvColumn);

	auto sizeText = m_resourceLoader->LoadString(IDS_MERGE_FILES_COLUMN_SIZE);
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = sizeText.data();
	ListView_InsertColumn(hListView, 2, &lvColumn);

	auto dateModifiedText = m_resourceLoader->LoadString(IDS_MERGE_FILES_COLUMN_DATE_MODIFIED);
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = dateModifiedText.data();
	ListView_InsertColumn(hListView, 3, &lvColumn);

	int iItem = 0;

	for (const auto &strFullFilename : m_filePaths)
	{
		TCHAR szFullFilename[MAX_PATH];

		StringCchCopy(szFullFilename, std::size(szFullFilename), strFullFilename.c_str());

		/* TODO: Perform in background thread. */
		SHFILEINFO shfi;
		SHGetFileInfo(szFullFilename, 0, &shfi, sizeof(SHFILEINFO),
			SHGFI_SYSICONINDEX | SHGFI_TYPENAME);

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

		ULARGE_INTEGER fileSize = { { wfad.nFileSizeLow, wfad.nFileSizeHigh } };
		auto fileSizeText = FormatSizeString(fileSize.QuadPart);
		ListView_SetItemText(hListView, iItem, 2, fileSizeText.data());

		TCHAR szDateModified[32];
		CreateFileTimeString(&wfad.ftLastWriteTime, szDateModified, std::size(szDateModified),
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
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::MergeFiles, iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> MergeFilesDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_LISTVIEW), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_BUTTON_MOVEUP), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_BUTTON_MOVEDOWN), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_STATIC_OUTPUT), MovingType::Vertical,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_EDIT_FILENAME), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_BUTTON_OUTPUT), MovingType::Both,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_PROGRESS), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MERGE_STATIC_ETCHED), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Both, SizingType::None);
	return controls;
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
	OnCancel();
	return 0;
}

INT_PTR MergeFilesDialog::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case NMergeFilesDialog::WM_APP_SETCURRENTMERGECOUNT:
		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETPOS, wParam, 0);
		break;
	case NMergeFilesDialog::WM_APP_MERGINGFINISHED:
		OnFinished();
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
			auto errorMessage = m_resourceLoader->LoadString(IDS_MERGE_OUTPUTINVALID);
			MessageBox(m_hDlg, errorMessage.c_str(), AppInfo::NAME, MB_ICONWARNING | MB_OK);
			return;
		}

		std::wstring outputFileName = GetWindowString(hOutputFileName);

		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETPOS, 0, 0);

		GetDlgItemText(m_hDlg, IDOK, m_szOk, static_cast<int>(std::size(m_szOk)));

		auto cancelText = m_resourceLoader->LoadString(IDS_CANCEL);
		SetDlgItemText(m_hDlg, IDOK, cancelText.c_str());

		m_bMergingFiles = true;

		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETRANGE32, 0, 1000);
		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETPOS, 0, 0);
		for (int id : { IDC_MERGE_EDIT_FILENAME, IDC_MERGE_BUTTON_OUTPUT, IDC_MERGE_BUTTON_MOVEUP,
				 IDC_MERGE_BUTTON_MOVEDOWN })
			EnableWindow(GetDlgItem(m_hDlg, id), FALSE);
		m_closePending = false;
		m_result = {};
		try
		{
			m_worker = std::jthread(
				[this, inputs = m_filePaths, output = outputFileName](std::stop_token stop)
				{
					int previous = -1;
					auto progress = [this, &previous](uint64_t completed, uint64_t total)
					{
						int value = total ? static_cast<int>(999.0 * completed / total) : 0;
						if (value == previous)
							return;
						previous = value;
						PostMessage(m_hDlg, NMergeFilesDialog::WM_APP_SETCURRENTMERGECOUNT, value,
							0);
					};

					try
					{
						m_result = FileStreamOperations::Merge(inputs, output, stop, progress);
					}
					catch (const std::bad_alloc &)
					{
						m_result.error = E_OUTOFMEMORY;
					}
					catch (const std::exception &)
					{
						m_result.error = E_FAIL;
					}

					PostMessage(m_hDlg, NMergeFilesDialog::WM_APP_MERGINGFINISHED, 0, 0);
				});
		}
		catch (const std::system_error &)
		{
			m_result = { HRESULT_FROM_WIN32(ERROR_MAX_THRDS_REACHED) };
			OnFinished();
		}
		catch (const std::bad_alloc &)
		{
			m_result = { E_OUTOFMEMORY };
			OnFinished();
		}
	}
	else
	{
		m_worker.request_stop();
	}
}

void MergeFilesDialog::OnCancel()
{
	if (m_bMergingFiles)
	{
		m_closePending = true;
		m_worker.request_stop();
	}
	else
	{
		EndDialog(m_hDlg, 0);
	}
}

void MergeFilesDialog::OnChangeOutputDirectory()
{
	auto title = m_resourceLoader->LoadString(IDS_MERGE_SELECTDESTINATION);

	unique_pidl_absolute pidl;
	BOOL bSucceeded = FileOperations::CreateBrowseDialog(m_hDlg, title, wil::out_param(pidl));

	if (!bSucceeded)
	{
		return;
	}

	std::wstring parsingName;
	HRESULT hr = GetDisplayName(pidl.get(), SHGDN_FORPARSING, parsingName);

	if (FAILED(hr))
	{
		return;
	}

	SetDlgItemText(m_hDlg, IDC_MERGE_EDIT_FILENAME, parsingName.c_str());
}

void MergeFilesDialog::OnMove(bool bUp)
{
	if (m_bMergingFiles)
		return;
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
			if (iSelected == static_cast<int>((m_filePaths.size() - 1)))
			{
				return;
			}

			iSwap = iSelected + 1;
		}

		if (!FileStreamOperations::MoveItem(m_filePaths, iSelected, bUp))
			return;

		ListViewHelper::SwapItems(hListView, iSelected, iSwap);
	}
}

void MergeFilesDialog::OnFinished()
{
	// Joining establishes publication of the result and releases the worker before
	// any dialog state is reset or the HWND can be destroyed.
	if (m_worker.joinable())
		m_worker.join();
	m_bMergingFiles = false;
	for (int id : { IDC_MERGE_EDIT_FILENAME, IDC_MERGE_BUTTON_OUTPUT, IDC_MERGE_BUTTON_MOVEUP,
			 IDC_MERGE_BUTTON_MOVEDOWN })
		EnableWindow(GetDlgItem(m_hDlg, id), TRUE);
	SetDlgItemText(m_hDlg, IDOK, m_szOk);

	if (SUCCEEDED(m_result.error))
		SendDlgItemMessage(m_hDlg, IDC_MERGE_PROGRESS, PBM_SETPOS, 1000, 0);
	if ((FAILED(m_result.error) && !m_result.Cancelled()) || FAILED(m_result.cleanupError))
	{
		auto message = FileStreamOperations::DescribeError(m_result);
		MessageBox(m_hDlg, message.c_str(), AppInfo::NAME, MB_ICONERROR | MB_OK);
	}
	if (m_closePending)
		EndDialog(m_hDlg, 0);
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
