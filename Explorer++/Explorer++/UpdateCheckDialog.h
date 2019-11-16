// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Macros.h"

class CUpdateCheckDialog;

class CUpdateCheckDialogPersistentSettings : public CDialogSettings
{
public:

	static CUpdateCheckDialogPersistentSettings &GetInstance();

private:

	friend CUpdateCheckDialog;

	static const TCHAR SETTINGS_KEY[];

	CUpdateCheckDialogPersistentSettings();

	DISALLOW_COPY_AND_ASSIGN(CUpdateCheckDialogPersistentSettings);
};

class CUpdateCheckDialog : public CBaseDialog
{
public:

	CUpdateCheckDialog(HINSTANCE hInstance,int iResource,HWND hParent);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnTimer(int iTimerID);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

	void	SaveState();

	INT_PTR	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

	struct Version_t
	{
		int MajorVersion;
		int MinorVersion;
		int MicroVersion;

		WCHAR VersionString[32];
	};

	static const int WM_APP_UPDATE_CHECK_COMPLETE = WM_APP + 1;

	static const int UPDATE_CHECK_ERROR = 0;
	static const int UPDATE_CHECK_SUCCESS = 1;

	static const int STATUS_TIMER_ELAPSED = 800;

	static const TCHAR VERSION_FILE_URL[];

	static DWORD WINAPI	UpdateCheckThread(LPVOID pParam);
	static void			PerformUpdateCheck(HWND hDlg);

	void OnUpdateCheckError();
	void OnUpdateCheckSuccess(Version_t *Version);

	bool m_UpdateCheckComplete;

	CUpdateCheckDialogPersistentSettings	*m_pucdps;
};