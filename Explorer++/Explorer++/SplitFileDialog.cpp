// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SplitFileDialog.h"
#include "AppInfo.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/FileOperations.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"
#include <wil/resource.h>
#include <Shlwapi.h>
#include <comdef.h>
#include <unordered_map>

namespace NSplitFileDialog
{
const int WM_APP_SETCURRENTSPLITCOUNT = WM_APP + 2;
const int WM_APP_SPLITFINISHED = WM_APP + 3;

const TCHAR COUNTER_PATTERN[] = _T("/N");

}

const TCHAR SplitFileDialogPersistentSettings::SETTINGS_KEY[] = _T("SplitFile");

const TCHAR SplitFileDialogPersistentSettings::SETTING_SIZE[] = _T("Size");
const TCHAR SplitFileDialogPersistentSettings::SETTING_SIZE_GROUP[] = _T("SizeGroup");

SplitFileDialog *SplitFileDialog::Create(const ResourceLoader *resourceLoader, HWND hParent,
	const std::wstring &strFullFilename)
{
	return new SplitFileDialog(resourceLoader, hParent, strFullFilename);
}

SplitFileDialog::SplitFileDialog(const ResourceLoader *resourceLoader, HWND hParent,
	const std::wstring &strFullFilename) :
	BaseDialog(resourceLoader, IDD_SPLITFILE, hParent, DialogSizingType::None),
	m_strFullFilename(strFullFilename),
	m_bSplittingFile(false)
{
	m_persistentSettings = &SplitFileDialogPersistentSettings::GetInstance();
}

SplitFileDialog::~SplitFileDialog()
{
	if (m_worker.joinable())
	{
		m_worker.request_stop();
		m_worker.join();
	}
}

INT_PTR SplitFileDialog::OnInitDialog()
{
	SHFILEINFO shfi;
	DWORD_PTR dwRes = SHGetFileInfo(m_strFullFilename.c_str(), 0, &shfi, sizeof(shfi), SHGFI_ICON);

	if (dwRes != 0)
	{
		ICONINFO ii;
		GetIconInfo(shfi.hIcon, &ii);
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_STATIC_ICON, STM_SETIMAGE, IMAGE_BITMAP,
			reinterpret_cast<LPARAM>(ii.hbmColor));

		DeleteObject(ii.hbmColor);
		DeleteObject(ii.hbmMask);
		DestroyIcon(shfi.hIcon);
	}

	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_FILENAME, m_strFullFilename.c_str());

	HANDLE hFile = CreateFile(m_strFullFilename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, 0, nullptr);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER lFileSize;
		GetFileSizeEx(hFile, &lFileSize);

		auto fileSizeText = FormatSizeString(lFileSize.QuadPart);
		SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_FILESIZE, fileSizeText.c_str());

		CloseHandle(hFile);
	}

	TCHAR szOutputDirectory[MAX_PATH];
	StringCchCopy(szOutputDirectory, std::size(szOutputDirectory), m_strFullFilename.c_str());
	PathRemoveFileSpec(szOutputDirectory);
	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_OUTPUT, szOutputDirectory);

	HWND hComboBox = GetDlgItem(m_hDlg, IDC_SPLIT_COMBOBOX_SIZES);
	int iPos;

	auto text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_BYTES);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::Bytes));
	text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_KB);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::KB));
	text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_MB);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::MB));
	text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_GB);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::GB));

	SendMessage(hComboBox, CB_SELECTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(m_persistentSettings->m_strSplitGroup.c_str()));

	HWND hEditSize = GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_SIZE);
	SetWindowText(hEditSize, m_persistentSettings->m_strSplitSize.c_str());
	SendMessage(hEditSize, EM_SETSEL, 0, -1);
	SetFocus(hEditSize);

	TCHAR szOutputFilename[MAX_PATH];
	StringCchCopy(szOutputFilename, std::size(szOutputFilename), m_strFullFilename.c_str());
	PathStripPath(szOutputFilename);
	StringCchPrintf(szOutputFilename, std::size(szOutputFilename), _T("%s.part%s"),
		szOutputFilename, NSplitFileDialog::COUNTER_PATTERN);
	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_OUTPUTFILENAME, szOutputFilename);

	auto hCurentFont = reinterpret_cast<HFONT>(
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_STATIC_FILENAMEHELPER, WM_GETFONT, 0, 0));

	LOGFONT lf;
	GetObject(hCurentFont, sizeof(lf), reinterpret_cast<LPVOID>(&lf));

	lf.lfItalic = TRUE;
	m_hHelperTextFont = CreateFontIndirect(&lf);

	SendDlgItemMessage(m_hDlg, IDC_SPLIT_STATIC_FILENAMEHELPER, WM_SETFONT,
		reinterpret_cast<WPARAM>(m_hHelperTextFont), MAKEWORD(TRUE, 0));

	SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_ELAPSEDTIME, _T("00:00:00"));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

wil::unique_hicon SplitFileDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::SplitFiles, iconWidth, iconHeight);
}

INT_PTR SplitFileDialog::OnTimer(int iTimerID)
{
	if (iTimerID == ELPASED_TIMER_ID)
	{
		m_uElapsedTime++;

		/* Update the elapsed time display (form is hh:mm:ss). */
		TCHAR szElapsedTime[9];
		StringCchPrintf(szElapsedTime, std::size(szElapsedTime), _T("%02d:%02d:%02d"),
			m_uElapsedTime / 3600, (m_uElapsedTime / 60) % 60, m_uElapsedTime % 60);
		SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_ELAPSEDTIME, szElapsedTime);
	}

	return 0;
}

INT_PTR SplitFileDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_SPLIT_BUTTON_OUTPUT:
		OnChangeOutputDirectory();
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

INT_PTR SplitFileDialog::OnClose()
{
	OnCancel();
	return 0;
}

INT_PTR SplitFileDialog::OnDestroy()
{
	DeleteObject(m_hHelperTextFont);

	return 0;
}

void SplitFileDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_strSplitSize = GetWindowString(GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_SIZE));
	m_persistentSettings->m_strSplitGroup =
		GetWindowString(GetDlgItem(m_hDlg, IDC_SPLIT_COMBOBOX_SIZES));

	m_persistentSettings->m_bStateSaved = TRUE;
}

INT_PTR SplitFileDialog::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case NSplitFileDialog::WM_APP_SETCURRENTSPLITCOUNT:
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_SETPOS, wParam, 0);
		break;
	case NSplitFileDialog::WM_APP_SPLITFINISHED:
		OnSplitFinished();
		break;
	}
	return 0;
}

void SplitFileDialog::OnOk()
{
	if (!m_bSplittingFile)
	{
		HWND hOutputFilename = GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_OUTPUTFILENAME);

		if (GetWindowTextLength(hOutputFilename) == 0)
		{
			auto errorMessage =
				m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_OUTPUTFILENAMEERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

			SetFocus(hOutputFilename);
			return;
		}

		std::wstring strOutputFilename = GetWindowString(hOutputFilename);

		/* Now, check that the filename has a variable component. Without the
		variable component, the filenames of all the split files would be exactly
		the same. */
		if (strOutputFilename.find(NSplitFileDialog::COUNTER_PATTERN) == std::wstring::npos)
		{
			auto errorMessage =
				m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_OUTPUTFILENAMECONSTANTERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

			SetFocus(hOutputFilename);
			return;
		}

		HWND hEditOutputDirectory = GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_OUTPUT);

		if (GetWindowTextLength(hEditOutputDirectory) == 0)
		{
			auto errorMessage =
				m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_OUTPUTDIRECTORYERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

			SetFocus(hEditOutputDirectory);
			return;
		}

		std::wstring strOutputDirectory = GetWindowString(hEditOutputDirectory);

		HWND hComboBox = GetDlgItem(m_hDlg, IDC_SPLIT_COMBOBOX_SIZES);
		int selection = static_cast<int>(SendMessage(hComboBox, CB_GETCURSEL, 0, 0));
		auto entry = m_SizeMap.find(selection);
		uint64_t multiplier = 1;
		if (entry != m_SizeMap.end())
		{
			switch (entry->second)
			{
			case SizeType::Bytes:
				break;
			case SizeType::KB:
				multiplier = KB;
				break;
			case SizeType::MB:
				multiplier = MB;
				break;
			case SizeType::GB:
				multiplier = GB;
				break;
			}
		}
		auto partSize = FileStreamOperations::ParsePartSize(
			GetWindowString(GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_SIZE)), multiplier);
		if (!partSize || entry == m_SizeMap.end())
		{
			auto message = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_SIZEERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, message.c_str());
			SetFocus(GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_SIZE));
			return;
		}

		GetDlgItemText(m_hDlg, IDOK, m_szOk, static_cast<int>(std::size(m_szOk)));

		auto stopText = m_resourceLoader->LoadString(IDS_STOP);
		SetDlgItemText(m_hDlg, IDOK, stopText.c_str());

		m_bSplittingFile = true;

		m_uElapsedTime = 0;
		SetTimer(m_hDlg, ELPASED_TIMER_ID, ELPASED_TIMER_TIMEOUT, nullptr);

		auto splittingText = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_SPLITTING);
		SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, splittingText.c_str());

		SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_SETRANGE32, 0, 1000);
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_SETPOS, 0, 0);
		for (int id : { IDC_SPLIT_EDIT_SIZE, IDC_SPLIT_COMBOBOX_SIZES, IDC_SPLIT_EDIT_OUTPUT,
				 IDC_SPLIT_EDIT_OUTPUTFILENAME, IDC_SPLIT_BUTTON_OUTPUT })
			EnableWindow(GetDlgItem(m_hDlg, id), FALSE);
		m_closePending = false;
		m_result = {};
		try
		{
			m_worker = std::jthread(
				[this, input = m_strFullFilename, directory = strOutputDirectory,
					pattern = strOutputFilename, size = *partSize](std::stop_token stop)
				{
					int previous = -1;
					auto progress = [this, &previous](uint64_t completed, uint64_t total)
					{
						int value = total ? static_cast<int>(999.0 * completed / total) : 0;
						if (value == previous)
							return;
						previous = value;
						PostMessage(m_hDlg, NSplitFileDialog::WM_APP_SETCURRENTSPLITCOUNT, value,
							0);
					};

					try
					{
						m_result = FileStreamOperations::Split(input, directory, pattern, size,
							stop, progress);
					}
					catch (const std::bad_alloc &)
					{
						m_result.error = E_OUTOFMEMORY;
					}
					catch (const std::exception &)
					{
						m_result.error = E_FAIL;
					}

					PostMessage(m_hDlg, NSplitFileDialog::WM_APP_SPLITFINISHED, 0, 0);
				});
		}
		catch (const std::system_error &)
		{
			m_result = { HRESULT_FROM_WIN32(ERROR_MAX_THRDS_REACHED) };
			OnSplitFinished();
		}
		catch (const std::bad_alloc &)
		{
			m_result = { E_OUTOFMEMORY };
			OnSplitFinished();
		}
	}
	else
	{
		m_worker.request_stop();
	}
}

void SplitFileDialog::OnCancel()
{
	if (m_bSplittingFile)
	{
		m_closePending = true;
		m_worker.request_stop();
	}
	else
	{
		EndDialog(m_hDlg, 0);
	}
}

void SplitFileDialog::OnChangeOutputDirectory()
{
	auto title = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_DIRECTORYTITLE);

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

	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_OUTPUT, parsingName.c_str());
}

void SplitFileDialog::OnSplitFinished()
{
	// Joining establishes publication of the result and releases the worker before
	// any dialog state is reset or the HWND can be destroyed.
	if (m_worker.joinable())
		m_worker.join();
	m_bSplittingFile = false;
	for (int id : { IDC_SPLIT_EDIT_SIZE, IDC_SPLIT_COMBOBOX_SIZES, IDC_SPLIT_EDIT_OUTPUT,
			 IDC_SPLIT_EDIT_OUTPUTFILENAME, IDC_SPLIT_BUTTON_OUTPUT })
		EnableWindow(GetDlgItem(m_hDlg, id), TRUE);
	SetDlgItemText(m_hDlg, IDOK, m_szOk);

	KillTimer(m_hDlg, ELPASED_TIMER_ID);
	auto message = m_resourceLoader->LoadString(
		m_result.Cancelled() ? IDS_SPLITFILEDIALOG_CANCELLED : IDS_SPLITFILEDIALOG_FINISHED);
	if (FAILED(m_result.error) && !m_result.Cancelled())
		message = FileStreamOperations::DescribeError(m_result);
	SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, message.c_str());

	if (SUCCEEDED(m_result.error))
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_SETPOS, 1000, 0);
	if ((FAILED(m_result.error) && !m_result.Cancelled()) || FAILED(m_result.cleanupError))
	{
		auto errorMessage = FileStreamOperations::DescribeError(m_result);
		MessageBox(m_hDlg, errorMessage.c_str(), AppInfo::NAME, MB_ICONERROR | MB_OK);
	}
	if (m_closePending)
		EndDialog(m_hDlg, 0);
}

SplitFileDialogPersistentSettings::SplitFileDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
	m_strSplitSize = _T("10");
	m_strSplitGroup = _T("KB");
}

SplitFileDialogPersistentSettings &SplitFileDialogPersistentSettings::GetInstance()
{
	static SplitFileDialogPersistentSettings sfadps;
	return sfadps;
}

void SplitFileDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::SaveString(hKey, SETTING_SIZE, m_strSplitSize);
	RegistrySettings::SaveString(hKey, SETTING_SIZE_GROUP, m_strSplitGroup);
}

void SplitFileDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::ReadString(hKey, SETTING_SIZE, m_strSplitSize);
	RegistrySettings::ReadString(hKey, SETTING_SIZE_GROUP, m_strSplitGroup);
}

void SplitFileDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SIZE, m_strSplitSize.c_str());
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SIZE_GROUP,
		m_strSplitGroup.c_str());
}

void SplitFileDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (lstrcmpi(bstrName, SETTING_SIZE) == 0)
	{
		m_strSplitSize = _bstr_t(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_SIZE_GROUP) == 0)
	{
		m_strSplitGroup = _bstr_t(bstrValue);
	}
}
