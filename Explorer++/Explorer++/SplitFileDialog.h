// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"
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
	SplitFileDialogPersistentSettings &operator=(const SplitFileDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey) override;
	void LoadExtraRegistrySettings(HKEY hKey) override;

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	std::wstring m_strSplitSize;
	std::wstring m_strSplitGroup;
};

class SplitFile : public ReferenceCount
{
public:
	SplitFile(HWND hDlg, const std::wstring &strFullFilename, const std::wstring &strOutputFilename,
		const std::wstring &strOutputDirectory, UINT uSplitSize);
	~SplitFile();

	void Split();
	void StopSplitting();

private:
	void SplitInternal(HANDLE hInputFile, const LARGE_INTEGER &lFileSize);
	void ProcessFilename(int nSplitsMade, std::wstring &strOutputFullFilename);

	HWND m_hDlg;

	std::wstring m_strFullFilename;
	std::wstring m_strOutputFilename;
	std::wstring m_strOutputDirectory;
	UINT m_uSplitSize;

	CRITICAL_SECTION m_csStop;
	bool m_bStopSplitting;
};

class SplitFileDialog : public BaseDialog
{
public:
	SplitFileDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, HWND hParent,
		const std::wstring &strFullFilename);
	~SplitFileDialog();

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnTimer(int iTimerID) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;
	INT_PTR OnDestroy() override;

	void SaveState() override;

	INT_PTR OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	enum class SizeType
	{
		Bytes,
		KB,
		MB,
		GB,
	};

	enum class ErrorType
	{
		None,
		OutputFilenameEmpty,
		OutputFilenameConstant,
		OutputDirectoryEmpty,
		SplitSize
	};

	static const int KB = (1024);
	static const int MB = (1024 * 1024);
	static const int GB = (1024 * 1024 * 1024);

	static const UINT_PTR ELPASED_TIMER_ID = 1;
	static const UINT_PTR ELPASED_TIMER_TIMEOUT = 1000;

	void OnOk();
	void OnCancel();
	void OnChangeOutputDirectory();
	void OnSplitFinished();

	std::wstring m_strFullFilename;
	bool m_bSplittingFile;
	bool m_bStopSplitting;

	std::unordered_map<int, SizeType> m_SizeMap;

	SplitFile *m_pSplitFile;
	HFONT m_hHelperTextFont;

	TCHAR m_szOk[32];
	UINT m_uElapsedTime;

	ErrorType m_CurrentError;

	SplitFileDialogPersistentSettings *m_persistentSettings;
};
