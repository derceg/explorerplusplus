// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainRebarRegistryStorage.h"
#include "MainRebarStorage.h"
#include "MainRebarStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>
#include <wil/registry.h>

using namespace testing;

class MainRebarRegistryStorageTest : public RegistryStorageTest
{
protected:
	static inline const wchar_t MAIN_REBAR_KEY_NAME[] = L"Toolbars";
};

TEST_F(MainRebarRegistryStorageTest, Load)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	ImportRegistryResource(L"main-rebar.reg");

	wil::unique_hkey mainRebarKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(m_applicationTestKey.get(), MAIN_REBAR_KEY_NAME,
		mainRebarKey, wil::reg::key_access::read);
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto loadedRebarStorageInfo = MainRebarRegistryStorage::Load(mainRebarKey.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}

TEST_F(MainRebarRegistryStorageTest, Save)
{
	auto referenceRebarStorageInfo = BuildMainRebarLoadSaveReference();

	wil::unique_hkey mainRebarKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(m_applicationTestKey.get(),
		MAIN_REBAR_KEY_NAME, mainRebarKey, wil::reg::key_access::readwrite);
	ASSERT_HRESULT_SUCCEEDED(hr);

	MainRebarRegistryStorage::Save(mainRebarKey.get(), referenceRebarStorageInfo);
	auto loadedRebarStorageInfo = MainRebarRegistryStorage::Load(mainRebarKey.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}
