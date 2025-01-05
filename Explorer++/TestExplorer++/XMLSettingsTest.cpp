// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/XMLSettings.h"
#include "Config.h"
#include "ConfigStorageTestHelper.h"
#include "ConfigXmlStorage.h"
#include "Storage.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

class FormatXmlDocumentTest : public XmlStorageTest
{
};

// This does a basic check that data can still be loaded from a document that has been formatted.
TEST_F(FormatXmlDocumentTest, Format)
{
	auto referenceConfig = ConfigStorageTestHelper::BuildReference();
	auto xmlDocumentData = CreateXmlDocument();
	ConfigXmlStorage::Save(xmlDocumentData.xmlDocument.get(), xmlDocumentData.rootNode.get(),
		referenceConfig);

	HRESULT hr = XMLSettings::FormatXmlDocument(xmlDocumentData.xmlDocument.get());
	ASSERT_HRESULT_SUCCEEDED(hr);

	wil::com_ptr_nothrow<IXMLDOMNode> formattedRootNode;
	auto rootName = wil::make_bstr_nothrow(Storage::CONFIG_FILE_ROOT_NODE_NAME);
	hr = xmlDocumentData.xmlDocument->selectSingleNode(rootName.get(), &formattedRootNode);
	ASSERT_EQ(hr, S_OK);

	Config loadedConfig;
	ConfigXmlStorage::Load(formattedRootNode.get(), loadedConfig);

	EXPECT_EQ(loadedConfig, referenceConfig);
}
