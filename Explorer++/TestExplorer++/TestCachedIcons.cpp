// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/ShellBrowser/CachedIcons.h"

TEST(TestCachedIcons, TestMaxSize)
{
	CachedIcons cachedIcons(2);

	CachedIcon cachedIcon;
	cachedIcon.filePath = L"C:\\file1";
	cachedIcon.iconIndex = 0;
	cachedIcons.insert(cachedIcon);

	cachedIcon.filePath = L"C:\\file2";
	cachedIcon.iconIndex = 0;
	cachedIcons.insert(cachedIcon);

	auto itr = cachedIcons.findByPath(L"C:\\file1");
	EXPECT_TRUE(itr != cachedIcons.end());

	cachedIcon.filePath = L"C:\\file3";
	cachedIcon.iconIndex = 0;
	cachedIcons.insert(cachedIcon);

	// The cache can hold a maximum of 2 icons, so the addition of the third
	// icon above should have pushed out the oldest item.
	itr = cachedIcons.findByPath(L"C:\\file1");
	EXPECT_TRUE(itr == cachedIcons.end());

	// But the second item should still be there.
	itr = cachedIcons.findByPath(L"C:\\file2");
	EXPECT_TRUE(itr != cachedIcons.end());
}

TEST(TestCachedIcons, TestLookup)
{
	CachedIcons cachedIcons(2);

	CachedIcon cachedIcon;
	cachedIcon.filePath = L"C:\\file1";
	cachedIcon.iconIndex = 0;
	cachedIcons.insert(cachedIcon);

	auto itr = cachedIcons.findByPath(L"C:\\file1");
	EXPECT_TRUE(itr != cachedIcons.end());

	itr = cachedIcons.findByPath(L"C:\\non-existent");
	EXPECT_TRUE(itr == cachedIcons.end());
}

TEST(TestCachedIcons, TestReplace)
{
	CachedIcons cachedIcons(2);

	CachedIcon cachedIcon;
	cachedIcon.filePath = L"C:\\file1";
	cachedIcon.iconIndex = 0;
	cachedIcons.insert(cachedIcon);

	cachedIcon.filePath = L"C:\\file2";
	cachedIcon.iconIndex = 0;
	cachedIcons.insert(cachedIcon);

	auto itr = cachedIcons.findByPath(L"C:\\file1");
	cachedIcon = *itr;
	cachedIcon.iconIndex = 1;
	cachedIcons.replace(itr, cachedIcon);

	itr = cachedIcons.findByPath(L"C:\\file1");
	EXPECT_EQ(itr->iconIndex, 1);

	cachedIcon.filePath = L"C:\\file3";
	cachedIcon.iconIndex = 0;
	cachedIcons.insert(cachedIcon);

	// Replacing the item above should have moved it to the front of the
	// list. This means that when the third item was inserted, the
	// second item is what should have been removed.
	itr = cachedIcons.findByPath(L"C:\\file2");
	EXPECT_TRUE(itr == cachedIcons.end());

	// The replaced item should still exist.
	itr = cachedIcons.findByPath(L"C:\\file1");
	EXPECT_TRUE(itr != cachedIcons.end());
}