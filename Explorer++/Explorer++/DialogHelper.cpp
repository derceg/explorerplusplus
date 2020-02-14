// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DialogHelper.h"
#include "AddBookmarkDialog.h"
#include "Explorer++.h"
#include "ColorRuleDialog.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "FilterDialog.h"
#include "ManageBookmarksDialog.h"
#include "MassRenameDialog.h"
#include "MergeFilesDialog.h"
#include "RenameTabDialog.h"
#include "SearchDialog.h"
#include "SelectColumnsDialog.h"
#include "SetDefaultColumnsDialog.h"
#include "SetFileAttributesDialog.h"
#include "SplitFileDialog.h"
#include "UpdateCheckDialog.h"
#include "WildcardSelectDialog.h"
#include "../Helper/XMLSettings.h"


namespace
{
	const TCHAR DIALOGS_REGISTRY_KEY[] = _T("Software\\Explorer++\\Dialogs");
	const TCHAR DIALOGS_XML_KEY[] = _T("State");

	/* Safe provided that the object returned through
	GetInstance is stable throughout the lifetime of
	the program (which is true, as these are all
	singletons). */
	DialogSettings* const DIALOG_SETTINGS[] = {
		&SearchDialogPersistentSettings::GetInstance(),
		&WildcardSelectDialogPersistentSettings::GetInstance(),
		&SetFileAttributesDialogPersistentSettings::GetInstance(),
		&RenameTabDialogPersistentSettings::GetInstance(),
		&MassRenameDialogPersistentSettings::GetInstance(),
		&FilterDialogPersistentSettings::GetInstance(),
		&ColorRuleDialogPersistentSettings::GetInstance(),
		&CustomizeColorsDialogPersistentSettings::GetInstance(),
		&SplitFileDialogPersistentSettings::GetInstance(),
		&DestroyFilesDialogPersistentSettings::GetInstance(),
		&MergeFilesDialogPersistentSettings::GetInstance(),
		&SelectColumnsDialogPersistentSettings::GetInstance(),
		&SetDefaultColumnsDialogPersistentSettings::GetInstance(),
		&AddBookmarkDialogPersistentSettings::GetInstance(),
		&ManageBookmarksDialogPersistentSettings::GetInstance(),
		&DisplayColoursDialogPersistentSettings::GetInstance(),
		&UpdateCheckDialogPersistentSettings::GetInstance()
	};
}

void Explorerplusplus::LoadDialogStatesFromRegistry(void)
{
	HKEY hKey;
	LONG ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,DIALOGS_REGISTRY_KEY,0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		for(DialogSettings *ds : DIALOG_SETTINGS)
		{
			ds->LoadRegistrySettings(hKey);
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveDialogStatesToRegistry(void)
{
	HKEY hKey;
	LONG ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,DIALOGS_REGISTRY_KEY,
		0, nullptr,REG_OPTION_NON_VOLATILE,KEY_WRITE, nullptr,&hKey, nullptr);

	if(ReturnValue == ERROR_SUCCESS)
	{
		for(DialogSettings *ds : DIALOG_SETTINGS)
		{
			ds->SaveRegistrySettings(hKey);
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadDialogStatesFromXML(IXMLDOMDocument *pXMLDom)
{
	IXMLDOMNodeList		*pNodes = nullptr;
	IXMLDOMNode			*pNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	IXMLDOMNode			*pChildNode = nullptr;
	BSTR						bstrName;
	BSTR						bstrValue;
	BSTR						bstr = nullptr;
	HRESULT						hr;
	long						length;
	long						lChildNodes;

	if(pXMLDom == nullptr)
		goto clean;

	TCHAR tempNodeSelector[64];
	StringCchPrintf(tempNodeSelector, SIZEOF_ARRAY(tempNodeSelector),
		_T("//%s/*"), DIALOGS_XML_KEY);
	bstr = SysAllocString(tempNodeSelector);
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

						pChildNode->get_nodeName(&bstrName);
						pChildNode->get_text(&bstrValue);

						for(DialogSettings *ds : DIALOG_SETTINGS)
						{
							TCHAR settingsKey[64];
							bool success = ds->GetSettingsKey(settingsKey, SIZEOF_ARRAY(settingsKey));
							assert(success);

							if(!success)
							{
								continue;
							}

							if(lstrcmpi(bstrValue, settingsKey) == 0)
							{
								ds->LoadXMLSettings(am, lChildNodes);
							}
						}
					}
				}
			}

			pNode->Release();
			pNode = nullptr;
		}
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();
}

void Explorerplusplus::SaveDialogStatesToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	IXMLDOMElement	*pe = nullptr;
	BSTR					bstr = nullptr;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(DIALOGS_XML_KEY);
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = nullptr;

	for(DialogSettings *ds : DIALOG_SETTINGS)
	{
		ds->SaveXMLSettings(pXMLDom, pe);
	}

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = nullptr;
}