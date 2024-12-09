// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsRegistryStorage.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include "../Helper/SystemClockImpl.h"
#include <gtest/gtest.h>

class FrequentLocationsRegistryStorageTest : public RegistryStorageTest
{
protected:
	SystemClockImpl m_systemClock;
};

TEST_F(FrequentLocationsRegistryStorageTest, Load)
{
	FrequentLocationsModel referenceModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	ImportRegistryResource(L"frequent-locations.reg");

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsRegistryStorage::Load(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(FrequentLocationsRegistryStorageTest, Save)
{
	FrequentLocationsModel referenceModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	FrequentLocationsRegistryStorage::Save(m_applicationTestKey.get(), &referenceModel);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsRegistryStorage::Load(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}
