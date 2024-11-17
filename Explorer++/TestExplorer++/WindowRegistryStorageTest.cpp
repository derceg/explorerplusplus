// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowRegistryStorage.h"
#include "MainRebarStorage.h"
#include "RegistryStorageTestHelper.h"
#include "TabStorage.h"
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
	auto referenceWindows = BuildV2ReferenceWindows(TestStorageType::Registry);

	ImportRegistryResource(L"windows-v2.reg");

	auto loadedWindows = WindowRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowRegistryStorageTest, V2LoadFallback)
{
	auto referenceWindow = BuildV2FallbackReferenceWindow(TestStorageType::Registry);

	// In this case, some of the window data is stored in the original format.
	// WindowRegistryStorage::Load() should detect that the data isn't present within the windows
	// registry key and fallback.
	ImportRegistryResource(L"windows-v2-fallback.reg");

	auto loadedWindows = WindowRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_THAT(loadedWindows, ElementsAre(referenceWindow));
}

TEST_F(WindowRegistryStorageTest, V2Save)
{
	auto referenceWindows = BuildV2ReferenceWindows(TestStorageType::Registry);

	WindowRegistryStorage::Save(m_applicationTestKey.get(), referenceWindows);

	auto loadedWindows = WindowRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_EQ(loadedWindows, referenceWindows);
}

TEST_F(WindowRegistryStorageTest, V1Load)
{
	auto referenceWindow = BuildV1ReferenceWindow(TestStorageType::Registry);

	ImportRegistryResource(L"windows-v1.reg");

	auto loadedWindows = WindowRegistryStorage::Load(m_applicationTestKey.get());

	EXPECT_THAT(loadedWindows, ElementsAre(referenceWindow));
}
