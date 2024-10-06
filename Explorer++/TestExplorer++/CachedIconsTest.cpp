// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/CachedIcons.h"
#include <gtest/gtest.h>

TEST(CachedIconsTest, MaxSize)
{
	CachedIcons cachedIcons(2);

	cachedIcons.AddOrUpdateIcon(L"C:\\file1", 0);
	cachedIcons.AddOrUpdateIcon(L"C:\\file2", 0);

	auto iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\file1");
	EXPECT_NE(iconIndex, std::nullopt);

	cachedIcons.AddOrUpdateIcon(L"C:\\file3", 0);

	// The cache can hold a maximum of 2 icons, so the addition of the third icon above should have
	// pushed out the oldest item.
	iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\file1");
	EXPECT_EQ(iconIndex, std::nullopt);

	// But the second item should still be there.
	iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\file2");
	EXPECT_NE(iconIndex, std::nullopt);
}

TEST(CachedIconsTest, Lookup)
{
	CachedIcons cachedIcons(2);

	cachedIcons.AddOrUpdateIcon(L"C:\\file1", 0);

	auto iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\file1");
	EXPECT_NE(iconIndex, std::nullopt);

	iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\non-existent");
	EXPECT_EQ(iconIndex, std::nullopt);
}

TEST(CachedIconsTest, Update)
{
	CachedIcons cachedIcons(2);

	cachedIcons.AddOrUpdateIcon(L"C:\\file1", 0);
	cachedIcons.AddOrUpdateIcon(L"C:\\file2", 0);

	// This should update the existing entry.
	cachedIcons.AddOrUpdateIcon(L"C:\\file1", 1);
	auto iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\file1");
	EXPECT_EQ(iconIndex, 1);

	cachedIcons.AddOrUpdateIcon(L"C:\\file3", 0);

	// Replacing the item above should have moved it to the front of the list. This means that when
	// the third item was inserted, the second item is what should have been removed.
	iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\file2");
	EXPECT_EQ(iconIndex, std::nullopt);

	// The replaced item should still exist.
	iconIndex = cachedIcons.MaybeGetIconIndex(L"C:\\file1");
	EXPECT_NE(iconIndex, std::nullopt);
}
