// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColorRuleRegistryStorage.h"
#include "ColorRuleModel.h"
#include "ColorRulesStorageTestHelper.h"
#include "MovableModelHelper.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class ColorRuleRegistryStorageTest : public RegistryStorageTest
{
};

TEST_F(ColorRuleRegistryStorageTest, Load)
{
	ColorRuleModel referenceModel;
	BuildLoadSaveReferenceModel(&referenceModel);

	ImportRegistryResource(L"color-rules.reg");

	ColorRuleModel loadedModel;
	ColorRuleRegistryStorage::Load(APPLICATION_TEST_KEY, &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(ColorRuleRegistryStorageTest, Save)
{
	ColorRuleModel referenceModel;
	BuildLoadSaveReferenceModel(&referenceModel);

	ColorRuleRegistryStorage::Save(APPLICATION_TEST_KEY, &referenceModel);

	ColorRuleModel loadedModel;
	ColorRuleRegistryStorage::Load(APPLICATION_TEST_KEY, &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}
