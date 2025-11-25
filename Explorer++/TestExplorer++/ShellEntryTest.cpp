// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellEntry.h"
#include "GeneratorTestHelper.h"
#include "PidlTestHelper.h"
#include "ShellContextFake.h"
#include "SimulatedFileSystem.h"
#include "../Helper/Pidl.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace testing;

namespace
{

MATCHER(ShellEntryPidlMatcher, "")
{
	return std::get<0>(arg)->GetPidl() == std::get<1>(arg);
}

}

class ShellEntryTest : public Test
{
protected:
	ShellEntryTest() :
		m_fileSystem(m_shellContext.GetFileSystem()),
		m_rootPidl(m_fileSystem->GetRoot())
	{
	}

	void VerifyFolderContents(const ShellEntry *shellEntry,
		const std::vector<PidlAbsolute> &expectedPidls)
	{
		ASSERT_TRUE(shellEntry->AreChildrenLoaded());
		EXPECT_THAT(GeneratorToVector(shellEntry->GetChildren()),
			UnorderedPointwise(ShellEntryPidlMatcher(), expectedPidls));
	}

	ShellContextFake m_shellContext;
	SimulatedFileSystem *const m_fileSystem;
	PidlAbsolute m_rootPidl;
};

TEST_F(ShellEntryTest, LoadChildren)
{
	auto folder1Pidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder 1");
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");
	auto folder2Pidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder 2");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();
	VerifyFolderContents(&rootShellEntry, { folder1Pidl, filePidl, folder2Pidl });
}

TEST_F(ShellEntryTest, LoadChildrenFoldersOnly)
{
	auto folder1Pidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder 1");
	m_fileSystem->AddFile(m_rootPidl, L"File");
	auto folder2Pidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder 2");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext, ShellItemFilter::ItemType::FoldersOnly,
		ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();
	VerifyFolderContents(&rootShellEntry, { folder1Pidl, folder2Pidl });
}

TEST_F(ShellEntryTest, UnloadChildren)
{
	m_fileSystem->AddFolder(m_rootPidl, L"Folder");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();
	rootShellEntry.UnloadChildren();
	EXPECT_THAT(GeneratorToVector(rootShellEntry.GetChildren()), IsEmpty());
}

TEST_F(ShellEntryTest, AreChildrenLoaded)
{
	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	EXPECT_FALSE(rootShellEntry.AreChildrenLoaded());

	rootShellEntry.LoadChildren();
	EXPECT_TRUE(rootShellEntry.AreChildrenLoaded());

	rootShellEntry.UnloadChildren();
	EXPECT_FALSE(rootShellEntry.AreChildrenLoaded());
}

TEST_F(ShellEntryTest, GetParent)
{
	auto folder1Pidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder 1");
	auto folder2Pidl = m_fileSystem->AddFolder(folder1Pidl, L"Folder 2");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	EXPECT_EQ(rootShellEntry.GetParent(), nullptr);

	rootShellEntry.LoadChildren();
	auto *folder1ShellEntry = rootShellEntry.MaybeGetChild(folder1Pidl);
	ASSERT_NE(folder1ShellEntry, nullptr);
	EXPECT_EQ(folder1ShellEntry->GetParent(), &rootShellEntry);

	folder1ShellEntry->LoadChildren();
	auto *folder2ShellEntry = folder1ShellEntry->MaybeGetChild(folder2Pidl);
	ASSERT_NE(folder2ShellEntry, nullptr);
	EXPECT_EQ(folder2ShellEntry->GetParent(), folder1ShellEntry);
}

TEST_F(ShellEntryTest, AddItem)
{
	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");
	VerifyFolderContents(&rootShellEntry, { filePidl });
}

TEST_F(ShellEntryTest, AddItemFoldersOnly)
{
	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext, ShellItemFilter::ItemType::FoldersOnly,
		ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	auto folderPidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder");
	m_fileSystem->AddFile(m_rootPidl, L"File");
	VerifyFolderContents(&rootShellEntry, { folderPidl });
}

TEST_F(ShellEntryTest, AddHiddenItem)
{
	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Exclude);
	rootShellEntry.LoadChildren();

	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");
	m_fileSystem->AddFile(m_rootPidl, L"Hidden file", ShellItemExtraAttributes::Hidden);
	VerifyFolderContents(&rootShellEntry, { filePidl });
}

TEST_F(ShellEntryTest, RenameItem)
{
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	auto updatedFilePidl = m_fileSystem->RenameItem(filePidl, L"Updated name");
	VerifyFolderContents(&rootShellEntry, { updatedFilePidl });
}

TEST_F(ShellEntryTest, RenameTopLevelItem)
{
	auto folderPidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder");
	m_fileSystem->AddFile(folderPidl, L"File");

	ShellEntry shellEntry(folderPidl, &m_shellContext, ShellItemFilter::ItemType::FoldersAndFiles,
		ShellItemFilter::HiddenItemPolicy::Include);
	shellEntry.LoadChildren();

	m_fileSystem->RenameItem(folderPidl, L"Updated name");
	VerifyFolderContents(&shellEntry,
		{ CreateSimplePidlForTest(
			SimulatedFileSystem::ROOT_FOLDER_PATH + L"\\Updated name\\File"s) });
}

TEST_F(ShellEntryTest, RenameNonFolderToFolder)
{
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File.zip.tmp");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext, ShellItemFilter::ItemType::FoldersOnly,
		ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	// This mimics the behavior that can be seen when renaming a file. That is, the file might start
	// as a regular file, but then change to a container file on rename (and thus function as a
	// virtual folder).
	auto updatedFilePidl = m_fileSystem->RenameItem(filePidl, L"File.zip", ShellItemType::Folder);
	VerifyFolderContents(&rootShellEntry, { updatedFilePidl });
}

TEST_F(ShellEntryTest, RenameFolderToNonFolder)
{
	auto filePidl = m_fileSystem->AddFolder(m_rootPidl, L"File.zip");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext, ShellItemFilter::ItemType::FoldersOnly,
		ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	m_fileSystem->RenameItem(filePidl, L"File.zip.tmp", ShellItemType::File);
	VerifyFolderContents(&rootShellEntry, {});
}

TEST_F(ShellEntryTest, RenameItemUpdatesChildren)
{
	auto folder1Pidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder 1");
	auto nestedFolder1Pidl = m_fileSystem->AddFolder(folder1Pidl, L"Nested folder 1");
	m_fileSystem->AddFolder(nestedFolder1Pidl, L"Nested folder 2");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	auto *folder1ShellEntry = rootShellEntry.MaybeGetChild(folder1Pidl);
	ASSERT_NE(folder1ShellEntry, nullptr);
	folder1ShellEntry->LoadChildren();

	auto *nestedFolder1ShellEntry = folder1ShellEntry->MaybeGetChild(nestedFolder1Pidl);
	ASSERT_NE(nestedFolder1ShellEntry, nullptr);
	nestedFolder1ShellEntry->LoadChildren();

	auto folder1PidlUpdated = m_fileSystem->RenameItem(folder1Pidl, L"Updated name");
	EXPECT_EQ(rootShellEntry.MaybeGetChild(folder1PidlUpdated), folder1ShellEntry);

	auto nestedFolder1PidlUpdated = CreateSimplePidlForTest(
		SimulatedFileSystem::ROOT_FOLDER_PATH + L"\\Updated name\\Nested folder 1"s);
	EXPECT_EQ(folder1ShellEntry->MaybeGetChild(nestedFolder1PidlUpdated), nestedFolder1ShellEntry);

	// Directory monitoring should still work after an item has been renamed. That is, once an item
	// has been renamed, the updated path should be watched for changes.
	auto newFilePidl = m_fileSystem->AddFile(folder1PidlUpdated, L"New file");
	VerifyFolderContents(folder1ShellEntry, { nestedFolder1PidlUpdated, newFilePidl });

	auto newFolderPidl = m_fileSystem->AddFolder(nestedFolder1PidlUpdated, L"New folder");
	VerifyFolderContents(nestedFolder1ShellEntry,
		{ CreateSimplePidlForTest(SimulatedFileSystem::ROOT_FOLDER_PATH
			  + L"\\Updated name\\Nested folder 1\\Nested folder 2"s),
			newFolderPidl });
}

TEST_F(ShellEntryTest, UpdateHiddenItemToNonHidden)
{
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File", ShellItemExtraAttributes::Hidden);

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Exclude);
	rootShellEntry.LoadChildren();

	m_fileSystem->UpdateItem(filePidl, ShellItemExtraAttributes::None);
	VerifyFolderContents(&rootShellEntry, { filePidl });
}

TEST_F(ShellEntryTest, UpdateNonHiddenItemToHidden)
{
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Exclude);
	rootShellEntry.LoadChildren();

	m_fileSystem->UpdateItem(filePidl, ShellItemExtraAttributes::Hidden);
	VerifyFolderContents(&rootShellEntry, {});
}

TEST_F(ShellEntryTest, RemoveItem)
{
	auto folderPidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	m_fileSystem->RemoveItem(folderPidl);
	EXPECT_THAT(GeneratorToVector(rootShellEntry.GetChildren()), IsEmpty());
}

TEST_F(ShellEntryTest, ChildAddedSignal)
{
	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	MockFunction<void(ShellEntry * shellEntry)> callback;
	rootShellEntry.childAddedSignal.AddObserver(callback.AsStdFunction());

	PidlAbsolute callbackPidl;
	EXPECT_CALL(callback, Call(_))
		.WillOnce(
			[&callbackPidl](ShellEntry *shellEntry) { callbackPidl = shellEntry->GetPidl(); });
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");
	EXPECT_EQ(callbackPidl, filePidl);
}

TEST_F(ShellEntryTest, ChildRemovedSignal)
{
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	MockFunction<void(ShellEntry * childEntry)> callback;
	rootShellEntry.childRemovedSignal.AddObserver(callback.AsStdFunction());

	auto *fileShellEntry = rootShellEntry.MaybeGetChild(filePidl);
	ASSERT_NE(fileShellEntry, nullptr);

	EXPECT_CALL(callback, Call(fileShellEntry));
	m_fileSystem->RemoveItem(filePidl);
}

TEST_F(ShellEntryTest, RenamedSignal)
{
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	auto *fileShellEntry = rootShellEntry.MaybeGetChild(filePidl);
	ASSERT_NE(fileShellEntry, nullptr);

	MockFunction<void()> callback;
	fileShellEntry->renamedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	m_fileSystem->RenameItem(filePidl, L"Updated name");
}

TEST_F(ShellEntryTest, UpdatedSignal)
{
	auto filePidl = m_fileSystem->AddFile(m_rootPidl, L"File");

	ShellEntry rootShellEntry(m_rootPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	auto *fileShellEntry = rootShellEntry.MaybeGetChild(filePidl);
	ASSERT_NE(fileShellEntry, nullptr);

	MockFunction<void()> callback;
	fileShellEntry->updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	m_fileSystem->UpdateItem(filePidl, ShellItemExtraAttributes::None);
}

TEST_F(ShellEntryTest, TopLevelItemUpdatedSignal)
{
	auto folderPidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder");

	ShellEntry rootShellEntry(folderPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	rootShellEntry.LoadChildren();

	MockFunction<void()> callback;
	rootShellEntry.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	m_fileSystem->UpdateItem(folderPidl, ShellItemExtraAttributes::None);
}

TEST_F(ShellEntryTest, TopLevelEntryDeletedSignal)
{
	auto folderPidl = m_fileSystem->AddFolder(m_rootPidl, L"Folder");

	ShellEntry folderShellEntry(folderPidl, &m_shellContext,
		ShellItemFilter::ItemType::FoldersAndFiles, ShellItemFilter::HiddenItemPolicy::Include);
	folderShellEntry.LoadChildren();

	MockFunction<void()> callback;
	folderShellEntry.topLevelEntryDeletedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	m_fileSystem->RemoveItem(folderPidl);
}
