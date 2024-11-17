// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabXmlStorage.h"
#include "ResourceTestHelper.h"
#include "TabStorage.h"
#include "TabStorageTestHelper.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class TabXmlStorageTest : public XmlStorageTest
{
protected:
	static inline const WCHAR TABS_NODE_NAME[] = L"Tabs";
};

TEST_F(TabXmlStorageTest, Load)
{
	std::vector<TabStorageData> referenceTabs;
	BuildTabStorageLoadSaveReference(referenceTabs, TestStorageType::Xml);

	std::wstring xmlFilePath = GetResourcePath(L"tabs-config.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	wil::com_ptr_nothrow<IXMLDOMNode> tabsNode;
	auto queryString = wil::make_bstr_nothrow(TABS_NODE_NAME);
	HRESULT hr = xmlDocumentData.rootNode->selectSingleNode(queryString.get(), &tabsNode);
	ASSERT_EQ(hr, S_OK);

	auto loadedTabs = TabXmlStorage::Load(tabsNode.get());

	EXPECT_EQ(loadedTabs, referenceTabs);
}

TEST_F(TabXmlStorageTest, Save)
{
	std::vector<TabStorageData> referenceTabs;
	BuildTabStorageLoadSaveReference(referenceTabs, TestStorageType::Xml);

	auto xmlDocumentData = CreateXmlDocument();

	wil::com_ptr_nothrow<IXMLDOMElement> tabsNode;
	auto nodeName = wil::make_bstr_nothrow(TABS_NODE_NAME);
	HRESULT hr = xmlDocumentData.xmlDocument->createElement(nodeName.get(), &tabsNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	TabXmlStorage::Save(xmlDocumentData.xmlDocument.get(), tabsNode.get(), referenceTabs);
	auto loadedTabs = TabXmlStorage::Load(tabsNode.get());

	EXPECT_EQ(loadedTabs, referenceTabs);
}
