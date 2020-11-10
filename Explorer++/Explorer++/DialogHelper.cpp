// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Bookmarks/UI/AddBookmarkDialog.h"
#include "Bookmarks/UI/ManageBookmarksDialog.h"
#include "ColorRuleDialog.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "FilterDialog.h"
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
#include <wil/com.h>
#include <wil/resource.h>

namespace
{
	const TCHAR DIALOGS_REGISTRY_KEY[] = _T("Software\\Explorer++\\Dialogs");
	const TCHAR DIALOGS_XML_KEY[] = _T("State");

	/* Safe provided that the object returned through
	GetInstance is stable throughout the lifetime of
	the program (which is true, as these are all
	singletons). */
	DialogSettings *const DIALOG_SETTINGS[] = { &SearchDialogPersistentSettings::GetInstance(),
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
		&UpdateCheckDialogPersistentSettings::GetInstance() };
}

void Explorerplusplus::LoadDialogStatesFromRegistry()
{
	HKEY hKey;
	LONG returnValue = RegOpenKeyEx(HKEY_CURRENT_USER, DIALOGS_REGISTRY_KEY, 0, KEY_READ, &hKey);

	if (returnValue == ERROR_SUCCESS)
	{
		for (DialogSettings *ds : DIALOG_SETTINGS)
		{
			ds->LoadRegistrySettings(hKey);
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveDialogStatesToRegistry()
{
	HKEY hKey;
	LONG returnValue = RegCreateKeyEx(HKEY_CURRENT_USER, DIALOGS_REGISTRY_KEY, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);

	if (returnValue == ERROR_SUCCESS)
	{
		for (DialogSettings *ds : DIALOG_SETTINGS)
		{
			ds->SaveRegistrySettings(hKey);
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadDialogStatesFromXML(IXMLDOMDocument *pXMLDom)
{
	if (!pXMLDom)
	{
		return;
	}

	TCHAR tempNodeSelector[64];
	StringCchPrintf(
		tempNodeSelector, SIZEOF_ARRAY(tempNodeSelector), _T("//%s/*"), DIALOGS_XML_KEY);
	auto bstr = wil::make_bstr_nothrow(tempNodeSelector);

	wil::com_ptr_nothrow<IXMLDOMNodeList> pNodes;
	pXMLDom->selectNodes(bstr.get(), &pNodes);

	if (!pNodes)
	{
		return;
	}

	long length;
	pNodes->get_length(&length);

	for (long i = 0; i < length; i++)
	{
		/* This should never fail, as the number
		of nodes has already been counted (so
		they must exist). */
		wil::com_ptr_nothrow<IXMLDOMNode> pNode;
		HRESULT hr = pNodes->get_item(i, &pNode);

		if (SUCCEEDED(hr))
		{
			wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
			hr = pNode->get_attributes(&am);

			if (SUCCEEDED(hr))
			{
				/* Retrieve the total number of attributes
				attached to this node. */
				long lChildNodes;
				am->get_length(&lChildNodes);

				if (lChildNodes >= 1)
				{
					wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
					am->get_item(0, &pChildNode);

					wil::unique_bstr bstrValue;
					pChildNode->get_text(&bstrValue);

					for (DialogSettings *ds : DIALOG_SETTINGS)
					{
						TCHAR settingsKey[64];
						bool success = ds->GetSettingsKey(settingsKey, SIZEOF_ARRAY(settingsKey));
						assert(success);

						if (!success)
						{
							continue;
						}

						if (lstrcmpi(bstrValue.get(), settingsKey) == 0)
						{
							ds->LoadXMLSettings(am.get(), lChildNodes);
						}
					}
				}
			}
		}
	}
}

void Explorerplusplus::SaveDialogStatesToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot)
{
	auto bstr_wsnt = wil::make_bstr_nothrow(L"\n\t");
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pRoot);

	wil::com_ptr_nothrow<IXMLDOMElement> pe;
	auto bstr = wil::make_bstr_nothrow(DIALOGS_XML_KEY);
	pXMLDom->createElement(bstr.get(), &pe);

	for (DialogSettings *ds : DIALOG_SETTINGS)
	{
		ds->SaveXMLSettings(pXMLDom, pe.get());
	}

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pe.get());
	NXMLSettings::AppendChildToParent(pe.get(), pRoot);
}