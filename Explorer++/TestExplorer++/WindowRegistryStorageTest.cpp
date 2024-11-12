// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowRegistryStorage.h"
#include "RegistryStorageTestHelper.h"
#include "WindowStorage.h"
#include "WindowStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace WindowStorageTestHelper;

class WindowRegistryStorageTest : public RegistryStorageTest
{
};

TEST_F(WindowRegistryStorageTest, V2Load)
{
	auto referenceWindows = BuildV2ReferenceWindows();

	ImportRegistryResource(L"windows-v2.reg");

	auto loadedWindows = WindowRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowRegistryStorageTest, V2Save)
{
	auto referenceWindows = BuildV2ReferenceWindows();

	WindowRegistryStorage::Save(m_applicationTestKey.get(), referenceWindows);

	auto loadedWindows = WindowRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowRegistryStorageTest, V1Load)
{
	auto referenceWindow = BuildV1ReferenceWindow();

	ImportRegistryResource(L"windows-v1.reg");

	auto loadedWindows = WindowRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_THAT(loadedWindows, ElementsAre(referenceWindow));
}
