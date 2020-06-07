// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Macros.h"

class UpdateCheckDialog;

class UpdateCheckDialogPersistentSettings : public DialogSettings
{
public:
	static UpdateCheckDialogPersistentSettings &GetInstance();

private:
	friend UpdateCheckDialog;

	static const TCHAR SETTINGS_KEY[];

	UpdateCheckDialogPersistentSettings();

	DISALLOW_COPY_AND_ASSIGN(UpdateCheckDialogPersistentSettings);
};

class UpdateCheckDialog : public BaseDialog
{
public:
	UpdateCheckDialog(HINSTANCE hInstance, HWND hParent);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnTimer(int iTimerID) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

	void SaveState() override;

	INT_PTR OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
	struct Version
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

	static DWORD WINAPI UpdateCheckThread(LPVOID pParam);
	static void PerformUpdateCheck(HWND hDlg);

	void OnUpdateCheckError();
	void OnUpdateCheckSuccess(Version *version);

	bool m_UpdateCheckComplete;

	UpdateCheckDialogPersistentSettings *m_pucdps;
};