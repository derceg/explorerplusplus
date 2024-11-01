// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTracker.h"
#include "BrowserList.h"
#include "BrowserListTestHelper.h"
#include "BrowserWindow.h"
#include <gtest/gtest.h>

using namespace testing;

// The only thing this class does is register itself with the provided BrowserList.
class BrowserWindowFake : public BrowserWindow
{
public:
	BrowserWindowFake(BrowserList *browserList) : m_browserTracker(browserList, this)
	{
	}

	BrowserCommandController *GetCommandController() override
	{
		return nullptr;
	}

	BrowserPane *GetActivePane() const override
	{
		return nullptr;
	}

	void FocusActiveTab() override
	{
	}

	ShellBrowser *GetActiveShellBrowser() override
	{
		return nullptr;
	}

	HWND GetHWND() const override
	{
		return nullptr;
	}

	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition) override
	{
		UNREFERENCED_PARAMETER(itemPath);
		UNREFERENCED_PARAMETER(openFolderDisposition);
	}

	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition) override
	{
		UNREFERENCED_PARAMETER(pidlItem);
		UNREFERENCED_PARAMETER(openFolderDisposition);
	}

private:
	BrowserTracker m_browserTracker;
};

TEST(BrowserTrackerTest, AddRemove)
{
	BrowserList browserList;

	auto browser1 = std::make_unique<BrowserWindowFake>(&browserList);
	EXPECT_THAT(GetBrowserListAsVector(&browserList), UnorderedElementsAre(browser1.get()));

	auto browser2 = std::make_unique<BrowserWindowFake>(&browserList);
	EXPECT_THAT(GetBrowserListAsVector(&browserList),
		UnorderedElementsAre(browser1.get(), browser2.get()));

	browser1.reset();
	EXPECT_THAT(GetBrowserListAsVector(&browserList), UnorderedElementsAre(browser2.get()));

	browser2.reset();
	EXPECT_THAT(GetBrowserListAsVector(&browserList), IsEmpty());
}
