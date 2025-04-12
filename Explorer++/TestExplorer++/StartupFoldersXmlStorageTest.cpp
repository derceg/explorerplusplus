// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "StartupFoldersXmlStorage.h"
#include "ResourceTestHelper.h"
#include "StartupFoldersStorageTestHelper.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

class StartupFoldersXmlStorageTest : public XmlStorageTest
{
protected:
	static constexpr wchar_t STARTUP_FOLDERS_NODE_NAME[] = L"StartupFolders";
};

TEST_F(StartupFoldersXmlStorageTest, Load)
{
	auto referenceStartupFolders = StartupFoldersStorageTestHelper::BuildReference();

	auto xmlDocumentData = LoadXmlDocument(GetResourcePath(L"startup-folders.xml"));

	wil::com_ptr_nothrow<IXMLDOMNode> startupFoldersNode;
	HRESULT hr = xmlDocumentData.rootNode->selectSingleNode(
		wil::make_bstr_failfast(STARTUP_FOLDERS_NODE_NAME).get(), &startupFoldersNode);
	ASSERT_EQ(hr, S_OK);

	auto loadedStartupFolders = StartupFoldersXmlStorage::Load(startupFoldersNode.get());

	EXPECT_EQ(loadedStartupFolders, referenceStartupFolders);
}

TEST_F(StartupFoldersXmlStorageTest, Save)
{
	auto referenceStartupFolders = StartupFoldersStorageTestHelper::BuildReference();

	auto xmlDocumentData = CreateXmlDocument();

	wil::com_ptr_nothrow<IXMLDOMElement> startupFoldersNode;
	HRESULT hr = xmlDocumentData.xmlDocument->createElement(
		wil::make_bstr_nothrow(STARTUP_FOLDERS_NODE_NAME).get(), &startupFoldersNode);
	ASSERT_HRESULT_SUCCEEDED(hr);
	StartupFoldersXmlStorage::Save(xmlDocumentData.xmlDocument.get(), startupFoldersNode.get(),
		referenceStartupFolders);

	auto loadedStartupFolders = StartupFoldersXmlStorage::Load(startupFoldersNode.get());

	EXPECT_EQ(loadedStartupFolders, referenceStartupFolders);
}
