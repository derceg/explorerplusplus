// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RebarXmlStorage.h"
#include "RebarBandStorageData.h"
#include "RebarStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class RebarXmlStorageTest : public XmlStorageTest
{
protected:
	static inline const wchar_t REBAR_NODE_NAME[] = L"Toolbars";
};

TEST_F(RebarXmlStorageTest, Load)
{
	auto referenceRebarStorageInfo = BuildRebarLoadSaveReference();

	std::wstring xmlFilePath = GetResourcePath(L"rebar.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	wil::com_ptr_nothrow<IXMLDOMNode> rebarNode;
	auto queryString = wil::make_bstr_nothrow(REBAR_NODE_NAME);
	HRESULT hr = xmlDocumentData.rootNode->selectSingleNode(queryString.get(), &rebarNode);
	ASSERT_EQ(hr, S_OK);

	auto loadedRebarStorageInfo = RebarXmlStorage::Load(rebarNode.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}

TEST_F(RebarXmlStorageTest, Save)
{
	auto referenceRebarStorageInfo = BuildRebarLoadSaveReference();

	auto xmlDocumentData = CreateXmlDocument();

	wil::com_ptr_nothrow<IXMLDOMElement> rebarNode;
	auto nodeName = wil::make_bstr_nothrow(REBAR_NODE_NAME);
	HRESULT hr = xmlDocumentData.xmlDocument->createElement(nodeName.get(), &rebarNode);
	ASSERT_HRESULT_SUCCEEDED(hr);

	RebarXmlStorage::Save(xmlDocumentData.xmlDocument.get(), rebarNode.get(),
		referenceRebarStorageInfo);
	auto loadedRebarStorageInfo = RebarXmlStorage::Load(rebarNode.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}
