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
#pragma warning(disable : 4995)
#include <boost\lexical_cast.hpp>
#pragma warning(pop)

#include <stdexcept>
#include <vector>

const TCHAR UpdateCheckDialogPersistentSettings::SETTINGS_KEY[] = _T("UpdateCheck");
const TCHAR UpdateCheckDialog::VERSION_FILE_URL[] =
	_T("https://explorerplusplus.com/software/version.txt");

UpdateCheckDialog::UpdateCheckDialog(HINSTANCE hInstance, HWND hParent) :
	BaseDialog(hInstance, IDD_UPDATECHECK, hParent, false),
	m_UpdateCheckComplete(false)
{
	m_pucdps = &UpdateCheckDialogPersistentSettings::GetInstance();
}

INT_PTR UpdateCheckDialog::OnInitDialog()
{
	SetDlgItemText(m_hDlg, IDC_STATIC_CURRENT_VERSION, VERSION_STRING_W);

	TCHAR szTemp[64];
	LoadString(GetInstance(), IDS_UPDATE_CHECK_STATUS, szTemp, SIZEOF_ARRAY(szTemp));
	SetDlgItemText(m_hDlg, IDC_STATIC_UPDATE_STATUS, szTemp);

	SetTimer(m_hDlg, 0, STATUS_TIMER_ELAPSED, nullptr);

	/* The actual version check will be performed in a background
	thread (to avoid blocking the main thread while the version
	file is downloaded). */
	HANDLE hThread =
		CreateThread(nullptr, 0, UpdateCheckThread, reinterpret_cast<LPVOID>(m_hDlg), 0, nullptr);
	CloseHandle(hThread);

	m_pucdps->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

DWORD WINAPI UpdateCheckDialog::UpdateCheckThread(LPVOID pParam)
{
	assert(pParam != nullptr);

	PerformUpdateCheck(reinterpret_cast<HWND>(pParam));

	return 0;
}

void UpdateCheckDialog::PerformUpdateCheck(HWND hDlg)
{
	TCHAR tempPath[MAX_PATH];
	DWORD pathRes = GetTempPath(SIZEOF_ARRAY(tempPath), tempPath);

	if (pathRes == 0)
	{
		PostMessage(hDlg, UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
			UpdateCheckDialog::UPDATE_CHECK_ERROR, 0);
		return;
	}

	TCHAR tempFileName[MAX_PATH];
	UINT fileRes = GetTempFileName(tempPath, _T("exp"), 0, tempFileName);

	if (fileRes == 0)
	{
		PostMessage(hDlg, UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
			UpdateCheckDialog::UPDATE_CHECK_ERROR, 0);
		return;
	}

	bool versionRetrieved = false;

	/* Not that any cached version of this file
	will be deleted first. This ensures that the
	version check is not performed against an
	outdated file. */
	DeleteUrlCacheEntry(UpdateCheckDialog::VERSION_FILE_URL);
	HRESULT hr =
		URLDownloadToFile(nullptr, UpdateCheckDialog::VERSION_FILE_URL, tempFileName, 0, nullptr);

	if (SUCCEEDED(hr))
	{
		HANDLE hFile =
			CreateFile(tempFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			char versionNumber[16];
			DWORD numBytesRead;
			BOOL readRes =
				ReadFile(hFile, versionNumber, sizeof(versionNumber) - 1, &numBytesRead, nullptr);

			if (readRes && numBytesRead > 0)
			{
				versionNumber[numBytesRead] = '\0';

				std::string versionNumberString(versionNumber);
				std::vector<std::string> versionNumberComponents;
				boost::split(versionNumberComponents, versionNumberString, boost::is_any_of("."));

				try
				{
					UpdateCheckDialog::Version version;
					version.MajorVersion = boost::lexical_cast<int>(versionNumberComponents.at(0));
					version.MinorVersion = boost::lexical_cast<int>(versionNumberComponents.at(1));
					version.MicroVersion = boost::lexical_cast<int>(versionNumberComponents.at(2));
					MultiByteToWideChar(CP_ACP, 0, versionNumber, -1, version.VersionString,
						SIZEOF_ARRAY(version.VersionString));

					SendMessage(hDlg, UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
						UpdateCheckDialog::UPDATE_CHECK_SUCCESS,
						reinterpret_cast<LPARAM>(&version));

					versionRetrieved = true;
				}
				catch (std::out_of_range)
				{
					/* VersionRetrieved won't be set, so an error
					will be returned below. Nothing needs to be done
					here. */
				}
			}

			CloseHandle(hFile);
		}
	}

	if (!versionRetrieved)
	{
		PostMessage(hDlg, UpdateCheckDialog::WM_APP_UPDATE_CHECK_COMPLETE,
			UpdateCheckDialog::UPDATE_CHECK_ERROR, 0);
	}

	DeleteFile(tempFileName);
}

INT_PTR UpdateCheckDialog::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_APP_UPDATE_CHECK_COMPLETE:
		KillTimer(m_hDlg, 0);

		/* A WM_TIMER message may already be in the
		message queue. Set a flag here to indicate
		that the update check has already completed. */
		m_UpdateCheckComplete = true;

		switch (wParam)
		{
		case UPDATE_CHECK_ERROR:
			OnUpdateCheckError();
			break;

		case UPDATE_CHECK_SUCCESS:
			OnUpdateCheckSuccess(reinterpret_cast<Version *>(lParam));
			break;
		}
		break;
	}

	return 0;
}

void UpdateCheckDialog::OnUpdateCheckError()
{
	TCHAR szTemp[64];
	LoadString(GetInstance(), IDS_UPDATE_CHECK_ERROR, szTemp, SIZEOF_ARRAY(szTemp));
	SetDlgItemText(m_hDlg, IDC_STATIC_UPDATE_STATUS, szTemp);
}

void UpdateCheckDialog::OnUpdateCheckSuccess(Version *version)
{
	TCHAR szStatus[128];
	TCHAR szTemp[128];

	if ((version->MajorVersion > MAJOR_VERSION)
		|| (version->MajorVersion == MAJOR_VERSION && version->MinorVersion > MINOR_VERSION)
		|| (version->MajorVersion == MAJOR_VERSION && version->MinorVersion == MINOR_VERSION
			&& version->MicroVersion > MICRO_VERSION))
	{
		LoadString(
			GetInstance(), IDS_UPDATE_CHECK_NEW_VERSION_AVAILABLE, szTemp, SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szStatus, SIZEOF_ARRAY(szStatus), szTemp, version->VersionString);
	}
	else
	{
		LoadString(GetInstance(), IDS_UPDATE_CHECK_UP_TO_DATE, szTemp, SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szStatus, SIZEOF_ARRAY(szStatus), szTemp, version->VersionString);
	}

	SetDlgItemText(m_hDlg, IDC_STATIC_UPDATE_STATUS, szStatus);
}

INT_PTR UpdateCheckDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDOK:
		EndDialog(m_hDlg, 1);
		break;

	case IDCANCEL:
		EndDialog(m_hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR UpdateCheckDialog::OnTimer(int iTimerID)
{
	UNREFERENCED_PARAMETER(iTimerID);

	if (m_UpdateCheckComplete)
	{
		return 0;
	}

	TCHAR updateStatus[64];
	LoadString(GetInstance(), IDS_UPDATE_CHECK_STATUS, updateStatus, SIZEOF_ARRAY(updateStatus));

	static int step = 0;

	for (int i = 0; i < step; i++)
	{
		StringCchCat(updateStatus, SIZEOF_ARRAY(updateStatus), _T("."));
	}

	SetDlgItemText(m_hDlg, IDC_STATIC_UPDATE_STATUS, updateStatus);

	step++;

	if (step > 3)
	{
		step = 0;
	}

	return 0;
}

INT_PTR UpdateCheckDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		if (pnmhdr->hwndFrom == GetDlgItem(m_hDlg, IDC_SYSLINK_DOWNLOAD))
		{
			auto pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);
			ShellExecute(nullptr, L"open", pnmlink->item.szUrl, nullptr, nullptr, SW_SHOW);
		}
		break;
	}

	return 0;
}

INT_PTR UpdateCheckDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
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

UpdateCheckDialogPersistentSettings &UpdateCheckDialogPersistentSettings::GetInstance()
{
	static UpdateCheckDialogPersistentSettings ucdps;
	return ucdps;
}