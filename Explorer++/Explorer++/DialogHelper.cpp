/******************************************************************
 *
 * Project: Explorer++
 * File: DialogHelper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Dialog helper functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "DialogHelper.h"
#include "SearchDialog.h"
#include "WildcardSelectDialog.h"
#include "SetFileAttributesDialog.h"
#include "RenameTabDialog.h"
#include "MassRenameDialog.h"
#include "FilterDialog.h"
#include "ColorRuleDialog.h"
#include "CustomizeColorsDialog.h"
#include "SplitFileDialog.h"
#include "DestroyFilesDialog.h"
#include "MergeFilesDialog.h"
#include "SelectColumnsDialog.h"
#include "SetDefaultColumnsDialog.h"
#include "AddBookmarkDialog.h"
#include "DisplayColoursDialog.h"
#include "UpdateCheckDialog.h"
#include "../Helper/XMLSettings.h"


namespace
{
	const TCHAR REG_DIALOGS_KEY[] = _T("Software\\Explorer++\\Dialogs");
}

void Explorerplusplus::LoadDialogStatesFromRegistry(void)
{
	HKEY hKey;
	LONG ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_DIALOGS_KEY,0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		CSearchDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CWildcardSelectDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSetFileAttributesDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CRenameTabDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CMassRenameDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CFilterDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CColorRuleDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CCustomizeColorsDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSplitFileDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CDestroyFilesDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CMergeFilesDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSelectColumnsDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSetDefaultColumnsDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CAddBookmarkDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CDisplayColoursDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CUpdateCheckDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveDialogStatesToRegistry(void)
{
	HKEY hKey;
	LONG ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_DIALOGS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,NULL);

	if(ReturnValue == ERROR_SUCCESS)
	{
		CSearchDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CWildcardSelectDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSetFileAttributesDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CRenameTabDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CMassRenameDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CFilterDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CColorRuleDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CCustomizeColorsDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSplitFileDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CDestroyFilesDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CMergeFilesDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSelectColumnsDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSetDefaultColumnsDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CAddBookmarkDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CDisplayColoursDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CUpdateCheckDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadDialogStatesFromXML(MSXML2::IXMLDOMDocument *pXMLDom)
{
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
	MSXML2::IXMLDOMNode			*pChildNode = NULL;
	BSTR						bstrName;
	BSTR						bstrValue;
	BSTR						bstr = NULL;
	HRESULT						hr;
	long						length;
	long						lChildNodes;

	if(pXMLDom == NULL)
		goto clean;

	bstr = SysAllocString(L"//State/*");
	pXMLDom->selectNodes(bstr,&pNodes);

	if(!pNodes)
	{
		goto clean;
	}
	else
	{
		pNodes->get_length(&length);

		for(long i = 0;i < length;i++)
		{
			/* This should never fail, as the number
			of nodes has already been counted (so
			they must exist). */
			hr = pNodes->get_item(i,&pNode);

			if(SUCCEEDED(hr))
			{
				hr = pNode->get_attributes(&am);

				if(SUCCEEDED(hr))
				{
					/* Retrieve the total number of attributes
					attached to this node. */
					am->get_length(&lChildNodes);

					if(lChildNodes >= 1)
					{
						am->get_item(0,&pChildNode);

						/* Element name. */
						pChildNode->get_nodeName(&bstrName);

						/* Element value. */
						pChildNode->get_text(&bstrValue);

						if(lstrcmpi(bstrValue,_T("ColorRules")) == 0)
							CColorRuleDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("CustomizeColors")) == 0)
							CCustomizeColorsDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("Search")) == 0)
							CSearchDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("WildcardSelect")) == 0)
							CWildcardSelectDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("RenameTab")) == 0)
							CRenameTabDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("MassRename")) == 0)
							CMassRenameDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("Filter")) == 0)
							CFilterDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("SplitFile")) == 0)
							CSplitFileDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("DestroyFiles")) == 0)
							CDestroyFilesDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("MergeFiles")) == 0)
							CMergeFilesDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("SetFileAttributes")) == 0)
							CSetFileAttributesDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("SelectColumns")) == 0)
							CSelectColumnsDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("SetDefaultColumns")) == 0)
							CSetDefaultColumnsDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("AddBookmark")) == 0)
							CAddBookmarkDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("DisplayColors")) == 0)
							CDisplayColoursDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
						else if(lstrcmpi(bstrValue,_T("UpdateCheck")) == 0)
							CUpdateCheckDialogPersistentSettings::GetInstance().LoadXMLSettings(am,lChildNodes);
					}
				}
			}

			pNode->Release();
			pNode = NULL;
		}
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();
}

void Explorerplusplus::SaveDialogStatesToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement	*pe = NULL;
	BSTR					bstr = NULL;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"State");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = NULL;

	CSearchDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CWildcardSelectDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CSetFileAttributesDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CRenameTabDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CMassRenameDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CFilterDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CColorRuleDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CCustomizeColorsDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CSplitFileDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CDestroyFilesDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CMergeFilesDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CSelectColumnsDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CSetDefaultColumnsDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CAddBookmarkDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CDisplayColoursDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);
	CUpdateCheckDialogPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = NULL;
}