// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/RegistryAppStorage.h"
#include "RegistryStorageTestHelper.h"
#include "TemporaryFileTest.h"
#include "../Explorer++/Config.h"
#include "../Explorer++/RegistryAppStorageFactory.h"
#include "../Explorer++/XmlAppStorage.h"
#include "../Explorer++/XmlAppStorageFactory.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/StorageWriteScope.h"
#include "../Helper/XMLSettings.h"
#include <ktmw32.h>
#include <sddl.h>

#pragma comment(lib, "KtmW32.lib")

using Storage::OperationType;
class RegistryCommitTest : public RegistryStorageTest
{
};

TEST_F(RegistryCommitTest, AbandoningAnUncommittedSaveKeepsTheEntirePreviousTree)
{
	wil::unique_hkey child;
	ASSERT_HRESULT_SUCCEEDED(
		RegistrySettings::CreateKey(m_applicationTestKey.get(), L"Child\\Nested", child));
	ASSERT_EQ(RegistrySettings::SaveString(child.get(), L"Keep", L"original"), ERROR_SUCCESS);
	child.reset();
	{
		HRESULT error;
		auto storage = RegistryAppStorageFactory::MaybeCreate(APPLICATION_TEST_KEY,
			OperationType::Save, &error);
		ASSERT_TRUE(storage) << error;
		Config config;
		config.alwaysOpenNewTab = true;
		storage->SaveConfig(config);
		// Destruction simulates an abandoned/failed save before its commit.
	}
	ASSERT_EQ(RegOpenKeyEx(m_applicationTestKey.get(), L"Child\\Nested", 0, KEY_READ, child.put()),
		ERROR_SUCCESS);
	std::wstring value;
	ASSERT_EQ(RegistrySettings::ReadString(child.get(), L"Keep", value), ERROR_SUCCESS);
	EXPECT_EQ(value, L"original");
}

TEST_F(RegistryCommitTest, CommitPublishesACompleteReplacementAndRemovesStaleItems)
{
	wil::unique_hkey stale;
	ASSERT_HRESULT_SUCCEEDED(
		RegistrySettings::CreateKey(m_applicationTestKey.get(), L"Obsolete\\Child", stale));
	stale.reset();
	Config config;
	config.alwaysOpenNewTab = true;
	config.defaultTabDirectory = L"C:\\Saved folder";
	auto storage =
		RegistryAppStorageFactory::MaybeCreate(APPLICATION_TEST_KEY, OperationType::Save);
	ASSERT_TRUE(storage);
	storage->SaveConfig(config);
	storage->SaveDefaultColumns(config.globalFolderSettings.folderColumns);
	ASSERT_HRESULT_SUCCEEDED(storage->Commit());
	EXPECT_HRESULT_FAILED(storage->Commit());
	storage.reset();
	auto loaded = RegistryAppStorageFactory::MaybeCreate(APPLICATION_TEST_KEY, OperationType::Load);
	ASSERT_TRUE(loaded);
	Config actual;
	loaded->LoadConfig(actual);
	EXPECT_TRUE(actual.alwaysOpenNewTab);
	EXPECT_EQ(actual.defaultTabDirectory, config.defaultTabDirectory);
	EXPECT_EQ(RegOpenKeyEx(m_applicationTestKey.get(), L"Obsolete", 0, KEY_READ, stale.put()),
		ERROR_FILE_NOT_FOUND);
}

TEST_F(RegistryCommitTest, FailedKeyCreationIsReportedInsteadOfCommittingAPartialSnapshot)
{
	// A read-only handle alone does not prevent RegCreateKeyEx from creating a
	// child: Windows checks the parent's security descriptor for that operation.
	DWORD size = 0;
	ASSERT_EQ(
		RegGetKeySecurity(m_applicationTestKey.get(), DACL_SECURITY_INFORMATION, nullptr, &size),
		ERROR_INSUFFICIENT_BUFFER);
	std::vector<BYTE> previous(size);
	ASSERT_EQ(RegGetKeySecurity(m_applicationTestKey.get(), DACL_SECURITY_INFORMATION,
				  previous.data(), &size),
		ERROR_SUCCESS);
	PSECURITY_DESCRIPTOR descriptor = nullptr;
	ASSERT_TRUE(ConvertStringSecurityDescriptorToSecurityDescriptor(L"D:P(A;;KR;;;WD)",
		SDDL_REVISION_1, &descriptor, nullptr));
	auto freeDescriptor = wil::scope_exit([&] { LocalFree(descriptor); });
	auto restore = wil::scope_exit(
		[&]
		{
			EXPECT_EQ(RegSetKeySecurity(m_applicationTestKey.get(), DACL_SECURITY_INFORMATION,
						  previous.data()),
				ERROR_SUCCESS);
		});
	ASSERT_EQ(RegSetKeySecurity(m_applicationTestKey.get(), DACL_SECURITY_INFORMATION, descriptor),
		ERROR_SUCCESS);
	wil::unique_hkey readonly;
	ASSERT_EQ(
		RegOpenKeyEx(HKEY_CURRENT_USER, APPLICATION_TEST_KEY.c_str(), 0, KEY_READ, readonly.put()),
		ERROR_SUCCESS);
	RegistryAppStorage storage(std::move(readonly));
	Config config;
	storage.SaveConfig(config);
	EXPECT_EQ(storage.Commit(), HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
}

TEST_F(RegistryCommitTest, FailedValueWriteIsCapturedAcrossNestedHelpers)
{
	HRESULT error = S_OK;
	{
		StorageWriteScope scope(error);
		EXPECT_NE(RegistrySettings::SaveDword(nullptr, L"Invalid", 1), ERROR_SUCCESS);
		RegistrySettings::SaveDword(m_applicationTestKey.get(), L"SuccessfulLaterWrite", 2);
	}
	EXPECT_HRESULT_FAILED(error);
	HRESULT next = S_OK;
	{
		StorageWriteScope scope(next);
		EXPECT_EQ(RegistrySettings::SaveDword(m_applicationTestKey.get(), L"Value", 3),
			ERROR_SUCCESS);
	}
	EXPECT_EQ(next, S_OK);
}
