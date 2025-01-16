// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StartupFoldersXmlStorage.h"
#include "../Helper/XMLSettings.h"

namespace
{

constexpr wchar_t STARTUP_FOLDER_NODE_NAME[] = L"StartupFolder";

constexpr wchar_t SETTING_STARTUP_FOLDER_PATH[] = L"Path";

std::optional<std::wstring> LoadStartupFolder(IXMLDOMNode *startupFolderNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = startupFolderNode->get_attributes(&attributeMap);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	std::wstring path;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_STARTUP_FOLDER_PATH, path);

	if (hr != S_OK || path.empty())
	{
		return std::nullopt;
	}

	return path;
}

void SaveStartupFolder(IXMLDOMDocument *xmlDocument, IXMLDOMElement *startupFolderNode,
	const std::wstring &startupFolder)
{
	XMLSettings::AddAttributeToNode(xmlDocument, startupFolderNode, SETTING_STARTUP_FOLDER_PATH,
		startupFolder);
}

}

namespace StartupFoldersXmlStorage
{

std::vector<std::wstring> Load(IXMLDOMNode *startupFoldersNode)
{
	return XMLSettings::ReadItemList<std::wstring>(startupFoldersNode, STARTUP_FOLDER_NODE_NAME,
		LoadStartupFolder);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *startupFoldersNode,
	const std::vector<std::wstring> &startupFolders)
{
	XMLSettings::SaveItemList<std::wstring>(xmlDocument, startupFoldersNode, startupFolders,
		STARTUP_FOLDER_NODE_NAME, SaveStartupFolder);
}

}
