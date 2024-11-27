// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Saves/loads settings for a dialog. Window position
 * is saved automatically (if required); all other
 * settings must be explicitly specified.
 */

#include "stdafx.h"
#include "DialogSettings.h"
#include "Helper.h"
#include "RegistrySettings.h"
#include "WindowHelper.h"
#include "XMLSettings.h"
#include <wil/com.h>
#include <wil/resource.h>

const TCHAR DialogSettings::SETTING_POSITION[] = _T("Position");
const TCHAR DialogSettings::SETTING_POSITION_X[] = _T("PosX");
const TCHAR DialogSettings::SETTING_POSITION_Y[] = _T("PosY");
const TCHAR DialogSettings::SETTING_WIDTH[] = _T("Width");
const TCHAR DialogSettings::SETTING_HEIGHT[] = _T("Height");

DialogSettings::DialogSettings(const TCHAR *szSettingsKey, bool bSavePosition) :
	m_szSettingsKey(szSettingsKey),
	m_bSavePosition(bSavePosition)
{
	m_bStateSaved = FALSE;
}

void DialogSettings::SaveRegistrySettings(HKEY hParentKey)
{
	if (!m_bStateSaved)
	{
		return;
	}

	HKEY hKey;
	DWORD dwDisposition;

	LSTATUS lRes = RegCreateKeyEx(hParentKey, m_szSettingsKey.c_str(), 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &dwDisposition);

	if (lRes == ERROR_SUCCESS)
	{
		if (m_bSavePosition)
		{
			RegSetValueEx(hKey, SETTING_POSITION, 0, REG_BINARY,
				reinterpret_cast<LPBYTE>(&m_ptDialog), sizeof(m_ptDialog));

			RegistrySettings::SaveDword(hKey, SETTING_WIDTH, m_iWidth);
			RegistrySettings::SaveDword(hKey, SETTING_HEIGHT, m_iHeight);
		}

		SaveExtraRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

void DialogSettings::LoadRegistrySettings(HKEY hParentKey)
{
	HKEY hKey;
	LSTATUS lRes;

	lRes = RegOpenKeyEx(hParentKey, m_szSettingsKey.c_str(), 0, KEY_READ, &hKey);

	if (lRes == ERROR_SUCCESS)
	{
		if (m_bSavePosition)
		{
			DWORD dwSize = sizeof(POINT);
			RegQueryValueEx(hKey, SETTING_POSITION, nullptr, nullptr, (LPBYTE) &m_ptDialog,
				&dwSize);

			RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_WIDTH, m_iWidth);
			RegistrySettings::Read32BitValueFromRegistry(hKey, SETTING_HEIGHT, m_iHeight);
		}

		LoadExtraRegistrySettings(hKey);

		m_bStateSaved = TRUE;

		RegCloseKey(hKey);
	}
}

void DialogSettings::SaveXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe)
{
	if (!m_bStateSaved)
	{
		return;
	}

	auto bstr_wsntt = wil::make_bstr_nothrow(L"\n\t\t");
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe);

	wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe, _T("DialogState"),
		m_szSettingsKey.c_str());

	if (m_bSavePosition)
	{
		XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), SETTING_POSITION_X,
			XMLSettings::EncodeIntValue(m_ptDialog.x));
		XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), SETTING_POSITION_Y,
			XMLSettings::EncodeIntValue(m_ptDialog.y));
		XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), SETTING_WIDTH,
			XMLSettings::EncodeIntValue(m_iWidth));
		XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), SETTING_HEIGHT,
			XMLSettings::EncodeIntValue(m_iHeight));
	}

	SaveExtraXMLSettings(pXMLDom, pParentNode.get());
}

void DialogSettings::LoadXMLSettings(IXMLDOMNamedNodeMap *pam, long lChildNodes)
{
	for (int i = 1; i < lChildNodes; i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pNode;
		pam->get_item(i, &pNode);

		wil::unique_bstr bstrName;
		pNode->get_nodeName(&bstrName);

		wil::unique_bstr bstrValue;
		pNode->get_text(&bstrValue);

		bool bHandled = false;

		if (m_bSavePosition)
		{
			if (lstrcmpi(bstrName.get(), SETTING_POSITION_X) == 0)
			{
				m_ptDialog.x = XMLSettings::DecodeIntValue(bstrValue.get());
				bHandled = true;
			}
			else if (lstrcmpi(bstrName.get(), SETTING_POSITION_Y) == 0)
			{
				m_ptDialog.y = XMLSettings::DecodeIntValue(bstrValue.get());
				bHandled = true;
			}
			else if (lstrcmpi(bstrName.get(), SETTING_WIDTH) == 0)
			{
				m_iWidth = XMLSettings::DecodeIntValue(bstrValue.get());
				bHandled = true;
			}
			else if (lstrcmpi(bstrName.get(), SETTING_HEIGHT) == 0)
			{
				m_iHeight = XMLSettings::DecodeIntValue(bstrValue.get());
				bHandled = true;
			}
		}

		if (!bHandled)
		{
			/* Pass the node name and value to any
			descendant class to handle. */
			LoadExtraXMLSettings(bstrName.get(), bstrValue.get());
		}
	}

	m_bStateSaved = TRUE;
}

bool DialogSettings::GetSettingsKey(TCHAR *out, size_t cchMax) const
{
	if (cchMax < (m_szSettingsKey.length() + 1))
	{
		return false;
	}

	StringCchCopy(out, cchMax, m_szSettingsKey.c_str());
	return true;
}

/* No extra values are saved or loaded by default.
Derived class should override these to save any
dialog specific values they need. */
void DialogSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	UNREFERENCED_PARAMETER(hKey);
}

void DialogSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	UNREFERENCED_PARAMETER(hKey);
}

void DialogSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode)
{
	UNREFERENCED_PARAMETER(pXMLDom);
	UNREFERENCED_PARAMETER(pParentNode);
}

void DialogSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	UNREFERENCED_PARAMETER(bstrName);
	UNREFERENCED_PARAMETER(bstrValue);
}

void DialogSettings::SaveDialogPosition(HWND hDlg)
{
	RECT rc;
	GetWindowRect(hDlg, &rc);
	m_ptDialog.x = rc.left;
	m_ptDialog.y = rc.top;
	m_iWidth = GetRectWidth(&rc);
	m_iHeight = GetRectHeight(&rc);
}

void DialogSettings::RestoreDialogPosition(HWND hDlg, bool bRestoreSize)
{
	bool shouldRestore = false;

	if (m_bStateSaved)
	{
		// Some portion of the dialog window should at least be visible for the position of the
		// dialog to be restored. Note that although the dialog size isn't always restored (e.g.
		// because the dialog is a fixed size), the dialog width + height values are always saved,
		// so it's valid to use them here.
		// Also, this check will be run every time the dialog is opened (not just the first time the
		// dialog is opened after starting the application). That's likely the best option, since
		// the monitor setup could change while the application is running, or the user could have
		// moved the dialog off screen the last time it was opened. Either way, the dialog should
		// still be moved back on screen.
		RECT dialogRect = { m_ptDialog.x, m_ptDialog.y, m_ptDialog.x + m_iWidth,
			m_ptDialog.y + m_iHeight };

		if (IsRectVisible(&dialogRect))
		{
			shouldRestore = true;
		}
	}

	if (shouldRestore)
	{
		if (bRestoreSize)
		{
			SetWindowPos(hDlg, nullptr, m_ptDialog.x, m_ptDialog.y, m_iWidth, m_iHeight,
				SWP_NOZORDER);
		}
		else
		{
			SetWindowPos(hDlg, nullptr, m_ptDialog.x, m_ptDialog.y, 0, 0,
				SWP_NOSIZE | SWP_NOZORDER);
		}
	}
	else
	{
		CenterWindow(GetParent(hDlg), hDlg);
	}
}
