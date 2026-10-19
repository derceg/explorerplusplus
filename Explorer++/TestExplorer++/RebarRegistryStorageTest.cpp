// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RebarRegistryStorage.h"
#include "RebarBandStorageData.h"
#include "RebarStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>
#include <wil/registry.h>

using namespace testing;

class RebarRegistryStorageTest : public RegistryStorageTest
{
protected:
	static inline const wchar_t REBAR_KEY_NAME[] = L"Toolbars";
};

TEST_F(RebarRegistryStorageTest, Load)
{
	auto referenceRebarStorageInfo = BuildRebarLoadSaveReference();

	ImportRegistryResource(L"rebar.reg");

	wil::unique_hkey rebarKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(m_applicationTestKey.get(), REBAR_KEY_NAME,
		rebarKey, wil::reg::key_access::read);
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto loadedRebarStorageInfo = RebarRegistryStorage::Load(rebarKey.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}

TEST_F(RebarRegistryStorageTest, Save)
{
	auto referenceRebarStorageInfo = BuildRebarLoadSaveReference();

	wil::unique_hkey rebarKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(m_applicationTestKey.get(), REBAR_KEY_NAME,
		rebarKey, wil::reg::key_access::readwrite);
	ASSERT_HRESULT_SUCCEEDED(hr);

	RebarRegistryStorage::Save(rebarKey.get(), referenceRebarStorageInfo);
	auto loadedRebarStorageInfo = RebarRegistryStorage::Load(rebarKey.get());

	EXPECT_EQ(loadedRebarStorageInfo, referenceRebarStorageInfo);
}
