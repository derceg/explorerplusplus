// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileStreamOperations.h"
#include "../Helper/ResizableDialogHelper.h"
#include <string>
#include <thread>
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

	FileStreamOperations::Result m_result;
	std::jthread m_worker;
	bool m_bMergingFiles;
	bool m_closePending = false;
	TCHAR m_szOk[32];

	MergeFilesDialogPersistentSettings *m_persistentSettings;
};
