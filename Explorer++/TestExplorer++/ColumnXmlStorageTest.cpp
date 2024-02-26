// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColumnXmlStorage.h"
#include "ColumnStorageHelper.h"
#include "ResourceHelper.h"
#include "ShellBrowser/FolderSettings.h"
#include "XmlStorageHelper.h"
#include <gtest/gtest.h>
#include <wil/resource.h>

using namespace testing;

class ColumnXmlStorageTest : public XmlStorageTest
{
protected:
	static inline const WCHAR COLUMNS_NODE_NAME[] = L"Columns";
};

TEST_F(ColumnXmlStorageTest, Load)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	std::wstring xmlFilePath = GetResourcePath(L"columns-config.xml");
	auto xmlDocument = LoadXmlDocument(xmlFilePath);
	ASSERT_TRUE(xmlDocument);

	wil::com_ptr_nothrow<IXMLDOMNode> columnsNode;
	auto queryString =
		wil::make_bstr_nothrow((std::wstring(L"/ExplorerPlusPlus/") + COLUMNS_NODE_NAME).c_str());
	HRESULT hr = xmlDocument->selectSingleNode(queryString.get(), &columnsNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	FolderColumns loadedColumns;
	ColumnXmlStorage::LoadAllColumnSets(columnsNode.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}

TEST_F(ColumnXmlStorageTest, Save)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	auto xmlDocumentData = CreateXmlDocument();
	ASSERT_TRUE(xmlDocumentData);

	wil::com_ptr_nothrow<IXMLDOMElement> columnsNode;
	auto nodeName = wil::make_bstr_nothrow(COLUMNS_NODE_NAME);
	HRESULT hr = xmlDocumentData->xmlDocument->createElement(nodeName.get(), &columnsNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	ColumnXmlStorage::SaveAllColumnSets(xmlDocumentData->xmlDocument.get(), columnsNode.get(),
		referenceColumns);

	FolderColumns loadedColumns;
	ColumnXmlStorage::LoadAllColumnSets(columnsNode.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}
