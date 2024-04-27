// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainRebarRegistryStorage.h"
#include "MainRebarStorage.h"
#include "MainRebarStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class MainRebarRegistryStorageTest : public RegistryStorageTest
{
};

TEST_F(MainRebarRegistryStorageTest, Load)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	ImportRegistryResource(L"main-rebar.reg");

	auto loadedRebarStorageInfo = MainRebarRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}

TEST_F(MainRebarRegistryStorageTest, Save)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	MainRebarRegistryStorage::Save(m_applicationTestKey.get(), referenceRebarStorageInfo);
	auto loadedRebarStorageInfo = MainRebarRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}
