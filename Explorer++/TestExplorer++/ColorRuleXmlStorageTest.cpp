// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColorRuleXmlStorage.h"
#include "ColorRuleModel.h"
#include "ColorRulesStorageTestHelper.h"
#include "MovableModelHelper.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class ColorRuleXmlStorageTest : public XmlStorageTest
{
};

TEST_F(ColorRuleXmlStorageTest, Load)
{
	ColorRuleModel referenceModel;
	BuildLoadSaveReferenceModel(&referenceModel);

	std::wstring xmlFilePath = GetResourcePath(L"color-rules-config.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	ColorRuleModel loadedModel;
	ColorRuleXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(ColorRuleXmlStorageTest, Save)
{
	ColorRuleModel referenceModel;
	BuildLoadSaveReferenceModel(&referenceModel);

	auto xmlDocumentData = CreateXmlDocument();

	ColorRuleXmlStorage::Save(xmlDocumentData.xmlDocument.get(), xmlDocumentData.rootNode.get(),
		&referenceModel);

	ColorRuleModel loadedModel;
	ColorRuleXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}
