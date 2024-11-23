// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TestHelper.h"
#include "../Helper/UniqueResources.h"
#include <gtest/gtest.h>
#include <gdiplus.h>

using namespace testing;

class ComEnvironment : public Environment
{
public:
	void SetUp() override
	{
		// Technically, an STA thread requires that messages be pumped, which isn't something that's
		// done here. That should be ok, as it shouldn't be needed for the few specific COM-related
		// methods that are called (e.g. SHCreateShellItemArrayFromIDLists).
		// It might be a problem if a test were to create a COM object on the main thread, marshal
		// that object to another thread, then invoke a method on it from that second thread. But
		// that's not something that's explicitly done and it's unlikely that any of the COM-related
		// methods that are being called are doing that either.
		// Ultimately, COM needs to be initialized for some of the methods called within the tests
		// to succeed.
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		ASSERT_HRESULT_SUCCEEDED(hr);
	}

	void TearDown() override
	{
		CoUninitialize();
	}
};

// A few tests rely on being able to use GDI+.
class GdiplusEnvironment : public Environment
{
public:
	GdiplusEnvironment() : m_uniqueGdiplusShutdown(CheckedGdiplusStartup())
	{
	}

private:
	unique_gdiplus_shutdown m_uniqueGdiplusShutdown;
};

// This listener will ensure that when as ASSERT_* fails in a subroutine, the entire test will fail.
// See https://google.github.io/googletest/advanced.html#asserting-on-subroutines-with-an-exception.
class ThrowListener : public EmptyTestEventListener
{
public:
	void OnTestPartResult(const TestPartResult &result) override
	{
		if (result.type() == TestPartResult::kFatalFailure)
		{
			throw AssertionException(result);
		}
	}
};

int wmain(int argc, wchar_t *argv[])
{
	SetIsInTest();

	AddGlobalTestEnvironment(new ComEnvironment);
	AddGlobalTestEnvironment(new GdiplusEnvironment);
	InitGoogleTest(&argc, argv);
	UnitTest::GetInstance()->listeners().Append(new ThrowListener);
	return RUN_ALL_TESTS();
}
