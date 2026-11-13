// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/AtomicFile.h"
#include "TemporaryFileTest.h"
#include <wil/resource.h>

class AtomicFileTest : public TemporaryFileTest
{
};

TEST_F(AtomicFileTest, RollbackDoesNotDeleteAFileSubstitutedAtThePublishedPath)
{
	AtomicFile output;
	ASSERT_HRESULT_SUCCEEDED(output.Create(Path(L"output")));
	ASSERT_HRESULT_SUCCEEDED(output.Write("owned", 5));
	ASSERT_HRESULT_SUCCEEDED(output.Flush());
	ASSERT_HRESULT_SUCCEEDED(output.Publish());
	ASSERT_TRUE(MoveFileEx(Path(L"output").c_str(), Path(L"renamed").c_str(), 0));
	Write(L"output", "another file");
	ASSERT_HRESULT_SUCCEEDED(output.Discard());
	EXPECT_EQ(Read(L"output"), "another file");
	EXPECT_FALSE(std::filesystem::exists(Path(L"renamed")));
	ExpectNoTemporaryFiles();
}

TEST_F(AtomicFileTest, FailedReplacementKeepsDestinationAndRemovesOwnedTemporary)
{
	Write(L"output", "previous");
	wil::unique_hfile locked(CreateFile(Path(L"output").c_str(), GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	ASSERT_TRUE(locked);
	AtomicFile output;
	ASSERT_HRESULT_SUCCEEDED(output.Create(Path(L"output")));
	ASSERT_HRESULT_SUCCEEDED(output.Write("replacement", 11));
	ASSERT_HRESULT_SUCCEEDED(output.Flush());
	EXPECT_HRESULT_FAILED(output.Publish(true));
	ASSERT_HRESULT_SUCCEEDED(output.Discard());
	EXPECT_EQ(Read(L"output"), "previous");
	ExpectNoTemporaryFiles();
}
