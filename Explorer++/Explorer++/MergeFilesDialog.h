// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"
#include "../Helper/ResizableDialogHelper.h"
#include <string>
#include <vector>

class MergeFilesDialog;

class MergeFilesDialogPersistentSettings : public DialogSettings
{
public:
	static MergeFilesDialogPersistentSettings &GetInstance();

private:
	friend MergeFilesDialog;

	static const TCHAR SETTINGS_KEY[];

	MergeFilesDialogPersistentSettings();

	MergeFilesDialogPersistentSettings(const MergeFilesDialogPersistentSettings &);
	MergeFilesDialogPersistentSettings &operator=(const MergeFilesDialogPersistentSettings &);
};

class MergeFiles : public ReferenceCount
{
public:
	MergeFiles(HWND hDlg, const std::wstring &strOutputFilename,
		const std::vector<std::wstring> &filePaths);
	~MergeFiles();

	void StartMerging();
	void StopMerging();

private:
	HWND m_hDlg;

	std::wstring m_strOutputFilename;
	std::vector<std::wstring> m_filePaths;

	CRITICAL_SECTION m_csStop;
	bool m_bstopMerging;
};

class MergeFilesDialog : public BaseDialog
{
public:
	static MergeFilesDialog *Create(const ResourceLoader *resourceLoader, HWND hParent,
		const std::wstring &strOutputDirectory, const std::vector<std::wstring> &filePaths,
		BOOL bShowFriendlyDates);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	INT_PTR OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	MergeFilesDialog(const ResourceLoader *resourceLoader, HWND hParent,
		const std::wstring &strOutputDirectory, const std::vector<std::wstring> &filePaths,
		BOOL bShowFriendlyDates);
	~MergeFilesDialog();

	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnOk();
	void OnCancel();
	void OnChangeOutputDirectory();
	void OnMove(bool bUp);
	void OnFinished();

	std::wstring m_strOutputDirectory;
	std::vector<std::wstring> m_filePaths;
	BOOL m_bShowFriendlyDates;

	MergeFiles *m_pMergeFiles;
	bool m_bMergingFiles;
	bool m_bStopMerging;
	TCHAR m_szOk[32];

	MergeFilesDialogPersistentSettings *m_persistentSettings;
};
