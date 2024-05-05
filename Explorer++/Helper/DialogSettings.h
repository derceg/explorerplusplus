// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <MsXml2.h>
#include <objbase.h>
#include <string>

class DialogSettings : private boost::noncopyable
{
public:
	DialogSettings(const TCHAR *szSettingsKey, bool bSavePosition = true);
	virtual ~DialogSettings() = default;

	void SaveRegistrySettings(HKEY hParentKey);
	void LoadRegistrySettings(HKEY hParentKey);

	void SaveXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	void LoadXMLSettings(IXMLDOMNamedNodeMap *pam, long lChildNodes);

	bool GetSettingsKey(TCHAR *out, size_t cchMax) const;

protected:
	void SaveDialogPosition(HWND hDlg);
	void RestoreDialogPosition(HWND hDlg, bool bRestoreSize);

	BOOL m_bStateSaved;

private:
	static const TCHAR SETTING_POSITION[];
	static const TCHAR SETTING_POSITION_X[];
	static const TCHAR SETTING_POSITION_Y[];
	static const TCHAR SETTING_WIDTH[];
	static const TCHAR SETTING_HEIGHT[];

	virtual void SaveExtraRegistrySettings(HKEY hKey);
	virtual void LoadExtraRegistrySettings(HKEY hKey);

	virtual void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	virtual void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	const std::wstring m_szSettingsKey;
	const bool m_bSavePosition;

	POINT m_ptDialog;
	int m_iWidth;
	int m_iHeight;
};
