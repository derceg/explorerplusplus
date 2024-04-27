// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RegistryStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include <wil/registry.h>
#include <Shlwapi.h>
#include <shellapi.h>

void RegistryStorageTest::SetUp()
{
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		APPLICATION_TEST_KEY.c_str(), m_applicationTestKey, wil::reg::key_access::readwrite);
	ASSERT_HRESULT_SUCCEEDED(hr);
}

void RegistryStorageTest::TearDown()
{
	LSTATUS result = SHDeleteKey(HKEY_CURRENT_USER, APPLICATION_TEST_KEY.c_str());
	ASSERT_EQ(result, ERROR_SUCCESS);
}

void RegistryStorageTest::ImportRegistryResource(const std::wstring &filename)
{
	std::wstring command = L"/c reg import " + filename;
	auto resourcesPath = GetResourcesDirectoryPath();

	SHELLEXECUTEINFO shellExecuteInfo;
	shellExecuteInfo.cbSize = sizeof(shellExecuteInfo);
	shellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shellExecuteInfo.hwnd = nullptr;
	shellExecuteInfo.lpVerb = L"open";
	shellExecuteInfo.lpFile = L"cmd.exe";
	shellExecuteInfo.lpParameters = command.c_str();
	shellExecuteInfo.lpDirectory = resourcesPath.c_str();
	shellExecuteInfo.nShow = SW_HIDE;
	shellExecuteInfo.hInstApp = nullptr;
	BOOL result = ShellExecuteEx(&shellExecuteInfo);
	ASSERT_TRUE(result);

	WaitForSingleObject(shellExecuteInfo.hProcess, INFINITE);
	CloseHandle(shellExecuteInfo.hProcess);
}
