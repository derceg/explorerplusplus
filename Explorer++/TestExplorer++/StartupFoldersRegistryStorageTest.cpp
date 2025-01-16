// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "StartupFoldersRegistryStorage.h"
#include "RegistryStorageTestHelper.h"
#include "StartupFoldersStorageTestHelper.h"
#include <gtest/gtest.h>
#include <wil/registry.h>

class StartupFoldersRegistryStorageTest : public RegistryStorageTest
{
protected:
	static constexpr wchar_t STARTUP_FOLDERS_KEY_NAME[] = L"StartupFolders";
};

TEST_F(StartupFoldersRegistryStorageTest, Load)
{
	auto referenceStartupFolders = StartupFoldersStorageTestHelper::BuildReference();

	ImportRegistryResource(L"startup-folders.reg");

	wil::unique_hkey startupFoldersKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(m_applicationTestKey.get(),
		STARTUP_FOLDERS_KEY_NAME, startupFoldersKey, wil::reg::key_access::read);
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto loadedStartupFolders = StartupFoldersRegistryStorage::Load(startupFoldersKey.get());

	EXPECT_EQ(loadedStartupFolders, referenceStartupFolders);
}

TEST_F(StartupFoldersRegistryStorageTest, Save)
{
	auto referenceStartupFolders = StartupFoldersStorageTestHelper::BuildReference();

	wil::unique_hkey startupFoldersKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(m_applicationTestKey.get(),
		STARTUP_FOLDERS_KEY_NAME, startupFoldersKey, wil::reg::key_access::readwrite);
	ASSERT_HRESULT_SUCCEEDED(hr);

	StartupFoldersRegistryStorage::Save(startupFoldersKey.get(), referenceStartupFolders);
	auto loadedStartupFolders = StartupFoldersRegistryStorage::Load(startupFoldersKey.get());

	EXPECT_EQ(loadedStartupFolders, referenceStartupFolders);
}
