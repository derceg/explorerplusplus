// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/RegistrySettings.h"
#include "RegistryStorageHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wil/resource.h>

using namespace RegistrySettings;
using namespace testing;

bool operator==(const FILETIME &first, const FILETIME &second)
{
	return first.dwLowDateTime == second.dwLowDateTime
		&& first.dwHighDateTime == second.dwHighDateTime;
}

class RegistrySettingsTest : public RegistryStorageTest
{
protected:
	void SetUp() override
	{
		DWORD disposition;
		LSTATUS result = RegCreateKeyEx(HKEY_CURRENT_USER, APPLICATION_TEST_KEY.c_str(), 0, nullptr,
			REG_OPTION_VOLATILE, KEY_READ | KEY_WRITE, nullptr, &m_testKey, &disposition);
		ASSERT_EQ(result, ERROR_SUCCESS);
		ASSERT_EQ(disposition, static_cast<DWORD>(REG_CREATED_NEW_KEY));
	}

	wil::unique_hkey m_testKey;
};

TEST_F(RegistrySettingsTest, StringData)
{
	std::wstring valueName = L"TestStringName";
	std::wstring value = L"TestStringValue";

	auto res = SaveString(m_testKey.get(), valueName, value);
	ASSERT_EQ(res, ERROR_SUCCESS);

	std::wstring loadedValue;
	res = ReadString(m_testKey.get(), valueName, loadedValue);
	ASSERT_EQ(res, ERROR_SUCCESS);

	EXPECT_EQ(value, loadedValue);
}

TEST_F(RegistrySettingsTest, DwordData)
{
	std::wstring valueName = L"TestDwordName";
	DWORD value = 2479824753;

	auto res = SaveDword(m_testKey.get(), valueName, value);
	ASSERT_EQ(res, ERROR_SUCCESS);

	DWORD loadedValue;
	res = ReadDword(m_testKey.get(), valueName, loadedValue);
	ASSERT_EQ(res, ERROR_SUCCESS);

	EXPECT_EQ(value, loadedValue);

	MockFunction<void(DWORD)> callbackMock;
	EXPECT_CALL(callbackMock, Call(value));
	ReadDword(m_testKey.get(), valueName, callbackMock.AsStdFunction());
}

TEST_F(RegistrySettingsTest, StringListData)
{
	std::wstring valueName = L"TestStringListName";
	std::list<std::wstring> strings = { L"First", L"Second", L"Third", L"Fourth" };

	auto res = SaveStringList(m_testKey.get(), valueName, strings);
	ASSERT_EQ(res, ERROR_SUCCESS);

	std::list<std::wstring> loadedStrings;
	res = ReadStringList(m_testKey.get(), valueName, loadedStrings);
	ASSERT_EQ(res, ERROR_SUCCESS);

	EXPECT_EQ(strings, loadedStrings);
}

TEST_F(RegistrySettingsTest, DateTimeData)
{
	std::wstring valueName = L"TestDateTimeName";
	FILETIME dateTime = { 1777974944, 30988186 };

	auto res = SaveDateTime(m_testKey.get(), valueName, dateTime);
	ASSERT_TRUE(res);

	FILETIME loadedDateTime;
	res = ReadDateTime(m_testKey.get(), valueName, loadedDateTime);
	ASSERT_TRUE(res);

	EXPECT_EQ(dateTime, loadedDateTime);
}
