// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SplitFileDialog.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"
#include <boost/scope_exit.hpp>
#include <comdef.h>
#include <unordered_map>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

namespace NSplitFileDialog
{
	const int		WM_APP_SETTOTALSPLITCOUNT	= WM_APP + 1;
	const int		WM_APP_SETCURRENTSPLITCOUNT	= WM_APP + 2;
	const int		WM_APP_SPLITFINISHED		= WM_APP + 3;
	const int		WM_APP_INPUTFILEINVALID		= WM_APP + 4;

	const TCHAR		COUNTER_PATTERN[] = _T("/N");

	DWORD WINAPI	SplitFileThreadProcStub(LPVOID pParam);
}

const TCHAR CSplitFileDialogPersistentSettings::SETTINGS_KEY[] = _T("SplitFile");

const TCHAR CSplitFileDialogPersistentSettings::SETTING_SIZE[] = _T("Size");
const TCHAR CSplitFileDialogPersistentSettings::SETTING_SIZE_GROUP[] = _T("SizeGroup");

CSplitFileDialog::CSplitFileDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,std::wstring strFullFilename) :
CBaseDialog(hInstance,iResource,hParent,false)
{
	m_strFullFilename	= strFullFilename;
	m_bSplittingFile	= false;
	m_bStopSplitting	= false;
	m_CurrentError		= ERROR_NONE;
	m_pSplitFile		= NULL;

	m_psfdps = &CSplitFileDialogPersistentSettings::GetInstance();
}

CSplitFileDialog::~CSplitFileDialog()
{
	if(m_pSplitFile != NULL)
	{
		m_pSplitFile->StopSplitting();
		m_pSplitFile->Release();
	}
}

INT_PTR CSplitFileDialog::OnInitDialog()
{
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hDlg);
	m_icon = IconResourceLoader::LoadIconFromPNGForDpi(Icon::SplitFiles, DIALOG_ICON_SIZE_96DPI, dpi);
	SetClassLongPtr(m_hDlg, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_icon.get()));

	SHFILEINFO shfi;
	DWORD_PTR dwRes = SHGetFileInfo(m_strFullFilename.c_str(),0,&shfi,sizeof(shfi),SHGFI_ICON);

	if(dwRes != 0)
	{
		ICONINFO ii;
		GetIconInfo(shfi.hIcon,&ii);
		SendDlgItemMessage(m_hDlg,IDC_SPLIT_STATIC_ICON,STM_SETIMAGE,
			IMAGE_BITMAP,reinterpret_cast<LPARAM>(ii.hbmColor));

		DeleteObject(ii.hbmColor);
		DeleteObject(ii.hbmMask);
		DestroyIcon(shfi.hIcon);
	}

	SetDlgItemText(m_hDlg,IDC_SPLIT_EDIT_FILENAME,m_strFullFilename.c_str());

	HANDLE hFile = CreateFile(m_strFullFilename.c_str(),GENERIC_READ,
		FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER lFileSize;
		GetFileSizeEx(hFile,&lFileSize);

		ULARGE_INTEGER ulFileSize;
		ulFileSize.QuadPart = lFileSize.QuadPart;

		TCHAR szFileSize[32];
		FormatSizeString(ulFileSize,szFileSize,SIZEOF_ARRAY(szFileSize));
		SetDlgItemText(m_hDlg,IDC_SPLIT_EDIT_FILESIZE,szFileSize);

		CloseHandle(hFile);
	}

	TCHAR szOutputDirectory[MAX_PATH];
	StringCchCopy(szOutputDirectory,SIZEOF_ARRAY(szOutputDirectory),m_strFullFilename.c_str());
	PathRemoveFileSpec(szOutputDirectory);
	SetDlgItemText(m_hDlg,IDC_SPLIT_EDIT_OUTPUT,szOutputDirectory);

	HWND hComboBox = GetDlgItem(m_hDlg,IDC_SPLIT_COMBOBOX_SIZES);
	int iPos;

	TCHAR szTemp[64];

	LoadString(GetInstance(),IDS_SPLIT_FILE_SIZE_BYTES,szTemp,SIZEOF_ARRAY(szTemp));
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szTemp)));
	m_SizeMap.insert(std::unordered_map<int,SizeType_t>::value_type(iPos,SIZE_TYPE_BYTES));
	LoadString(GetInstance(),IDS_SPLIT_FILE_SIZE_KB,szTemp,SIZEOF_ARRAY(szTemp));
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szTemp)));
	m_SizeMap.insert(std::unordered_map<int,SizeType_t>::value_type(iPos,SIZE_TYPE_KB));
	LoadString(GetInstance(),IDS_SPLIT_FILE_SIZE_MB,szTemp,SIZEOF_ARRAY(szTemp));
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szTemp)));
	m_SizeMap.insert(std::unordered_map<int,SizeType_t>::value_type(iPos,SIZE_TYPE_MB));
	LoadString(GetInstance(),IDS_SPLIT_FILE_SIZE_GB,szTemp,SIZEOF_ARRAY(szTemp));
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szTemp)));
	m_SizeMap.insert(std::unordered_map<int,SizeType_t>::value_type(iPos,SIZE_TYPE_GB));

	SendMessage(hComboBox,CB_SELECTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(m_psfdps->m_strSplitGroup.c_str()));

	HWND hEditSize = GetDlgItem(m_hDlg,IDC_SPLIT_EDIT_SIZE);
	SetWindowText(hEditSize,m_psfdps->m_strSplitSize.c_str());
	SendMessage(hEditSize,EM_SETSEL,0,-1);
	SetFocus(hEditSize);

	TCHAR szOutputFilename[MAX_PATH];
	StringCchCopy(szOutputFilename,SIZEOF_ARRAY(szOutputFilename),m_strFullFilename.c_str());
	PathStripPath(szOutputFilename);
	StringCchPrintf(szOutputFilename,SIZEOF_ARRAY(szOutputFilename),_T("%s.part%s"),
		szOutputFilename,NSplitFileDialog::COUNTER_PATTERN);
	SetDlgItemText(m_hDlg,IDC_SPLIT_EDIT_OUTPUTFILENAME,szOutputFilename);

	HFONT hCurentFont = reinterpret_cast<HFONT>(SendDlgItemMessage(m_hDlg,
		IDC_SPLIT_STATIC_FILENAMEHELPER,WM_GETFONT,0,0));

	LOGFONT lf;
	GetObject(hCurentFont,sizeof(lf),reinterpret_cast<LPVOID>(&lf));

	lf.lfItalic = TRUE;
	m_hHelperTextFont = CreateFontIndirect(&lf);

	SendDlgItemMessage(m_hDlg,IDC_SPLIT_STATIC_FILENAMEHELPER,
		WM_SETFONT,reinterpret_cast<WPARAM>(m_hHelperTextFont),MAKEWORD(TRUE,0));

	SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_ELAPSEDTIME,_T("00:00:00"));

	m_psfdps->RestoreDialogPosition(m_hDlg,false);

	return 0;
}

INT_PTR CSplitFileDialog::OnTimer(int iTimerID)
{
	if(iTimerID == ELPASED_TIMER_ID)
	{
		m_uElapsedTime++;

		/* Update the elapsed time display (form is hh:mm:ss). */
		TCHAR szElapsedTime[9];
		StringCchPrintf(szElapsedTime,SIZEOF_ARRAY(szElapsedTime),
			_T("%02d:%02d:%02d"),m_uElapsedTime / 3600,(m_uElapsedTime / 60) % 60,
			m_uElapsedTime % 60);
		SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_ELAPSEDTIME,szElapsedTime);
	}

	return 0;
}

INT_PTR CSplitFileDialog::OnCtlColorStatic(HWND hwnd,HDC hdc)
{
	if(hwnd == GetDlgItem(m_hDlg,IDC_SPLIT_STATIC_FILENAMEHELPER))
	{
		/* Set a custom text color for the helper text. */
		SetTextColor(hdc,HELPER_TEXT_COLOR);
		SetBkMode(hdc,TRANSPARENT);
		return reinterpret_cast<INT_PTR>(GetStockObject(NULL_BRUSH));
	}

	return 0;
}

INT_PTR CSplitFileDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if(HIWORD(wParam) != 0)
	{
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			{
				bool bHideError = false;

				switch(m_CurrentError)
				{
				case ERROR_OUTPUT_FILENAME_EMPTY:
					bHideError = (LOWORD(wParam) == IDC_SPLIT_EDIT_OUTPUTFILENAME);
					break;

				case ERROR_OUTPUT_FILENAME_CONSTANT:
					bHideError = (LOWORD(wParam) == IDC_SPLIT_EDIT_OUTPUTFILENAME);
					break;

				case ERROR_OUTPUT_DIRECTORY_EMPTY:
					bHideError = (LOWORD(wParam) == IDC_SPLIT_EDIT_OUTPUT);
					break;

				case ERROR_SPLIT_SIZE:
					bHideError = (LOWORD(wParam) == IDC_SPLIT_EDIT_SIZE);
					break;
				}

				if(bHideError)
				{
					/* If an error is currently been shown, and it is
					for the control this notification is been sent for,
					hide the error message. */
					SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,EMPTY_STRING);

					m_CurrentError = ERROR_NONE;
				}
			}
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
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
	}

	return 0;
}

INT_PTR CSplitFileDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

INT_PTR CSplitFileDialog::OnDestroy()
{
	DeleteObject(m_hHelperTextFont);

	return 0;
}

void CSplitFileDialog::SaveState()
{
	m_psfdps->SaveDialogPosition(m_hDlg);

	GetWindowString(GetDlgItem(m_hDlg,IDC_SPLIT_EDIT_SIZE),m_psfdps->m_strSplitSize);
	GetWindowString(GetDlgItem(m_hDlg,IDC_SPLIT_COMBOBOX_SIZES),m_psfdps->m_strSplitGroup);

	m_psfdps->m_bStateSaved = TRUE;
}

INT_PTR CSplitFileDialog::OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(uMsg)
	{
	case NSplitFileDialog::WM_APP_SETTOTALSPLITCOUNT:
		SendDlgItemMessage(m_hDlg,IDC_SPLIT_PROGRESS,PBM_SETRANGE32,0,wParam);
		break;

	case NSplitFileDialog::WM_APP_SETCURRENTSPLITCOUNT:
		SendDlgItemMessage(m_hDlg,IDC_SPLIT_PROGRESS,PBM_SETPOS,wParam,0);
		break;

	case NSplitFileDialog::WM_APP_SPLITFINISHED:
		OnSplitFinished();
		break;

	case NSplitFileDialog::WM_APP_INPUTFILEINVALID:
		{
			TCHAR szTemp[128];
			LoadString(GetInstance(),IDS_SPLITFILEDIALOG_INPUTFILEINVALID,
				szTemp,SIZEOF_ARRAY(szTemp));
			SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,szTemp);

			assert(m_pSplitFile != NULL);

			m_pSplitFile->Release();
			m_pSplitFile = NULL;

			m_bSplittingFile = false;
			m_bStopSplitting = false;

			KillTimer(m_hDlg,ELPASED_TIMER_ID);

			SetDlgItemText(m_hDlg,IDOK,m_szOk);
		}
		break;
	}

	return 0;
}

void CSplitFileDialog::OnOk()
{
	if(!m_bSplittingFile)
	{
		HWND hOutputFilename = GetDlgItem(m_hDlg,IDC_SPLIT_EDIT_OUTPUTFILENAME);

		if(GetWindowTextLength(hOutputFilename) == 0)
		{
			TCHAR szTemp[128];

			LoadString(GetInstance(),IDS_SPLITFILEDIALOG_OUTPUTFILENAMEERROR,
				szTemp,SIZEOF_ARRAY(szTemp));

			SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,szTemp);

			m_CurrentError = ERROR_OUTPUT_FILENAME_EMPTY;

			SetFocus(hOutputFilename);
			return;
		}

		std::wstring strOutputFilename;
		GetWindowString(hOutputFilename,strOutputFilename);

		/* Now, check that the filename has a variable component. Without the
		variable component, the filenames of all the split files would be exactly
		the same. */
		if(strOutputFilename.find(NSplitFileDialog::COUNTER_PATTERN) == std::wstring::npos)
		{
			TCHAR szTemp[128];

			LoadString(GetInstance(),IDS_SPLITFILEDIALOG_OUTPUTFILENAMECONSTANTERROR,
				szTemp,SIZEOF_ARRAY(szTemp));

			SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,szTemp);

			m_CurrentError = ERROR_OUTPUT_FILENAME_CONSTANT;

			SetFocus(hOutputFilename);
			return;
		}

		HWND hEditOutputDirectory = GetDlgItem(m_hDlg,IDC_SPLIT_EDIT_OUTPUT);

		if(GetWindowTextLength(hEditOutputDirectory) == 0)
		{
			TCHAR szTemp[128];

			LoadString(GetInstance(),IDS_SPLITFILEDIALOG_OUTPUTDIRECTORYERROR,
				szTemp,SIZEOF_ARRAY(szTemp));

			SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,szTemp);

			m_CurrentError = ERROR_OUTPUT_DIRECTORY_EMPTY;

			SetFocus(hEditOutputDirectory);
			return;
		}

		std::wstring strOutputDirectory;
		GetWindowString(hEditOutputDirectory,strOutputDirectory);

		BOOL bTranslated;
		UINT uSplitSize = GetDlgItemInt(m_hDlg,IDC_SPLIT_EDIT_SIZE,&bTranslated,FALSE);

		if(!bTranslated)
		{
			TCHAR szTemp[128];

			LoadString(GetInstance(),IDS_SPLITFILEDIALOG_SIZEERROR,
				szTemp,SIZEOF_ARRAY(szTemp));

			SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,szTemp);

			m_CurrentError = ERROR_SPLIT_SIZE;

			SetFocus(GetDlgItem(m_hDlg,IDC_SPLIT_EDIT_SIZE));
			return;
		}

		HWND hComboBox = GetDlgItem(m_hDlg,IDC_SPLIT_COMBOBOX_SIZES);
		int iCurSel = static_cast<int>(SendMessage(hComboBox,CB_GETCURSEL,0,0));

		auto itr = m_SizeMap.find(iCurSel);

		if(itr != m_SizeMap.end())
		{
			switch(itr->second)
			{
			case SIZE_TYPE_BYTES:
				/* Nothing needs to be done, as the selection
				is in bytes. */
				break;

			case SIZE_TYPE_KB:
				uSplitSize *= KB;
				break;

			case SIZE_TYPE_MB:
				uSplitSize *= MB;
				break;

			case SIZE_TYPE_GB:
				uSplitSize *= GB;
				break;
			}
		}

		m_pSplitFile = new CSplitFile(m_hDlg,m_strFullFilename,strOutputFilename,
			strOutputDirectory,uSplitSize);

		GetDlgItemText(m_hDlg,IDOK,m_szOk,SIZEOF_ARRAY(m_szOk));

		TCHAR szTemp[64];
		LoadString(GetInstance(),IDS_STOP,szTemp,SIZEOF_ARRAY(szTemp));
		SetDlgItemText(m_hDlg,IDOK,szTemp);

		m_bSplittingFile = true;

		m_uElapsedTime = 0;
		SetTimer(m_hDlg,ELPASED_TIMER_ID,ELPASED_TIMER_TIMEOUT,NULL);

		LoadString(GetInstance(),IDS_SPLITFILEDIALOG_SPLITTING,
			szTemp,SIZEOF_ARRAY(szTemp));
		SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,szTemp);

		HANDLE hThread = CreateThread(NULL,0,NSplitFileDialog::SplitFileThreadProcStub,
			reinterpret_cast<LPVOID>(m_pSplitFile),0,NULL);
		SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
		CloseHandle(hThread);
	}
	else
	{
		m_bStopSplitting = true;

		if(m_pSplitFile != NULL)
		{
			m_pSplitFile->StopSplitting();
		}
	}
}

void CSplitFileDialog::OnCancel()
{
	if(m_bSplittingFile)
	{
		m_bStopSplitting = true;
	}
	else
	{
		EndDialog(m_hDlg,0);
	}
}

void CSplitFileDialog::OnChangeOutputDirectory()
{
	TCHAR szTitle[128];
	LoadString(GetInstance(),IDS_SPLITFILEDIALOG_DIRECTORYTITLE,
		szTitle,SIZEOF_ARRAY(szTitle));

	LPITEMIDLIST pidl;
	BOOL bSucceeded = NFileOperations::CreateBrowseDialog(m_hDlg,szTitle,&pidl);

	if (!bSucceeded)
	{
		return;
	}

	BOOST_SCOPE_EXIT(pidl) {
		CoTaskMemFree(pidl);
	} BOOST_SCOPE_EXIT_END

	TCHAR parsingName[MAX_PATH];
	HRESULT hr = GetDisplayName(pidl, parsingName, SIZEOF_ARRAY(parsingName), SHGDN_FORPARSING);

	if (FAILED(hr))
	{
		return;
	}

	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_OUTPUT, parsingName);
}

void CSplitFileDialog::OnSplitFinished()
{
	TCHAR szTemp[64];

	if(!m_bStopSplitting)
	{
		LoadString(GetInstance(),IDS_SPLITFILEDIALOG_FINISHED,
			szTemp,SIZEOF_ARRAY(szTemp));
	}
	else
	{
		LoadString(GetInstance(),IDS_SPLITFILEDIALOG_CANCELLED,
			szTemp,SIZEOF_ARRAY(szTemp));
	}

	SetDlgItemText(m_hDlg,IDC_SPLIT_STATIC_MESSAGE,szTemp);

	assert(m_pSplitFile != NULL);

	m_pSplitFile->Release();
	m_pSplitFile = NULL;

	m_bSplittingFile = false;
	m_bStopSplitting = false;

	KillTimer(m_hDlg,ELPASED_TIMER_ID);

	int iHighLimit = static_cast<int>(SendDlgItemMessage(m_hDlg,IDC_SPLIT_PROGRESS,PBM_GETRANGE,FALSE,0));
	SendDlgItemMessage(m_hDlg,IDC_SPLIT_PROGRESS,PBM_SETPOS,iHighLimit,0);

	SetDlgItemText(m_hDlg,IDOK,m_szOk);
}

DWORD WINAPI NSplitFileDialog::SplitFileThreadProcStub(LPVOID pParam)
{
	assert(pParam != NULL);

	CSplitFile *pSplitFile = reinterpret_cast<CSplitFile *>(pParam);
	pSplitFile->SplitFile();

	return 0;
}

CSplitFile::CSplitFile(HWND hDlg,std::wstring strFullFilename,
	std::wstring strOutputFilename,std::wstring strOutputDirectory,
	UINT uSplitSize)
{
	m_hDlg					= hDlg;
	m_strFullFilename		= strFullFilename;
	m_strOutputFilename		= strOutputFilename;
	m_strOutputDirectory	= strOutputDirectory;
	m_uSplitSize			= uSplitSize;

	m_bStopSplitting		= false;

	InitializeCriticalSection(&m_csStop);
}

CSplitFile::~CSplitFile()
{
	DeleteCriticalSection(&m_csStop);
}

void CSplitFile::SplitFile()
{
	HANDLE hInputFile = CreateFile(m_strFullFilename.c_str(),GENERIC_READ,
		FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);

	if(hInputFile == INVALID_HANDLE_VALUE)
	{
		PostMessage(m_hDlg,NSplitFileDialog::WM_APP_INPUTFILEINVALID,0,0);
		return;
	}

	LARGE_INTEGER lFileSize;
	GetFileSizeEx(hInputFile,&lFileSize);

	LONGLONG nSplits = lFileSize.QuadPart / m_uSplitSize;

	if((lFileSize.QuadPart % m_uSplitSize) != 0)
		nSplits++;

	PostMessage(m_hDlg,NSplitFileDialog::WM_APP_SETTOTALSPLITCOUNT,
		static_cast<WPARAM>(nSplits),0);

	SplitFileInternal(hInputFile,lFileSize);

	CloseHandle(hInputFile);

	SendMessage(m_hDlg,NSplitFileDialog::WM_APP_SPLITFINISHED,0,0);
}

void CSplitFile::SplitFileInternal(HANDLE hInputFile,const LARGE_INTEGER &lFileSize)
{
	LARGE_INTEGER lRunningSplitSize = {0};

	char *pBuffer = new char[m_uSplitSize];
	bool bStop = false;
	int nSplitsMade = 1;

	while(lRunningSplitSize.QuadPart < lFileSize.QuadPart &&
		!bStop)
	{
		DWORD dwNumberOfBytesRead;
		ReadFile(hInputFile,reinterpret_cast<LPVOID>(pBuffer),m_uSplitSize,&dwNumberOfBytesRead,NULL);

		std::wstring strOutputFullFilename;
		ProcessFilename(nSplitsMade,strOutputFullFilename);

		HANDLE hOutputFile = CreateFile(strOutputFullFilename.c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,NULL);

		if(hOutputFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwNumberOfBytesWritten;
			WriteFile(hOutputFile,reinterpret_cast<LPCVOID>(pBuffer),dwNumberOfBytesRead,
				&dwNumberOfBytesWritten,NULL);

			CloseHandle(hOutputFile);
		}

		/* TODO: Wait for a set period of time before sending message
		(so as not to block the GUI). */
		PostMessage(m_hDlg,NSplitFileDialog::WM_APP_SETCURRENTSPLITCOUNT,nSplitsMade,0);

		lRunningSplitSize.QuadPart += dwNumberOfBytesRead;
		nSplitsMade++;

		EnterCriticalSection(&m_csStop);
		if(m_bStopSplitting)
		{
			bStop = true;
		}
		LeaveCriticalSection(&m_csStop);
	}

	delete[] pBuffer;
}

void CSplitFile::ProcessFilename(int nSplitsMade,std::wstring &strOutputFullFilename)
{
	std::wstring strOutputFilename = m_strOutputFilename;

	std::wstringstream ss;
	ss << nSplitsMade;
	strOutputFilename.replace(strOutputFilename.find(NSplitFileDialog::COUNTER_PATTERN),2,ss.str());

	strOutputFullFilename = m_strOutputDirectory + _T("\\") + strOutputFilename;
}

void CSplitFile::StopSplitting()
{
	EnterCriticalSection(&m_csStop);
	m_bStopSplitting = true;
	LeaveCriticalSection(&m_csStop);
}

CSplitFileDialogPersistentSettings::CSplitFileDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_strSplitSize = _T("10");
	m_strSplitGroup = _T("KB");
}

CSplitFileDialogPersistentSettings::~CSplitFileDialogPersistentSettings()
{
	
}

CSplitFileDialogPersistentSettings& CSplitFileDialogPersistentSettings::GetInstance()
{
	static CSplitFileDialogPersistentSettings sfadps;
	return sfadps;
}

void CSplitFileDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveStringToRegistry(hKey, SETTING_SIZE, m_strSplitSize.c_str());
	NRegistrySettings::SaveStringToRegistry(hKey, SETTING_SIZE_GROUP, m_strSplitGroup.c_str());
}

void CSplitFileDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadStringFromRegistry(hKey, SETTING_SIZE, m_strSplitSize);
	NRegistrySettings::ReadStringFromRegistry(hKey, SETTING_SIZE_GROUP, m_strSplitGroup);
}

void CSplitFileDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SIZE, m_strSplitSize.c_str());
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SIZE_GROUP, m_strSplitGroup.c_str());
}

void CSplitFileDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName, SETTING_SIZE) == 0)
	{
		m_strSplitSize = _bstr_t(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_SIZE_GROUP) == 0)
	{
		m_strSplitGroup = _bstr_t(bstrValue);
	}
}