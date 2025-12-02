// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellEntryCollection.h"
#include "GeneratorTestHelper.h"
#include "ShellContextFake.h"
#include "ShellEntry.h"
#include <gtest/gtest.h>
#include <format>

using namespace testing;

class ShellEntryCollectionTest : public Test
{
protected:
	ShellEntryCollectionTest() :
		m_fileSystem(m_shellContext.GetFileSystem()),
		m_collection(&m_shellContext, ShellItemFilter::ItemType::FoldersAndFiles,
			ShellItemFilter::HiddenItemPolicy::Include)
	{
	}

	void VerifyMaybeLoadEntryForPidl(const PidlAbsolute &pidl)
	{
		EXPECT_THAT(m_collection.MaybeLoadEntryForPidl(pidl),
			Pointer(Property(&ShellEntry::GetPidl, pidl)));
	}

	ShellContextFake m_shellContext;
	SimulatedFileSystem *const m_fileSystem;
	ShellEntryCollection m_collection;
};

TEST_F(ShellEntryCollectionTest, GetTopLevelEntries)
{
	std::vector<ShellEntry *> topLevelShellEntries;

	for (int i = 0; i < 3; i++)
	{
		auto pidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), std::format(L"Folder {}", i));
		auto *shellEntry = m_collection.AddTopLevelEntry(pidl);
		topLevelShellEntries.push_back(shellEntry);
	}

	EXPECT_EQ(GeneratorToVector(m_collection.GetTopLevelEntries()), topLevelShellEntries);
}

TEST_F(ShellEntryCollectionTest, RemoveTopLevelEntry)
{
	auto rootPidl = m_fileSystem->GetRoot();
	auto folder1Pidl = m_fileSystem->AddFolder(rootPidl, L"Folder 1");
	auto folder2Pidl = m_fileSystem->AddFolder(rootPidl, L"Folder 2");

	auto *shellEntry1 = m_collection.AddTopLevelEntry(folder1Pidl);
	auto *shellEntry2 = m_collection.AddTopLevelEntry(folder2Pidl);

	// A shell item is only watched for changes if it's expanded. So, expanding the top-level items
	// is necessary to test that removal works.
	shellEntry1->LoadChildren();
	shellEntry2->LoadChildren();

	m_fileSystem->RemoveItem(folder1Pidl);
	EXPECT_THAT(GeneratorToVector(m_collection.GetTopLevelEntries()),
		UnorderedElementsAre(shellEntry2));

	m_fileSystem->RemoveItem(folder2Pidl);
	EXPECT_THAT(GeneratorToVector(m_collection.GetTopLevelEntries()), IsEmpty());
}

TEST_F(ShellEntryCollectionTest, GetTopLevelEntryIndex)
{
	for (int i = 0; i < 3; i++)
	{
		auto pidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), std::format(L"Folder {}", i));
		m_collection.AddTopLevelEntry(pidl);
	}

	size_t index = 0;

	for (const auto *shellEntry : m_collection.GetTopLevelEntries())
	{
		EXPECT_EQ(m_collection.GetTopLevelEntryIndex(shellEntry), index++);
	}
}

TEST_F(ShellEntryCollectionTest, MaybeLoadEntryForPidl)
{
	auto folder1Pidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder 1");
	auto folder2Pidl = m_fileSystem->AddFolder(folder1Pidl, L"Folder 2");
	auto folder3Pidl = m_fileSystem->AddFolder(folder2Pidl, L"Folder 3");

	auto folder4Pidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder 4");
	auto folder5Pidl = m_fileSystem->AddFolder(folder4Pidl, L"Folder 5");

	m_collection.AddTopLevelEntry(folder1Pidl);
	m_collection.AddTopLevelEntry(folder4Pidl);

	VerifyMaybeLoadEntryForPidl(folder1Pidl);
	VerifyMaybeLoadEntryForPidl(folder3Pidl);
	VerifyMaybeLoadEntryForPidl(folder2Pidl);

	VerifyMaybeLoadEntryForPidl(folder5Pidl);
	VerifyMaybeLoadEntryForPidl(folder4Pidl);
}

TEST_F(ShellEntryCollectionTest, TopLevelEntryAddedSignal)
{
	auto folderPidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder");

	MockFunction<void(ShellEntry * callbackEntry)> callback;
	m_collection.entryAddedSignal.AddObserver(callback.AsStdFunction());

	PidlAbsolute callbackPidl;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackPidl](ShellEntry *callbackEntry)
			{ callbackPidl = callbackEntry->GetPidl(); });
	m_collection.AddTopLevelEntry(folderPidl);
	EXPECT_EQ(callbackPidl, folderPidl);
}

TEST_F(ShellEntryCollectionTest, EntryAddedSignal)
{
	auto folder1Pidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder 1");
	auto *shellEntry = m_collection.AddTopLevelEntry(folder1Pidl);
	shellEntry->LoadChildren();

	MockFunction<void(ShellEntry * callbackEntry)> callback;
	m_collection.entryAddedSignal.AddObserver(callback.AsStdFunction());

	PidlAbsolute callbackPidl;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackPidl](ShellEntry *callbackEntry)
			{ callbackPidl = callbackEntry->GetPidl(); });
	auto folder2Pidl = m_fileSystem->AddFolder(folder1Pidl, L"Folder 2");
	EXPECT_EQ(callbackPidl, folder2Pidl);
}

TEST_F(ShellEntryCollectionTest, EntryRenamedSignal)
{
	auto folderPidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder");
	auto *shellEntry = m_collection.AddTopLevelEntry(folderPidl);
	shellEntry->LoadChildren();

	MockFunction<void(ShellEntry * callbackEntry)> callback;
	m_collection.entryRenamedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call(shellEntry));
	m_fileSystem->RenameItem(folderPidl, L"Updated name");
}

TEST_F(ShellEntryCollectionTest, EntryUpdatedSignal)
{
	auto folderPidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder");
	auto *shellEntry = m_collection.AddTopLevelEntry(folderPidl);
	shellEntry->LoadChildren();

	MockFunction<void(ShellEntry * callbackEntry)> callback;
	m_collection.entryUpdatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call(shellEntry));
	m_fileSystem->UpdateItem(folderPidl, ShellItemExtraAttributes::Hidden);
}

TEST_F(ShellEntryCollectionTest, TopLevelEntryRemovedSignal)
{
	auto folderPidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder");
	auto *shellEntry = m_collection.AddTopLevelEntry(folderPidl);
	shellEntry->LoadChildren();

	MockFunction<void(ShellEntry * callbackEntry)> callback;
	m_collection.entryRemovedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call(shellEntry));
	m_fileSystem->RemoveItem(folderPidl);
}

TEST_F(ShellEntryCollectionTest, EntryRemovedSignal)
{
	auto folder1Pidl = m_fileSystem->AddFolder(m_fileSystem->GetRoot(), L"Folder 1");
	auto folder2Pidl = m_fileSystem->AddFolder(folder1Pidl, L"Folder 2");
	auto *shellEntry = m_collection.AddTopLevelEntry(folder1Pidl);
	shellEntry->LoadChildren();

	MockFunction<void(ShellEntry * callbackEntry)> callback;
	m_collection.entryRemovedSignal.AddObserver(callback.AsStdFunction());

	PidlAbsolute callbackPidl;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackPidl](ShellEntry *callbackEntry)
			{ callbackPidl = callbackEntry->GetPidl(); });
	m_fileSystem->RemoveItem(folder2Pidl);
	EXPECT_EQ(callbackPidl, folder2Pidl);
}
