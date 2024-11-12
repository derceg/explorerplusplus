// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowXmlStorage.h"
#include "ResourceTestHelper.h"
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
	auto referenceWindows = BuildV2ReferenceWindows();

	std::wstring xmlFilePath = GetResourcePath(L"windows-v2-config.xml");
	auto xmlDocument = LoadXmlDocument(xmlFilePath);

	auto loadedWindows = WindowXmlStorage::Load(xmlDocument.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowXmlStorageTest, V2Save)
{
	auto referenceWindows = BuildV2ReferenceWindows();

	auto xmlDocumentData = CreateXmlDocument();

	WindowXmlStorage::Save(xmlDocumentData.xmlDocument.get(), xmlDocumentData.root.get(),
		referenceWindows);

	auto loadedWindows = WindowXmlStorage::Load(xmlDocumentData.xmlDocument.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowXmlStorageTest, V1Load)
{
	auto referenceWindow = BuildV1ReferenceWindow();

	std::wstring xmlFilePath = GetResourcePath(L"windows-v1-config.xml");
	auto xmlDocument = LoadXmlDocument(xmlFilePath);

	auto loadedWindows = WindowXmlStorage::Load(xmlDocument.get());

	EXPECT_THAT(loadedWindows, ElementsAre(referenceWindow));
}
