// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ConfigXmlStorage.h"
#include "Config.h"
#include "ConfigStorageTestHelper.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

class ConfigXmlStorageTest : public XmlStorageTest
{
};

TEST_F(ConfigXmlStorageTest, SaveLoad)
{
	auto referenceConfig = ConfigStorageTestHelper::BuildReference();
	auto xmlDocumentData = CreateXmlDocument();
	ConfigXmlStorage::Save(xmlDocumentData.xmlDocument.get(), xmlDocumentData.rootNode.get(),
		referenceConfig);

	Config loadedConfig;
	ConfigXmlStorage::Load(xmlDocumentData.rootNode.get(), loadedConfig);

	EXPECT_EQ(loadedConfig, referenceConfig);
}
