// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowXmlStorage.h"
#include "MainRebarStorage.h"
#include "ResourceTestHelper.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include "WindowStorageTestHelper.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace WindowStorageTestHelper;

class WindowXmlStorageTest : public XmlStorageTest
{
};

TEST_F(WindowXmlStorageTest, V2Load)
{
	auto referenceWindows = BuildV2ReferenceWindows(TestStorageType::Xml);

	std::wstring xmlFilePath = GetResourcePath(L"windows-v2-config.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	auto loadedWindows = WindowXmlStorage::Load(xmlDocumentData.rootNode.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowXmlStorageTest, V2LoadFallback)
{
	auto referenceWindow = BuildV2FallbackReferenceWindow(TestStorageType::Xml);

	std::wstring xmlFilePath = GetResourcePath(L"windows-v2-fallback-config.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	auto loadedWindows = WindowXmlStorage::Load(xmlDocumentData.rootNode.get());

	EXPECT_THAT(loadedWindows, ElementsAre(referenceWindow));
}

TEST_F(WindowXmlStorageTest, V2Save)
{
	auto referenceWindows = BuildV2ReferenceWindows(TestStorageType::Xml);

	auto xmlDocumentData = CreateXmlDocument();

	WindowXmlStorage::Save(xmlDocumentData.xmlDocument.get(), xmlDocumentData.rootNode.get(),
		referenceWindows);

	auto loadedWindows = WindowXmlStorage::Load(xmlDocumentData.rootNode.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowXmlStorageTest, V1Load)
{
	auto referenceWindow = BuildV1ReferenceWindow(TestStorageType::Xml);

	std::wstring xmlFilePath = GetResourcePath(L"windows-v1-config.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	auto loadedWindows = WindowXmlStorage::Load(xmlDocumentData.rootNode.get());

	EXPECT_THAT(loadedWindows, ElementsAre(referenceWindow));
}
