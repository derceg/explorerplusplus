// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"
#include "../Helper/ResizableDialog.h"

class CMergeFilesDialog;

class CMergeFilesDialogPersistentSettings : public CDialogSettings
{
public:

	~CMergeFilesDialogPersistentSettings();

	static CMergeFilesDialogPersistentSettings &GetInstance();

private:

	friend CMergeFilesDialog;

	static const TCHAR SETTINGS_KEY[];

	CMergeFilesDialogPersistentSettings();

	CMergeFilesDialogPersistentSettings(const CMergeFilesDialogPersistentSettings &);
	CMergeFilesDialogPersistentSettings & operator=(const CMergeFilesDialogPersistentSettings &);
};

class CMergeFiles : public CReferenceCount
{
public:
	
	CMergeFiles(HWND hDlg,std::wstring strOutputFilename,std::list<std::wstring> FullFilenameList);
	~CMergeFiles();

	void					StartMerging();
	void					StopMerging();

private:

	HWND					m_hDlg;

	std::wstring			m_strOutputFilename;
	std::list<std::wstring>	m_FullFilenameList;

	CRITICAL_SECTION		m_csStop;
	bool					m_bstopMerging;
};

class CMergeFilesDialog : public CBaseDialog
{
public:

	CMergeFilesDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::wstring strOutputDirectory,std::list<std::wstring> FullFilenameList,BOOL bShowFriendlyDates);
	~CMergeFilesDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

	INT_PTR	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnOk();
	void	OnCancel();
	void	OnChangeOutputDirectory();
	void	OnMove(bool bUp);
	void	OnFinished();

	std::wstring m_strOutputDirectory;
	std::list<std::wstring> m_FullFilenameList;
	BOOL m_bShowFriendlyDates;

	CMergeFiles *m_pMergeFiles;
	bool m_bMergingFiles;
	bool m_bStopMerging;
	TCHAR m_szOk[32];

	CMergeFilesDialogPersistentSettings *m_pmfdps;
};