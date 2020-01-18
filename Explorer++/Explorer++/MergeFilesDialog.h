// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"
#include "../Helper/ResizableDialog.h"

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
	MergeFilesDialogPersistentSettings & operator=(const MergeFilesDialogPersistentSettings &);
};

class MergeFiles : public ReferenceCount
{
public:
	
	MergeFiles(HWND hDlg,std::wstring strOutputFilename,std::list<std::wstring> FullFilenameList);
	~MergeFiles();

	void					StartMerging();
	void					StopMerging();

private:

	HWND					m_hDlg;

	std::wstring			m_strOutputFilename;
	std::list<std::wstring>	m_FullFilenameList;

	CRITICAL_SECTION		m_csStop;
	bool					m_bstopMerging;
};

class MergeFilesDialog : public BaseDialog
{
public:

	MergeFilesDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
		std::wstring strOutputDirectory, std::list<std::wstring> FullFilenameList,
		BOOL bShowFriendlyDates);
	~MergeFilesDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

	INT_PTR	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	void	GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnOk();
	void	OnCancel();
	void	OnChangeOutputDirectory();
	void	OnMove(bool bUp);
	void	OnFinished();

	IExplorerplusplus *m_expp;

	std::wstring m_strOutputDirectory;
	std::list<std::wstring> m_FullFilenameList;
	BOOL m_bShowFriendlyDates;

	MergeFiles *m_pMergeFiles;
	bool m_bMergingFiles;
	bool m_bStopMerging;
	TCHAR m_szOk[32];

	MergeFilesDialogPersistentSettings *m_pmfdps;
};