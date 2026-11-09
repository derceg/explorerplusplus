// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/XmlAppStorage.h"
#include "RegistryStorageTestHelper.h"
#include "TemporaryFileTest.h"
#include "../Explorer++/Config.h"
#include "../Explorer++/RegistryAppStorage.h"
#include "../Explorer++/RegistryAppStorageFactory.h"
#include "../Explorer++/XmlAppStorageFactory.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/StorageWriteScope.h"
#include "../Helper/XMLSettings.h"
#include <ktmw32.h>
#include <sddl.h>

#pragma comment(lib, "KtmW32.lib")

using Storage::OperationType;
class XmlCommitTest : public TemporaryFileTest
{
};

TEST_F(XmlCommitTest, RepeatedSavesRoundTripAndKeepTheImmediatelyPreviousValidCopy)
{
	for (int revision = 0; revision < 3; ++revision)
	{
		SCOPED_TRACE(revision);
		Config config;
		config.defaultTabDirectory = L"C:\\revision-" + std::to_wstring(revision);
		auto storage = XmlAppStorageFactory::MaybeCreate(Path(L"config.xml"), OperationType::Save);
		ASSERT_TRUE(storage);
		storage->SaveConfig(config);
		storage->SaveDefaultColumns(config.globalFolderSettings.folderColumns);
		ASSERT_HRESULT_SUCCEEDED(storage->Commit());
		EXPECT_HRESULT_FAILED(storage->Commit());
		auto loaded = XmlAppStorageFactory::MaybeCreate(Path(L"config.xml"), OperationType::Load);
		ASSERT_TRUE(loaded);
		Config actual;
		loaded->LoadConfig(actual);
		EXPECT_EQ(actual.defaultTabDirectory, config.defaultTabDirectory);
		if (revision > 0)
		{
			auto previous =
				XmlAppStorageFactory::MaybeCreate(Path(L"config.xml.bak"), OperationType::Load);
			ASSERT_TRUE(previous);
			Config backup;
			previous->LoadConfig(backup);
			EXPECT_EQ(backup.defaultTabDirectory, L"C:\\revision-" + std::to_wstring(revision - 1));
		}
		ExpectNoTemporaryFiles();
	}
}

TEST_F(XmlCommitTest, LockedDestinationKeepsOriginalAndRemovesTemporaryOutput)
{
	Write(L"config.xml", "original");
	wil::unique_hfile locked(CreateFile(Path(L"config.xml").c_str(), GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	ASSERT_TRUE(locked);
	auto storage = XmlAppStorageFactory::MaybeCreate(Path(L"config.xml"), OperationType::Save);
	ASSERT_TRUE(storage);
	Config config;
	storage->SaveConfig(config);
	EXPECT_HRESULT_FAILED(storage->Commit());
	EXPECT_EQ(Read(L"config.xml"), "original");
	ExpectNoTemporaryFiles();
}

TEST_F(XmlCommitTest, ReadonlyDestinationKeepsOriginalAndReportsFailure)
{
	Write(L"config.xml", "original");
	ASSERT_TRUE(SetFileAttributes(Path(L"config.xml").c_str(), FILE_ATTRIBUTE_READONLY));
	auto storage = XmlAppStorageFactory::MaybeCreate(Path(L"config.xml"), OperationType::Save);
	ASSERT_TRUE(storage);
	Config config;
	storage->SaveConfig(config);
	EXPECT_HRESULT_FAILED(storage->Commit());
	EXPECT_EQ(Read(L"config.xml"), "original");
	EXPECT_TRUE(SetFileAttributes(Path(L"config.xml").c_str(), FILE_ATTRIBUTE_NORMAL));
	ExpectNoTemporaryFiles();
}

TEST_F(XmlCommitTest, SerializerFailureCannotPublishAnIncompleteDocument)
{
	Write(L"config.xml", "original");
	auto document = XMLSettings::CreateXmlDocument();
	ASSERT_TRUE(document);
	XmlAppStorage storage(document, nullptr, Path(L"config.xml"), OperationType::Save);
	Config config;
	storage.SaveConfig(config);
	EXPECT_EQ(storage.Commit(), E_POINTER);
	EXPECT_EQ(Read(L"config.xml"), "original");
	ExpectNoTemporaryFiles();
}

TEST_F(XmlCommitTest, ConcurrentWriterIsRejectedWithoutChangingCurrentOrBackup)
{
	Write(L"config.xml", "original");
	Write(L"config.xml.bak", "backup");
	wil::unique_hfile lock(CreateFile(Path(L"config.xml.lock").c_str(), GENERIC_WRITE, 0, nullptr,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
	ASSERT_TRUE(lock);
	auto storage = XmlAppStorageFactory::MaybeCreate(Path(L"config.xml"), OperationType::Save);
	ASSERT_TRUE(storage);
	Config config;
	storage->SaveConfig(config);
	EXPECT_EQ(storage->Commit(), HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION));
	EXPECT_EQ(Read(L"config.xml"), "original");
	EXPECT_EQ(Read(L"config.xml.bak"), "backup");
	ExpectNoTemporaryFiles();
}

TEST_F(XmlCommitTest, MissingMalformedAndWrongRootHaveDistinctLoadResults)
{
	HRESULT error;
	EXPECT_FALSE(XmlAppStorageFactory::MaybeCreate(Path(L"missing"), OperationType::Load, &error));
	EXPECT_EQ(error, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
	Write(L"config.xml", "<ExplorerPlusPlus><broken>");
	EXPECT_FALSE(
		XmlAppStorageFactory::MaybeCreate(Path(L"config.xml"), OperationType::Load, &error));
	EXPECT_HRESULT_FAILED(error);
	EXPECT_NE(error, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
	EXPECT_EQ(Read(L"config.xml"), "<ExplorerPlusPlus><broken>");
	Write(L"config.xml", "<AnotherApplication/>");
	EXPECT_FALSE(
		XmlAppStorageFactory::MaybeCreate(Path(L"config.xml"), OperationType::Load, &error));
	EXPECT_EQ(error, HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
}

TEST_F(XmlCommitTest, RecoveryLoadsValidBackupWithoutChangingEitherFile)
{
	Config expected;
	expected.defaultTabDirectory = L"C:\\Recovered";
	auto storage = XmlAppStorageFactory::MaybeCreate(Path(L"config.xml.bak"), OperationType::Save);
	ASSERT_TRUE(storage);
	storage->SaveConfig(expected);
	ASSERT_HRESULT_SUCCEEDED(storage->Commit());
	auto originalBackup = Read(L"config.xml.bak");
	Write(L"config.xml", "<broken>");
	HRESULT error = S_OK;
	auto recovered = XmlAppStorageFactory::LoadWithRecovery(Path(L"config.xml"), error);
	ASSERT_TRUE(recovered);
	EXPECT_HRESULT_FAILED(error);
	Config actual;
	recovered->LoadConfig(actual);
	EXPECT_EQ(actual.defaultTabDirectory, expected.defaultTabDirectory);
	EXPECT_EQ(Read(L"config.xml"), "<broken>");
	EXPECT_EQ(Read(L"config.xml.bak"), originalBackup);
	ASSERT_TRUE(std::filesystem::remove(Path(L"config.xml")));
	recovered = XmlAppStorageFactory::LoadWithRecovery(Path(L"config.xml"), error);
	ASSERT_TRUE(recovered);
	EXPECT_EQ(error, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
	EXPECT_FALSE(std::filesystem::exists(Path(L"config.xml")));
}
