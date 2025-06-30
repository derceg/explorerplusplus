// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DialogStorageHelper.h"
#include "Bookmarks/UI/AddBookmarkDialog.h"
#include "Bookmarks/UI/ManageBookmarksDialog.h"
#include "ColorRuleEditorDialog.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "FilterDialog.h"
#include "MassRenameDialog.h"
#include "MergeFilesDialog.h"
#include "RenameTabDialog.h"
#include "SearchDialog.h"
#include "SearchTabsDialog.h"
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

const TCHAR DIALOGS_REGISTRY_KEY_PATH[] = _T("Dialogs");
const TCHAR DIALOGS_XML_KEY[] = _T("State");

/* Safe provided that the object returned through
GetInstance is stable throughout the lifetime of
the program (which is true, as these are all
singletons). */
// clang-format off
DialogSettings *const DIALOG_SETTINGS[] = {
	&SearchDialogPersistentSettings::GetInstance(),
	&WildcardSelectDialogPersistentSettings::GetInstance(),
	&SetFileAttributesDialogPersistentSettings::GetInstance(),
	&RenameTabDialogPersistentSettings::GetInstance(),
	&MassRenameDialogPersistentSettings::GetInstance(),
	&FilterDialogPersistentSettings::GetInstance(),
	&ColorRuleEditorDialogPersistentSettings::GetInstance(),
	&CustomizeColorsDialogPersistentSettings::GetInstance(),
	&SplitFileDialogPersistentSettings::GetInstance(),
	&DestroyFilesDialogPersistentSettings::GetInstance(),
	&MergeFilesDialogPersistentSettings::GetInstance(),
	&SelectColumnsDialogPersistentSettings::GetInstance(),
	&SetDefaultColumnsDialogPersistentSettings::GetInstance(),
	&AddBookmarkDialogPersistentSettings::GetInstance(),
	&ManageBookmarksDialogPersistentSettings::GetInstance(),
	&DisplayColoursDialogPersistentSettings::GetInstance(),
	&UpdateCheckDialogPersistentSettings::GetInstance(),
	&SearchTabsDialogPersistentSettings::GetInstance()
};
// clang-format on

}

namespace DialogStorageHelper
{

void LoadDialogStatesFromRegistry(HKEY applicationKey)
{
	wil::unique_hkey key;
	LSTATUS res = RegOpenKeyEx(applicationKey, DIALOGS_REGISTRY_KEY_PATH, 0, KEY_READ, &key);

	if (res == ERROR_SUCCESS)
	{
		for (DialogSettings *ds : DIALOG_SETTINGS)
		{
			ds->LoadRegistrySettings(key.get());
		}
	}
}

void SaveDialogStatesToRegistry(HKEY applicationKey)
{
	wil::unique_hkey key;
	LSTATUS res = RegCreateKeyEx(applicationKey, DIALOGS_REGISTRY_KEY_PATH, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr);

	if (res == ERROR_SUCCESS)
	{
		for (DialogSettings *ds : DIALOG_SETTINGS)
		{
			ds->SaveRegistrySettings(key.get());
		}
	}
}

void LoadDialogStatesFromXML(IXMLDOMDocument *xmlDocument)
{
	if (!xmlDocument)
	{
		return;
	}

	TCHAR tempNodeSelector[64];
	StringCchPrintf(tempNodeSelector, std::size(tempNodeSelector), _T("//%s/*"), DIALOGS_XML_KEY);
	auto bstr = wil::make_bstr_nothrow(tempNodeSelector);

	wil::com_ptr_nothrow<IXMLDOMNodeList> pNodes;
	xmlDocument->selectNodes(bstr.get(), &pNodes);

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
						bool success = ds->GetSettingsKey(settingsKey, std::size(settingsKey));
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

void SaveDialogStatesToXML(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode)
{
	wil::com_ptr_nothrow<IXMLDOMElement> pe;
	auto bstr = wil::make_bstr_nothrow(DIALOGS_XML_KEY);
	xmlDocument->createElement(bstr.get(), &pe);

	for (DialogSettings *ds : DIALOG_SETTINGS)
	{
		ds->SaveXMLSettings(xmlDocument, pe.get());
	}

	XMLSettings::AppendChildToParent(pe.get(), rootNode);
}

}
