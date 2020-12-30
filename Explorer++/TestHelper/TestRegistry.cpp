// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"

void CreateTestKey(PHKEY phKey)
{
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Explorer++Test", 0, NULL,
		REG_OPTION_VOLATILE, KEY_READ | KEY_WRITE, NULL, phKey, NULL);
	ASSERT_EQ(ERROR_SUCCESS, lRes);
}

TEST(String, SaveLoad)
{
	HKEY hKey;
	CreateTestKey(&hKey);

	TCHAR szKey[] = L"TestStringKey";
	TCHAR szValue[] = L"TestStringValue";
	LONG lRes = RegistrySettings::SaveString(hKey, szKey, szValue);
	ASSERT_EQ(ERROR_SUCCESS, lRes);

	TCHAR szOutputValue[32] = EMPTY_STRING;
	lRes = RegistrySettings::ReadString(hKey, szKey, szOutputValue, SIZEOF_ARRAY(szOutputValue));
	ASSERT_EQ(ERROR_SUCCESS, lRes);

	EXPECT_STREQ(szValue, szOutputValue);

	RegCloseKey(hKey);
}

TEST(Dword, SaveLoad)
{
	HKEY hKey;
	CreateTestKey(&hKey);

	TCHAR szKey[] = L"TestDwordKey";
	DWORD dwValue = 2479824753;
	LONG lRes = RegistrySettings::SaveDword(hKey, szKey, dwValue);
	ASSERT_EQ(ERROR_SUCCESS, lRes);

	DWORD dwOutputValue = 0;
	lRes = RegistrySettings::ReadDword(hKey, szKey, &dwOutputValue);
	ASSERT_EQ(ERROR_SUCCESS, lRes);

	EXPECT_EQ(dwValue, dwOutputValue);

	RegCloseKey(hKey);
}

TEST(StringList, SaveLoad)
{
	HKEY hKey;
	CreateTestKey(&hKey);

	TCHAR szBaseKey[] = L"TestStringListKey";
	std::list<std::wstring> strValues;
	strValues.push_back(L"First");
	strValues.push_back(L"Second");
	strValues.push_back(L"Third");
	strValues.push_back(L"Fourth");
	LONG lRes = RegistrySettings::SaveStringList(hKey, szBaseKey, strValues);
	ASSERT_EQ(ERROR_SUCCESS, lRes);

	std::list<std::wstring> strOutputValues;
	lRes = RegistrySettings::ReadStringList(hKey, szBaseKey, strOutputValues);
	ASSERT_EQ(ERROR_SUCCESS, lRes);

	EXPECT_EQ(strValues, strOutputValues);

	RegCloseKey(hKey);
}