// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Version.h"
#include <gtest/gtest.h>

using namespace testing;

class VersionTest : public Test
{
protected:
	void CheckLessThan(const Version &v1, const Version &v2)
	{
		EXPECT_LE(v1, v2);
	}

	void CheckGreaterThan(const Version &v1, const Version &v2)
	{
		EXPECT_GT(v1, v2);
	}

	void CheckEquality(const Version &v1, const Version &v2, bool equal)
	{
		EXPECT_EQ(v1 == v2, equal);
	}

	void CheckSegments(const std::initializer_list<uint32_t> &segments)
	{
		Version v{ segments };
		EXPECT_THAT(v.GetSegments(), ElementsAreArray(segments));
	}

	void CheckString(const Version &v, const std::wstring &expected)
	{
		EXPECT_EQ(v.GetString(), expected);
	}
};

TEST_F(VersionTest, Ordering)
{
	CheckLessThan({ 4 }, { 5 });
	CheckLessThan({ 8, 1 }, { 8, 2 });
	CheckLessThan({ 2, 0, 1 }, { 3, 8, 10 });
	CheckLessThan({ 1, 6, 1, 0 }, { 1, 8, 1, 0 });
	CheckGreaterThan({ 6 }, { 3 });
	CheckGreaterThan({ 4, 9 }, { 3 });
	CheckGreaterThan({ 2, 6, 12 }, { 2, 6, 10 });
	CheckGreaterThan({ 1, 7, 9, 100 }, { 1, 7, 9, 84 });
}

TEST_F(VersionTest, Equality)
{
	CheckEquality({ 1 }, { 2 }, false);
	CheckEquality({ 1, 4 }, { 1, 3 }, false);
	CheckEquality({ 1, 6, 2 }, { 1, 6, 1 }, false);
	CheckEquality({ 3, 22, 1, 0 }, { 4, 22, 1, 0 }, false);
	CheckEquality({ 6 }, { 6 }, true);
	CheckEquality({ 1, 7 }, { 1, 7 }, true);
	CheckEquality({ 1, 8, 3 }, { 1, 8, 3 }, true);
	CheckEquality({ 2, 5, 0, 0 }, { 2, 5, 0, 0 }, true);
}

TEST_F(VersionTest, GetSegments)
{
	CheckSegments({ 2 });
	CheckSegments({ 6, 8 });
	CheckSegments({ 3, 11, 18 });
	CheckSegments({ 10, 0, 0, 1264 });
}

TEST_F(VersionTest, GetString)
{
	CheckString({ 3 }, L"3");
	CheckString({ 1, 6 }, L"1.6");
	CheckString({ 4, 9, 3 }, L"4.9.3");
	CheckString({ 2, 3, 11, 863 }, L"2.3.11.863");
}
