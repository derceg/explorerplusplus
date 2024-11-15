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
};

TEST_F(MainRebarXmlStorageTest, Load)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	std::wstring xmlFilePath = GetResourcePath(L"main-rebar-config.xml");
	auto xmlDocument = LoadXmlDocument(xmlFilePath);

	auto loadedRebarStorageInfo = MainRebarXmlStorage::Load(xmlDocument.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}

TEST_F(MainRebarXmlStorageTest, Save)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	auto xmlDocumentData = CreateXmlDocument();

	MainRebarXmlStorage::Save(xmlDocumentData.xmlDocument.get(), xmlDocumentData.rootNode.get(),
		referenceRebarStorageInfo);
	auto loadedRebarStorageInfo = MainRebarXmlStorage::Load(xmlDocumentData.xmlDocument.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}
