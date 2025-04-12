// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainRebarXmlStorage.h"
#include "MainRebarStorage.h"
#include "MainRebarStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class MainRebarXmlStorageTest : public XmlStorageTest
{
protected:
	static inline const wchar_t MAIN_REBAR_NODE_NAME[] = L"Toolbars";
};

TEST_F(MainRebarXmlStorageTest, Load)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	std::wstring xmlFilePath = GetResourcePath(L"main-rebar.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	wil::com_ptr_nothrow<IXMLDOMNode> mainRebarNode;
	auto queryString = wil::make_bstr_nothrow(MAIN_REBAR_NODE_NAME);
	HRESULT hr = xmlDocumentData.rootNode->selectSingleNode(queryString.get(), &mainRebarNode);
	ASSERT_EQ(hr, S_OK);

	auto loadedRebarStorageInfo = MainRebarXmlStorage::Load(mainRebarNode.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}

TEST_F(MainRebarXmlStorageTest, Save)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	auto xmlDocumentData = CreateXmlDocument();

	wil::com_ptr_nothrow<IXMLDOMElement> mainRebarNode;
	auto nodeName = wil::make_bstr_nothrow(MAIN_REBAR_NODE_NAME);
	HRESULT hr = xmlDocumentData.xmlDocument->createElement(nodeName.get(), &mainRebarNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	MainRebarXmlStorage::Save(xmlDocumentData.xmlDocument.get(), mainRebarNode.get(),
		referenceRebarStorageInfo);
	auto loadedRebarStorageInfo = MainRebarXmlStorage::Load(mainRebarNode.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}
