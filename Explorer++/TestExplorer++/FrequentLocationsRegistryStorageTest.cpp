// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsRegistryStorage.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>

class FrequentLocationsRegistryStorageTest : public RegistryStorageTest
{
};

TEST_F(FrequentLocationsRegistryStorageTest, Load)
{
	FrequentLocationsModel referenceModel;
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	ImportRegistryResource(L"frequent-locations.reg");

	FrequentLocationsModel loadedModel;
	FrequentLocationsRegistryStorage::Load(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(FrequentLocationsRegistryStorageTest, Save)
{
	FrequentLocationsModel referenceModel;
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	FrequentLocationsRegistryStorage::Save(m_applicationTestKey.get(), &referenceModel);

	FrequentLocationsModel loadedModel;
	FrequentLocationsRegistryStorage::Load(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}
