// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellEnumeratorImpl.h"
#include "ResourceTestHelper.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>
#include <wil/com.h>

using namespace testing;

class ShellEnumeratorImplTestBase : public Test
{
protected:
	ShellEnumeratorImplTestBase(ShellEnumeratorImpl::HiddenItemsPolicy hiddenItemsPolicy) :
		m_shellEnumerator(nullptr, hiddenItemsPolicy)
	{
	}

	void SetUp() override
	{
		SetTestFileHidden(true);
	}

	void TearDown() override
	{
		// Removing the hidden attribute isn't necessary for any of the tests, but if the test file
		// has the hidden attribute set when building, the xcopy post-build event will fail to
		// overwrite it (specifically, xcopy will fail with an "Access denied" error). That's the
		// reason the attribute is removed here.
		SetTestFileHidden(false);
	}

	void CheckEnumeration(const std::vector<std::wstring> &expectedItems)
	{
		PidlAbsolute pidl;
		std::wstring testDirectory = GetEnumerationTestDirectory();
		ASSERT_HRESULT_SUCCEEDED(
			SHParseDisplayName(testDirectory.c_str(), nullptr, PidlOutParam(pidl), 0, nullptr));

		std::vector<PidlChild> items;
		ASSERT_HRESULT_SUCCEEDED(m_shellEnumerator.EnumerateDirectory(pidl.Raw(), items));

		wil::com_ptr_nothrow<IShellFolder> parent;
		ASSERT_HRESULT_SUCCEEDED(
			SHBindToObject(nullptr, pidl.Raw(), nullptr, IID_PPV_ARGS(&parent)));

		std::vector<std::wstring> itemNames;

		for (const auto &item : items)
		{
			std::wstring name;
			ASSERT_HRESULT_SUCCEEDED(GetDisplayName(parent.get(), item.Raw(), SHGDN_NORMAL, name));

			itemNames.push_back(name);
		}

		EXPECT_THAT(itemNames, UnorderedElementsAreArray(expectedItems));
	}

private:
	std::filesystem::path GetEnumerationTestDirectory()
	{
		return GetResourcePath(L"EnumerationTestDirectory");
	}

	void SetTestFileHidden(bool set)
	{
		auto testDirectory = GetEnumerationTestDirectory();
		auto hiddenItemPath = testDirectory / L"hidden.txt";
		auto attributes = GetFileAttributes(hiddenItemPath.c_str());
		ASSERT_NE(attributes, INVALID_FILE_ATTRIBUTES);

		if (set)
		{
			WI_SetFlag(attributes, FILE_ATTRIBUTE_HIDDEN);
		}
		else
		{
			WI_ClearFlag(attributes, FILE_ATTRIBUTE_HIDDEN);
		}

		auto res = SetFileAttributes(hiddenItemPath.c_str(), attributes);
		ASSERT_NE(res, 0);
	}

	ShellEnumeratorImpl m_shellEnumerator;
};

class ShellEnumeratorImplExcludeHiddenTest : public ShellEnumeratorImplTestBase
{
protected:
	ShellEnumeratorImplExcludeHiddenTest() :
		ShellEnumeratorImplTestBase(ShellEnumeratorImpl::HiddenItemsPolicy::ExcludeHidden)
	{
	}
};

TEST_F(ShellEnumeratorImplExcludeHiddenTest, EnumerateTestDirectory)
{
	CheckEnumeration({ L"item1.txt", L"item2.txt", L"item3.txt" });
}

class ShellEnumeratorImplIncludeHiddenTest : public ShellEnumeratorImplTestBase
{
protected:
	ShellEnumeratorImplIncludeHiddenTest() :
		ShellEnumeratorImplTestBase(ShellEnumeratorImpl::HiddenItemsPolicy::IncludeHidden)
	{
	}
};

TEST_F(ShellEnumeratorImplIncludeHiddenTest, EnumerateTestDirectory)
{
	CheckEnumeration({ L"item1.txt", L"item2.txt", L"item3.txt", L"hidden.txt" });
}
