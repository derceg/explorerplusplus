// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DefaultColumnXmlStorage.h"
#include "ColumnXmlStorage.h"
#include "../Helper/XMLSettings.h"

namespace
{

const wchar_t DEFAULT_COLUMNS_NODE_NAME[] = L"DefaultColumns";

}

namespace DefaultColumnXmlStorage
{

void Load(IXMLDOMDocument *xmlDocument, FolderColumns &defaultColumns)
{
	using namespace std::string_literals;

	wil::com_ptr_nothrow<IXMLDOMNode> defaultColumnsNode;
	auto queryString =
		wil::make_bstr_nothrow((L"/ExplorerPlusPlus/"s + DEFAULT_COLUMNS_NODE_NAME).c_str());
	xmlDocument->selectSingleNode(queryString.get(), &defaultColumnsNode);

	if (!defaultColumnsNode)
	{
		return;
	}

	ColumnXmlStorage::LoadAllColumnSets(defaultColumnsNode.get(), defaultColumns);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const FolderColumns &defaultColumns)
{
	wil::com_ptr_nothrow<IXMLDOMElement> defaultColumnsNode;
	auto nodeName = wil::make_bstr_nothrow(DEFAULT_COLUMNS_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(nodeName.get(), &defaultColumnsNode);

	if (FAILED(hr))
	{
		return;
	}

	ColumnXmlStorage::SaveAllColumnSets(xmlDocument, defaultColumnsNode.get(), defaultColumns);

	XMLSettings::AppendChildToParent(defaultColumnsNode.get(), rootNode);
}

}
