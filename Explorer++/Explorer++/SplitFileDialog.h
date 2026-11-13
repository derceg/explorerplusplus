// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileStreamOperations.h"
#include <string>
#include <thread>
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

class SplitFileDialog : public BaseDialog
{
public:
	static SplitFileDialog *Create(const ResourceLoader *resourceLoader, HWND hParent,
		const std::wstring &strFullFilename);

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

	static const int KB = (1024);
	static const int MB = (1024 * 1024);
	static const int GB = (1024 * 1024 * 1024);

	static const UINT_PTR ELPASED_TIMER_ID = 1;
	static const UINT_PTR ELPASED_TIMER_TIMEOUT = 1000;

	SplitFileDialog(const ResourceLoader *resourceLoader, HWND hParent,
		const std::wstring &strFullFilename);
	~SplitFileDialog();

	void OnOk();
	void OnCancel();
	void OnChangeOutputDirectory();
	void OnSplitFinished();

	std::wstring m_strFullFilename;
	bool m_bSplittingFile;
	bool m_closePending = false;

	std::unordered_map<int, SizeType> m_SizeMap;

	FileStreamOperations::Result m_result;
	std::jthread m_worker;
	HFONT m_hHelperTextFont;

	TCHAR m_szOk[32];
	UINT m_uElapsedTime;

	SplitFileDialogPersistentSettings *m_persistentSettings;
};
