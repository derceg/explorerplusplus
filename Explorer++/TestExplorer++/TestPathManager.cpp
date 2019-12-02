// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/ShellBrowser/PathManager.h"
#include "../Helper/ShellHelper.h"
#include <ShlObj.h>

TEST(PathManagerTest, TestInitial)
{
	PathManager pathManager;

	EXPECT_EQ(pathManager.GetNumBackEntriesStored(), 0);
	EXPECT_EQ(pathManager.GetNumForwardEntriesStored(), 0);

	auto backHistory = pathManager.GetBackHistory();
	EXPECT_TRUE(backHistory.empty());

	auto forwardHistory = pathManager.GetForwardHistory();
	EXPECT_TRUE(forwardHistory.empty());

	EXPECT_FALSE(pathManager.GetEntry(0));
	EXPECT_FALSE(pathManager.GetEntry(-1));
	EXPECT_FALSE(pathManager.GetEntry(1));
}

TEST(PathManagerTest, TestAdd)
{
	PathManager pathManager;

	unique_pidl_absolute pidl;
	SHParseDisplayName(L"C:\\", nullptr, wil::out_param(pidl), 0, nullptr);
	pathManager.AddEntry(HistoryEntry(pidl.get(), L"C:\\"));

	auto backHistory = pathManager.GetBackHistory();
	EXPECT_TRUE(backHistory.empty());

	auto forwardHistory = pathManager.GetForwardHistory();
	EXPECT_TRUE(forwardHistory.empty());
}

TEST(PathManagerTest, TestAddMultiple)
{
	PathManager pathManager;

	unique_pidl_absolute pidl;
	SHParseDisplayName(L"C:\\", nullptr, wil::out_param(pidl), 0, nullptr);
	pathManager.AddEntry(HistoryEntry(pidl.get(), L"C:\\"));

	SHParseDisplayName(L"C:\\Windows", nullptr, wil::out_param(pidl), 0, nullptr);
	pathManager.AddEntry(HistoryEntry(pidl.get(), L"C:\\Windows"));

	SHParseDisplayName(L"C:\\Windows\\System32", nullptr, wil::out_param(pidl), 0, nullptr);
	pathManager.AddEntry(HistoryEntry(pidl.get(), L"C:\\Windows\\System32"));

	auto backHistory = pathManager.GetBackHistory();
	EXPECT_EQ(backHistory.size(), 2);

	auto forwardHistory = pathManager.GetForwardHistory();
	EXPECT_TRUE(forwardHistory.empty());

	// Go back by one entry.
	pathManager.GetEntry(-1);

	backHistory = pathManager.GetBackHistory();
	EXPECT_EQ(backHistory.size(), 1);

	forwardHistory = pathManager.GetForwardHistory();
	EXPECT_EQ(forwardHistory.size(), 1);

	SHParseDisplayName(L"C:\\Users", nullptr, wil::out_param(pidl), 0, nullptr);
	pathManager.AddEntry(HistoryEntry(pidl.get(), L"C:\\Users"));

	backHistory = pathManager.GetBackHistory();
	EXPECT_EQ(backHistory.size(), 2);

	forwardHistory = pathManager.GetForwardHistory();
	EXPECT_TRUE(forwardHistory.empty());

	// Go back two entries.
	pathManager.GetEntry(-2);

	backHistory = pathManager.GetBackHistory();
	EXPECT_TRUE(backHistory.empty());

	forwardHistory = pathManager.GetForwardHistory();
	EXPECT_EQ(forwardHistory.size(), 2);

	// Go forward two entries.
	pathManager.GetEntry(2);

	backHistory = pathManager.GetBackHistory();
	EXPECT_EQ(backHistory.size(), 2);

	forwardHistory = pathManager.GetForwardHistory();
	EXPECT_TRUE(forwardHistory.empty());
}