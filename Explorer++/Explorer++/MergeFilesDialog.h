// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"
#include "../Helper/ResizableDialogHelper.h"

class IconResourceLoader;
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
		const std::list<std::wstring> &FullFilenameList);
	~MergeFiles();

	void StartMerging();
	void StopMerging();

private:
	HWND m_hDlg;

	std::wstring m_strOutputFilename;
	std::list<std::wstring> m_FullFilenameList;

	CRITICAL_SECTION m_csStop;
	bool m_bstopMerging;
};

class MergeFilesDialog : public ThemedDialog
{
public:
	MergeFilesDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, HWND hParent,
		ThemeManager *themeManager, const IconResourceLoader *iconResourceLoader,
		const std::wstring &strOutputDirectory, const std::list<std::wstring> &FullFilenameList,
		BOOL bShowFriendlyDates);
	~MergeFilesDialog();

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	INT_PTR OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnOk();
	void OnCancel();
	void OnChangeOutputDirectory();
	void OnMove(bool bUp);
	void OnFinished();

	const IconResourceLoader *const m_iconResourceLoader;

	std::wstring m_strOutputDirectory;
	std::list<std::wstring> m_FullFilenameList;
	BOOL m_bShowFriendlyDates;

	MergeFiles *m_pMergeFiles;
	bool m_bMergingFiles;
	bool m_bStopMerging;
	TCHAR m_szOk[32];

	MergeFilesDialogPersistentSettings *m_persistentSettings;
};
