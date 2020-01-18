// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"
#include <list>
#include <string>
#include <unordered_map>

class SplitFileDialog;

class SplitFileDialogPersistentSettings : public DialogSettings
{
public:

	static SplitFileDialogPersistentSettings &GetInstance();

private:

	friend SplitFileDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_SIZE[];
	static const TCHAR SETTING_SIZE_GROUP[];

	SplitFileDialogPersistentSettings();

	SplitFileDialogPersistentSettings(const SplitFileDialogPersistentSettings &);
	SplitFileDialogPersistentSettings & operator=(const SplitFileDialogPersistentSettings &);

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	std::wstring	m_strSplitSize;
	std::wstring	m_strSplitGroup;
};

class SplitFile : public ReferenceCount
{
public:
	
	SplitFile(HWND hDlg,std::wstring strFullFilename,std::wstring strOutputFilename,std::wstring strOutputDirectory,UINT uSplitSize);
	~SplitFile();

	void	Split();
	void	StopSplitting();

private:

	void				SplitInternal(HANDLE hInputFile,const LARGE_INTEGER &lFileSize);
	void				ProcessFilename(int nSplitsMade,std::wstring &strOutputFullFilename);

	HWND				m_hDlg;

	std::wstring		m_strFullFilename;
	std::wstring		m_strOutputFilename;
	std::wstring		m_strOutputDirectory;
	UINT				m_uSplitSize;

	CRITICAL_SECTION	m_csStop;
	bool				m_bStopSplitting;
};

class SplitFileDialog : public BaseDialog
{
public:

	SplitFileDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
		std::wstring strFullFilename);
	~SplitFileDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnTimer(int iTimerID);
	INT_PTR	OnCtlColorStatic(HWND hwnd,HDC hdc);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();
	INT_PTR	OnDestroy();

	void	SaveState();

	INT_PTR	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	enum SizeType_t
	{
		SIZE_TYPE_BYTES,
		SIZE_TYPE_KB,
		SIZE_TYPE_MB,
		SIZE_TYPE_GB,
	};

	enum ErrorType_t
	{
		ERROR_NONE,
		ERROR_OUTPUT_FILENAME_EMPTY,
		ERROR_OUTPUT_FILENAME_CONSTANT,
		ERROR_OUTPUT_DIRECTORY_EMPTY,
		ERROR_SPLIT_SIZE
	};

	static const COLORREF HELPER_TEXT_COLOR = RGB(120,120,120);

	static const int KB = (1024);
	static const int MB = (1024 * 1024);
	static const int GB = (1024 * 1024 * 1024);

	static const UINT_PTR ELPASED_TIMER_ID = 1;
	static const UINT_PTR ELPASED_TIMER_TIMEOUT = 1000;

	void	OnOk();
	void	OnCancel();
	void	OnChangeOutputDirectory();
	void	OnSplitFinished();

	IExplorerplusplus *m_expp;

	std::wstring m_strFullFilename;
	bool m_bSplittingFile;
	bool m_bStopSplitting;

	std::unordered_map<int,SizeType_t> m_SizeMap;

	SplitFile *m_pSplitFile;
	HFONT m_hHelperTextFont;

	TCHAR m_szOk[32];
	UINT m_uElapsedTime;

	ErrorType_t m_CurrentError;

	SplitFileDialogPersistentSettings *m_psfdps;
};