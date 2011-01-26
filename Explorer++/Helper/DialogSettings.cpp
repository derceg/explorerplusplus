/******************************************************************
 *
 * Project: Helper
 * File: DialogSettings.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Saves/loads settings for a dialog. Window position
 * is saved automatically (if required); all other
 * settings must be explicitly specified.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "DialogSettings.h"
#include "RegistrySettings.h"
#include "XMLSettings.h"


CDialogSettings::CDialogSettings(const TCHAR *szSettingsKey,bool bSavePosition)
{
	StringCchCopy(m_szSettingsKey,SIZEOF_ARRAY(m_szSettingsKey),
		szSettingsKey);
	m_bSavePosition = bSavePosition;

	m_bStateSaved = FALSE;
}

CDialogSettings::~CDialogSettings()
{

}

void CDialogSettings::SaveRegistrySettings(HKEY hParentKey)
{
	if(!m_bStateSaved)
	{
		return;
	}

	HKEY hKey;
	DWORD dwDisposition;

	LONG lRes = RegCreateKeyEx(hParentKey,m_szSettingsKey,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&dwDisposition);

	if(lRes == ERROR_SUCCESS)
	{
		if(m_bSavePosition)
		{
			RegSetValueEx(hKey,_T("Position"),0,REG_BINARY,
				reinterpret_cast<LPBYTE>(&m_ptDialog),
				sizeof(m_ptDialog));
		}

		SaveExtraRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

void CDialogSettings::LoadRegistrySettings(HKEY hParentKey)
{
	HKEY hKey;
	LONG lRes;

	lRes = RegOpenKeyEx(hParentKey,m_szSettingsKey,0,
		KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		if(m_bSavePosition)
		{
			DWORD dwSize = sizeof(POINT);
			RegQueryValueEx(hKey,_T("Position"),
				NULL,NULL,(LPBYTE)&m_ptDialog,&dwSize);
		}

		LoadExtraRegistrySettings(hKey);

		m_bStateSaved = TRUE;

		RegCloseKey(hKey);
	}
}

void CDialogSettings::SaveXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,
	MSXML2::IXMLDOMElement *pe)
{
	if(!m_bStateSaved)
	{
		return;
	}

	MSXML2::IXMLDOMElement *pParentNode = NULL;
	BSTR bstr_wsntt = SysAllocString(L"\n\t\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("DialogState"),m_szSettingsKey);

	if(m_bSavePosition)
	{
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("PosX"),
			NXMLSettings::EncodeIntValue(m_ptDialog.x));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("PosY"),
			NXMLSettings::EncodeIntValue(m_ptDialog.y));
	}

	SaveExtraXMLSettings(pXMLDom,pParentNode);
}

void CDialogSettings::LoadXMLSettings(MSXML2::IXMLDOMNamedNodeMap *pam,long lChildNodes)
{
	MSXML2::IXMLDOMNode *pNode = NULL;
	BSTR bstrName;
	BSTR bstrValue;

	for(int i = 1;i < lChildNodes;i++)
	{
		pam->get_item(i,&pNode);

		pNode->get_nodeName(&bstrName);
		pNode->get_text(&bstrValue);

		bool bHandled = false;

		if(m_bSavePosition)
		{
			if(lstrcmpi(bstrName,_T("PosX")) == 0)
			{
				m_ptDialog.x = NXMLSettings::DecodeIntValue(bstrValue);
				bHandled = true;
			}
			else if(lstrcmpi(bstrName,_T("PosY")) == 0)
			{
				m_ptDialog.y = NXMLSettings::DecodeIntValue(bstrValue);
				bHandled = true;
			}
		}

		if(!bHandled)
		{
			/* Pass the node name and value to any
			descendant class to handle. */
			LoadExtraXMLSettings(bstrName,bstrValue);
		}
	}

	m_bStateSaved = TRUE;
}

/* No extra values are saved or loaded by default.
Derived class should override these to save any
dialog specific values they need. */
void CDialogSettings::SaveExtraRegistrySettings(HKEY hKey)
{

}

void CDialogSettings::LoadExtraRegistrySettings(HKEY hKey)
{

}

void CDialogSettings::SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode)
{

}

void CDialogSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{

}