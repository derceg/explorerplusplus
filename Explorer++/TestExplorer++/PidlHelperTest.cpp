// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/PidlHelper.h"
#include "ShellTestHelper.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

void TestPidlEquality(const std::wstring &path1, const std::wstring &path2, bool equivalent)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(path1);
	PidlAbsolute pidl2 = CreateSimplePidlForTest(path2);

	bool res = (pidl1 == pidl2);
	EXPECT_EQ(res, equivalent);
}

}

TEST(PidlAbsolute, Empty)
{
	PidlAbsolute pidl;
	EXPECT_FALSE(pidl.HasValue());
	EXPECT_EQ(pidl.Raw(), nullptr);
}

TEST(PidlAbsolute, ConstructWithPointer)
{
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	PidlAbsolute pidl(ownedPidl.get());
	EXPECT_TRUE(pidl.HasValue());
	EXPECT_NE(pidl.Raw(), ownedPidl.get());
	EXPECT_TRUE(ArePidlsEquivalent(pidl.Raw(), ownedPidl.get()));

	PidlAbsolute pidl2(nullptr);
	EXPECT_FALSE(pidl2.HasValue());
	EXPECT_EQ(pidl2.Raw(), nullptr);
}

TEST(PidlAbsolute, CopyConstructor)
{
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	PidlAbsolute pidl(ownedPidl.get());

	PidlAbsolute pidl2(pidl);
	EXPECT_TRUE(pidl2.HasValue());
	EXPECT_NE(pidl2.Raw(), pidl.Raw());
	EXPECT_TRUE(ArePidlsEquivalent(pidl2.Raw(), pidl.Raw()));

	PidlAbsolute pidl3;
	PidlAbsolute pidl4(pidl3);
	EXPECT_FALSE(pidl4.HasValue());
	EXPECT_EQ(pidl4.Raw(), nullptr);
}

TEST(PidlAbsolute, CopyAssignment)
{
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	PidlAbsolute pidl(ownedPidl.get());

	PidlAbsolute pidl2;
	pidl2 = pidl;
	EXPECT_TRUE(pidl2.HasValue());
	EXPECT_NE(pidl2.Raw(), pidl.Raw());
	EXPECT_TRUE(ArePidlsEquivalent(pidl2.Raw(), pidl.Raw()));

	PidlAbsolute pidl3;
	pidl3 = ownedPidl.get();
	EXPECT_TRUE(pidl3.HasValue());
	EXPECT_NE(pidl3.Raw(), ownedPidl.get());
	EXPECT_TRUE(ArePidlsEquivalent(pidl3.Raw(), ownedPidl.get()));

	pidl3 = nullptr;
	EXPECT_FALSE(pidl3.HasValue());
	EXPECT_EQ(pidl3.Raw(), nullptr);
}

TEST(PidlAbsolute, MoveConstructor)
{
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	PidlAbsolute pidl(ownedPidl.get());
	auto rawPidl = pidl.Raw();

	PidlAbsolute pidl2(std::move(pidl));
	EXPECT_TRUE(pidl2.HasValue());
	EXPECT_EQ(pidl2.Raw(), rawPidl);
}

TEST(PidlAbsolute, MoveAssignment)
{
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	PidlAbsolute pidl(ownedPidl.get());
	auto rawPidl = pidl.Raw();

	PidlAbsolute pidl2;
	pidl2 = std::move(pidl);
	EXPECT_TRUE(pidl2.HasValue());
	EXPECT_EQ(pidl2.Raw(), rawPidl);
}

TEST(PidlAbsolute, OutParam)
{
	auto getIdList = [](const std::wstring &path, PIDLIST_ABSOLUTE *pidl)
	{
		*pidl = SHSimpleIDListFromPath(path.c_str());
	};

	PidlAbsolute pidl;
	getIdList(L"C:\\", PidlOutParam(pidl));
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	EXPECT_TRUE(pidl.HasValue());
	EXPECT_TRUE(ArePidlsEquivalent(pidl.Raw(), ownedPidl.get()));

	// When using PidlAbsolute as an output parameter, the original pidl should be replaced by the
	// output pidl.
	getIdList(L"D:\\", PidlOutParam(pidl));
	ownedPidl.reset(SHSimpleIDListFromPath(L"D:\\"));
	EXPECT_TRUE(pidl.HasValue());
	EXPECT_TRUE(ArePidlsEquivalent(pidl.Raw(), ownedPidl.get()));
}

TEST(PidlAbsolute, TakeOwnership)
{
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	PCIDLIST_ABSOLUTE rawPidl = ownedPidl.get();

	PidlAbsolute pidl(ownedPidl.release(), Pidl::takeOwnership);
	EXPECT_EQ(pidl.Raw(), rawPidl);
}

TEST(PidlAbsolute, Combine)
{
	auto fullPidl = CreateSimplePidlForTest(L"c:\\users\\public");
	PidlChild child(ILFindLastID(fullPidl.Raw()));

	auto parentPidl = CreateSimplePidlForTest(L"c:\\users");
	EXPECT_EQ(parentPidl + child.Raw(), fullPidl);
	EXPECT_EQ(parentPidl + child, fullPidl);
	EXPECT_EQ(parentPidl.Raw() + child, fullPidl);

	auto parentPidl2 = parentPidl;
	parentPidl2 += child.Raw();
	EXPECT_EQ(parentPidl2, fullPidl);

	auto parentPidl3 = parentPidl;
	parentPidl3 += child;
	EXPECT_EQ(parentPidl3, fullPidl);
}

TEST(PidlAbsolute, GetLastItem)
{
	auto pidl = CreateSimplePidlForTest(L"c:\\users\\public");
	auto originalPidl = pidl;

	auto lastItem = pidl.GetLastItem();
	ASSERT_TRUE(pidl.RemoveLastItem());

	EXPECT_EQ(pidl + lastItem, originalPidl);
}

TEST(PidlAbsolute, RemoveLastItem)
{
	auto pidl = CreateSimplePidlForTest(L"c:\\users\\public");
	EXPECT_TRUE(pidl.RemoveLastItem());
	EXPECT_EQ(pidl, CreateSimplePidlForTest(L"c:\\users"));
}

TEST(PidlAbsolute, Reset)
{
	unique_pidl_absolute ownedPidl(SHSimpleIDListFromPath(L"C:\\"));
	PidlAbsolute pidl(ownedPidl.get());

	pidl.Reset();
	EXPECT_FALSE(pidl.HasValue());
	EXPECT_EQ(pidl.Raw(), nullptr);
}

TEST(PidlAbsoluteEquality, Same)
{
	TestPidlEquality(L"c:\\", L"c:\\", true);
	TestPidlEquality(L"c:\\users\\public", L"c:\\users\\public", true);
}

TEST(PidlAbsoluteEquality, Different)
{
	TestPidlEquality(L"c:\\", L"c:\\windows", false);
	TestPidlEquality(L"c:\\", L"d:\\path\\to\\item", false);
}

TEST(PidlBase64Decoding, EmptyData)
{
	auto decodedPidl = DecodePidlFromBase64("");
	ASSERT_FALSE(decodedPidl.HasValue());
}

TEST(PidlBase64Decoding, InvalidData)
{
	// ';' isn't a valid character in the base64 alphabet.
	auto decodedPidl = DecodePidlFromBase64(";");
	ASSERT_FALSE(decodedPidl.HasValue());

	// The encoded data here isn't valid, since it's not long enough to represent a single byte.
	decodedPidl = DecodePidlFromBase64("a");
	ASSERT_FALSE(decodedPidl.HasValue());
}

class PidlBase64Encoding : public Test
{
protected:
	void PerformEncodeDecodeTest(const std::wstring &path)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto encodedPidl = EncodePidlToBase64(pidl.Raw());
		auto decodedPidl = DecodePidlFromBase64(encodedPidl);
		ASSERT_TRUE(decodedPidl.HasValue());
		EXPECT_EQ(decodedPidl, pidl);
	}
};

TEST_F(PidlBase64Encoding, EncodeDecode)
{
	PerformEncodeDecodeTest(LR"(c:\path)");
	PerformEncodeDecodeTest(LR"(c:\windows\system32)");
	PerformEncodeDecodeTest(LR"(d:\path\to\folder)");
}
