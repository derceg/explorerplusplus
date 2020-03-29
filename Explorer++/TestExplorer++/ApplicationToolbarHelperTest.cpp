// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "ApplicationToolbarHelper.h"
#include <gtest/gtest.h>

using namespace ApplicationToolbarHelper;

TEST(ApplicationToolbarHelperTest, WithoutQuotes)
{
	ApplicationInfo applicationInfo = ParseCommandString(L"C:\\Windows\\System32\\notepad.exe");
	EXPECT_EQ(applicationInfo.application, L"C:\\Windows\\System32\\notepad.exe");
	EXPECT_TRUE(applicationInfo.parameters.empty());

	applicationInfo = ParseCommandString(L"C:\\Windows\\System32\\notepad.exe C:\\file.txt");
	EXPECT_EQ(applicationInfo.application, L"C:\\Windows\\System32\\notepad.exe");
	EXPECT_EQ(applicationInfo.parameters, L"C:\\file.txt");

	// Paths with spaces need to be quoted. Although the user might consider the results below
	// surprising, they're considered "correct" internally.
	applicationInfo = ParseCommandString(L"C:\\Program Files\\PowerShell\\7\\pwsh.exe");
	EXPECT_EQ(applicationInfo.application, L"C:\\Program");
	EXPECT_EQ(applicationInfo.parameters, L"Files\\PowerShell\\7\\pwsh.exe");
}

TEST(ApplicationToolbarHelperTest, WithQuotes)
{
	ApplicationInfo applicationInfo = ParseCommandString(L"\"C:\\Windows\\System32\\notepad.exe\"");
	EXPECT_EQ(applicationInfo.application, L"C:\\Windows\\System32\\notepad.exe");
	EXPECT_TRUE(applicationInfo.parameters.empty());

	applicationInfo = ParseCommandString(L"\"C:\\Windows\\System32\\notepad.exe\" C:\\file.txt");
	EXPECT_EQ(applicationInfo.application, L"C:\\Windows\\System32\\notepad.exe");
	EXPECT_EQ(applicationInfo.parameters, L"C:\\file.txt");

	applicationInfo = ParseCommandString(L"\"C:\\Program Files\\PowerShell\\7\\pwsh.exe\"");
	EXPECT_EQ(applicationInfo.application, L"C:\\Program Files\\PowerShell\\7\\pwsh.exe");
	EXPECT_TRUE(applicationInfo.parameters.empty());

	applicationInfo =
		ParseCommandString(L"\"C:\\Program Files\\PowerShell\\7\\pwsh.exe\" -WorkingDirectory ~");
	EXPECT_EQ(applicationInfo.application, L"C:\\Program Files\\PowerShell\\7\\pwsh.exe");
	EXPECT_EQ(applicationInfo.parameters, L"-WorkingDirectory ~");
}