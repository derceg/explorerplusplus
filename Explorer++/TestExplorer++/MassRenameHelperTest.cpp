// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MassRenameHelper.h"
#include <gtest/gtest.h>

TEST(MassRenameHelperTest, ExpandMassRenamePattern)
{
	EXPECT_EQ(ExpandMassRenamePattern(L"/F", L"file.txt", 0), L"file.txt");
	EXPECT_EQ(ExpandMassRenamePattern(L"/L", L"FILE.TXT", 0), L"file.txt");
	EXPECT_EQ(ExpandMassRenamePattern(L"/U", L"file.txt", 0), L"FILE.TXT");
	EXPECT_EQ(ExpandMassRenamePattern(L"/B", L"file.txt", 0), L"file");
	EXPECT_EQ(ExpandMassRenamePattern(L"/E", L"file.txt", 0), L".txt");
	EXPECT_EQ(ExpandMassRenamePattern(L"/E", L"file", 0), L"");

	// Note that the index provided to the function starts from 0, whereas the index shown to the
	// user should start from 1.
	EXPECT_EQ(ExpandMassRenamePattern(L"/N", L"file.txt", 0), L"1");
	EXPECT_EQ(ExpandMassRenamePattern(L"/00N", L"file.txt", 0), L"001");
	EXPECT_EQ(ExpandMassRenamePattern(L"/000N", L"file.txt", 1000), L"1001");

	EXPECT_EQ(ExpandMassRenamePattern(L"Copy of /F", L"file.txt", 0), L"Copy of file.txt");
	EXPECT_EQ(ExpandMassRenamePattern(L"/B/E", L"archive.tar.gz", 0), L"archive.tar.gz");
	EXPECT_EQ(ExpandMassRenamePattern(L"/B (/N)/E", L"file.txt", 0), L"file (1).txt");
	EXPECT_EQ(ExpandMassRenamePattern(L"/F - /F", L"file.txt", 0), L"file.txt - file.txt");
	EXPECT_EQ(ExpandMassRenamePattern(L"unchanged", L"file.txt", 0), L"unchanged");

	EXPECT_EQ(ExpandMassRenamePattern(L"/N /00N /000N", L"file.txt", 4), L"5 005 0005");
}
