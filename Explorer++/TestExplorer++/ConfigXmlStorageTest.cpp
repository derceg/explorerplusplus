// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ConfigXmlStorage.h"
#include "Config.h"
#include "ConfigStorageTestHelper.h"
#include "XmlStorageTestHelper.h"
#include "../Explorer++/Storage.h"
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

TEST_F(ConfigXmlStorageTest, ConfigEnvVar)
{
	auto set = SetEnvironmentVariable(Storage::CONFIG_FILE_ENV_VAR_NAME, NULL);
	ASSERT_TRUE(set);
	auto path = Storage::GetConfigFilePath();
	ASSERT_TRUE(path.ends_with(L"config.xml"));

	set = SetEnvironmentVariable(L"FOO", L"BAR");
	ASSERT_TRUE(set);
	set = SetEnvironmentVariable(Storage::CONFIG_FILE_ENV_VAR_NAME, L"%FOO%\\explorerpp.xml");
	ASSERT_TRUE(set);
	path = Storage::GetConfigFilePath();
	ASSERT_EQ(path, L"BAR\\explorerpp.xml");

	set = SetEnvironmentVariable(Storage::CONFIG_FILE_ENV_VAR_NAME, NULL);
	ASSERT_TRUE(set);
	path = Storage::GetConfigFilePath();
	ASSERT_TRUE(path.ends_with(L"config.xml"));
}
