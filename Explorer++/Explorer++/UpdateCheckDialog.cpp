// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * This dialog performs an update check (i.e. it checks
 * whether a new version is available). Note that this
 * dialog does not actually download a new version if
 * one is available; it simply links to it.
 */

#include "stdafx.h"
#include "UpdateCheckDialog.h"
#include "MainResource.h"
#include "Version.h"
#include <boost\algorithm\string.hpp>

#pragma warning(push)
#pragma warning(disable:4995)
#include <boost\lexical_cast.hpp>
#pragma warning(pop)

#include <stdexcept>
#include <vector>


const TCHAR UpdateCheckDialogPersistentSettings::SETTINGS_KEY[] = _T("UpdateCheck");
const TCHAR UpdateCheckDialog::VERSION_FILE_URL[] = _T("https://explorerplusplus.com/software/version.txt");

UpdateCheckDialog::UpdateCheckDialog(HINSTANCE hInstance, HWND hParent) :
	BaseDialog(hInstance, IDD_UPDATECHECK, hParent, false),
	m_UpdateCheckComplete(false)
{
	m_pucdps = &UpdateCheckDialogPersistentSettings::GetInstance();
}

INT_PTR UpdateCheckDialog::OnInitDialog()
{
	SetDlgItemText(m_hDlg,IDC_STATIC_CURRENT_VERSION,VERSION_STRING_W);

	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_UPDATE_CHECK_STATUS,szTemp,SIZEOF_ARRAY(szTemp));
	SetDlgItemText(m_hDlg,IDC_STATIC_UPDATE_STATUS,szTemp);

	SetTimer(m_hDlg,0,STATUS_TIMER_ELAPSED,NULL);

	/* The actual version check will be performed in a background
	thread (to avoid blocking the main thread while the version
	file is downloaded). */
	HANDLE hThread = CreateThread(NULL,0,UpdateCheckThread,
		reinterpret_cast<LPVOID>(m_hDlg),0,NULL);
	CloseHandle(hThread);

	m_pucdps->RestoreDialogPosition(m_hDlg,false);

	return 0;
}

DWORD WINAPI UpdateCheckDialog::UpdateCheckThread(LPVOID pParam)
{
	assert(pParam != NULL);

	PerformUpdateCheck(reinterpret_cast<HWND>(pParam));

	return 0;
}

void UpdateCheckDialog::PerformUpdateCheck(HWND hDlg)
{
	TCHAR TempPath[MAX_PATH];
	DWORD PathRes = GetTempPath(SIZEOF_ARRAY(TempPath),TempPath);

	if(PathRes == 0)
	{
		PostMessage(hDlg,UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
			UpdateCheckDialog::UPDATE_CHECK_ERROR,0);
		return;
	}

	TCHAR TempFileName[MAX_PATH];
	UINT FileRes = GetTempFileName(TempPath,_T("exp"),0,TempFileName);

	if(FileRes == 0)
	{
		PostMessage(hDlg,UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
			UpdateCheckDialog::UPDATE_CHECK_ERROR,0);
		return;
	}

	bool VersionRetrieved = false;

	/* Not that any cached version of this file
	will be deleted first. This ensures that the
	version check is not performed against an
	outdated file. */
	DeleteUrlCacheEntry(UpdateCheckDialog::VERSION_FILE_URL);
	HRESULT hr = URLDownloadToFile(NULL,UpdateCheckDialog::VERSION_FILE_URL,TempFileName,0,NULL);

	if(SUCCEEDED(hr))
	{
		HANDLE hFile = CreateFile(TempFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			char VersionNumber[16];
			DWORD NumBytesRead;
			BOOL ReadRes = ReadFile(hFile,VersionNumber,sizeof(VersionNumber) - 1,&NumBytesRead,NULL);

			if(ReadRes && NumBytesRead > 0)
			{
				VersionNumber[NumBytesRead] = '\0';

				std::string VersionNumberString(VersionNumber);
				std::vector<std::string> VersionNumberComponents;
				boost::split(VersionNumberComponents,VersionNumberString,boost::is_any_of("."));

				try
				{
					UpdateCheckDialog::Version_t Version;
					Version.MajorVersion = boost::lexical_cast<int>(VersionNumberComponents.at(0));
					Version.MinorVersion = boost::lexical_cast<int>(VersionNumberComponents.at(1));
					Version.MicroVersion = boost::lexical_cast<int>(VersionNumberComponents.at(2));
					MultiByteToWideChar(CP_ACP,0,VersionNumber,-1,Version.VersionString,SIZEOF_ARRAY(Version.VersionString));

					SendMessage(hDlg,UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
						UpdateCheckDialog::UPDATE_CHECK_SUCCESS,reinterpret_cast<LPARAM>(&Version));

					VersionRetrieved = true;
				}
				catch(std::out_of_range)
				{
					/* VersionRetrieved won't be set, so an error
					will be returned below. Nothing needs to be done
					here. */
				}
			}

			CloseHandle(hFile);
		}
	}

	if(!VersionRetrieved)
	{
		PostMessage(hDlg,UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
			UpdateCheckDialog::UPDATE_CHECK_ERROR,0);
	}

	DeleteFile(TempFileName);
}

INT_PTR UpdateCheckDialog::OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_APP_UPDATE_CHECK_COMPLETE:
		KillTimer(m_hDlg,0);

		/* A WM_TIMER message may already be in the
		message queue. Set a flag here to indicate
		that the update check has already completed. */
		m_UpdateCheckComplete = true;

		switch(wParam)
		{
		case UPDATE_CHECK_ERROR:
			OnUpdateCheckError();
			break;

		case UPDATE_CHECK_SUCCESS:
			OnUpdateCheckSuccess(reinterpret_cast<Version_t *>(lParam));
			break;
		}
		break;
	}

	return 0;
}

void UpdateCheckDialog::OnUpdateCheckError()
{
	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_UPDATE_CHECK_ERROR,szTemp,SIZEOF_ARRAY(szTemp));
	SetDlgItemText(m_hDlg,IDC_STATIC_UPDATE_STATUS,szTemp);
}

void UpdateCheckDialog::OnUpdateCheckSuccess(Version_t *Version)
{
	TCHAR szStatus[128];
	TCHAR szTemp[128];

	if((Version->MajorVersion > MAJOR_VERSION) ||
		(Version->MajorVersion == MAJOR_VERSION && Version->MinorVersion > MINOR_VERSION) ||
		(Version->MajorVersion == MAJOR_VERSION && Version->MinorVersion == MINOR_VERSION && Version->MicroVersion > MICRO_VERSION))
	{
		LoadString(GetInstance(),IDS_UPDATE_CHECK_NEW_VERSION_AVAILABLE,szTemp,SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),szTemp,Version->VersionString);
	}
	else
	{
		LoadString(GetInstance(),IDS_UPDATE_CHECK_UP_TO_DATE,szTemp,SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szStatus,SIZEOF_ARRAY(szStatus),szTemp,Version->VersionString);
	}

	SetDlgItemText(m_hDlg,IDC_STATIC_UPDATE_STATUS,szStatus);
}

INT_PTR UpdateCheckDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(LOWORD(wParam))
	{
	case IDOK:
		EndDialog(m_hDlg,1);
		break;

	case IDCANCEL:
		EndDialog(m_hDlg,0);
		break;
	}

	return 0;
}

INT_PTR UpdateCheckDialog::OnTimer(int iTimerID)
{
	UNREFERENCED_PARAMETER(iTimerID);

	if(m_UpdateCheckComplete)
	{
		return 0;
	}

	TCHAR UpdateStatus[64];
	LoadString(GetInstance(),IDS_UPDATE_CHECK_STATUS,UpdateStatus,SIZEOF_ARRAY(UpdateStatus));

	static int Step = 0;

	for(int i = 0;i < Step;i++)
	{
		StringCchCat(UpdateStatus,SIZEOF_ARRAY(UpdateStatus),_T("."));
	}

	SetDlgItemText(m_hDlg,IDC_STATIC_UPDATE_STATUS,UpdateStatus);

	Step++;

	if(Step > 3)
	{
		Step = 0;
	}

	return 0;
}

INT_PTR UpdateCheckDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_SYSLINK_DOWNLOAD))
		{
			PNMLINK pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);
			ShellExecute(NULL,L"open",pnmlink->item.szUrl,NULL,NULL,SW_SHOW);
		}
		break;
	}

	return 0;
}

INT_PTR UpdateCheckDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void UpdateCheckDialog::SaveState()
{
	m_pucdps->SaveDialogPosition(m_hDlg);

	m_pucdps->m_bStateSaved = TRUE;
}

UpdateCheckDialogPersistentSettings::UpdateCheckDialogPersistentSettings() :
DialogSettings(SETTINGS_KEY)
{
	
}

UpdateCheckDialogPersistentSettings& UpdateCheckDialogPersistentSettings::GetInstance()
{
	static UpdateCheckDialogPersistentSettings ucdps;
	return ucdps;
}