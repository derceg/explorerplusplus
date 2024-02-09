// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

using namespace testing;

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

	PidlAbsolute pidl;
	pidl.TakeOwnership(ownedPidl.release());
	EXPECT_EQ(pidl.Raw(), rawPidl);
}
