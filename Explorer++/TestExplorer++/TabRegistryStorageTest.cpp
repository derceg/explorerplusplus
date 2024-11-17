// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabRegistryStorage.h"
#include "RegistryStorageTestHelper.h"
#include "TabStorage.h"
#include "TabStorageTestHelper.h"
#include <gtest/gtest.h>
#include <wil/registry.h>

using namespace testing;

class TabRegistryStorageTest : public RegistryStorageTest
{
protected:
	static inline const WCHAR TABS_KEY_NAME[] = L"Tabs";
};

TEST_F(TabRegistryStorageTest, Load)
{
	std::vector<TabStorageData> referenceTabs;
	BuildTabStorageLoadSaveReference(referenceTabs, TestStorageType::Registry);

	ImportRegistryResource(L"tabs.reg");

	wil::unique_hkey tabsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(m_applicationTestKey.get(), TABS_KEY_NAME,
		tabsKey, wil::reg::key_access::read);
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto loadedTabs = TabRegistryStorage::Load(tabsKey.get());

	EXPECT_EQ(loadedTabs, referenceTabs);
}

TEST_F(TabRegistryStorageTest, Save)
{
	std::vector<TabStorageData> referenceTabs;
	BuildTabStorageLoadSaveReference(referenceTabs, TestStorageType::Registry);

	wil::unique_hkey tabsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(m_applicationTestKey.get(), TABS_KEY_NAME,
		tabsKey, wil::reg::key_access::readwrite);
	ASSERT_HRESULT_SUCCEEDED(hr);

	TabRegistryStorage::Save(tabsKey.get(), referenceTabs);
	auto loadedTabs = TabRegistryStorage::Load(tabsKey.get());

	EXPECT_EQ(loadedTabs, referenceTabs);
}
