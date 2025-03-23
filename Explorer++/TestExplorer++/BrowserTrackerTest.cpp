// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTracker.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "GeneratorTestHelper.h"
#include "MainRebarStorage.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

// The only thing this class does is register itself with the provided BrowserList.
class BrowserWindowTrackerFake : public BrowserWindow
{
public:
	BrowserWindowTrackerFake(BrowserList *browserList) : m_browserTracker(browserList, this)
	{
	}

	HWND GetHWND() const override
	{
		return nullptr;
	}

	boost::signals2::connection AddBrowserInitializedObserver(
		const BrowserInitializedSignal::slot_type &observer) override
	{
		UNREFERENCED_PARAMETER(observer);

		return {};
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

	void CreateTabFromPreservedTab(const PreservedTab *tab) override
	{
		UNREFERENCED_PARAMETER(tab);
	}

	ShellBrowser *GetActiveShellBrowser() override
	{
		return nullptr;
	}

	const ShellBrowser *GetActiveShellBrowser() const
	{
		return nullptr;
	}

	WindowStorageData GetStorageData() const override
	{
		return { {}, WindowShowState::Normal };
	}

	bool IsActive() const override
	{
		return false;
	}

	void Activate() override
	{
	}

	void FocusChanged() override
	{
	}

	void TryClose() override
	{
	}

	void Close() override
	{
	}

	void OpenDefaultItem(OpenFolderDisposition openFolderDisposition) override
	{
		UNREFERENCED_PARAMETER(openFolderDisposition);
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

}

TEST(BrowserTrackerTest, AddRemove)
{
	BrowserList browserList;

	auto browser1 = std::make_unique<BrowserWindowTrackerFake>(&browserList);
	EXPECT_THAT(GeneratorToVector(browserList.GetList()), UnorderedElementsAre(browser1.get()));

	auto browser2 = std::make_unique<BrowserWindowTrackerFake>(&browserList);
	EXPECT_THAT(GeneratorToVector(browserList.GetList()),
		UnorderedElementsAre(browser1.get(), browser2.get()));

	browser1.reset();
	EXPECT_THAT(GeneratorToVector(browserList.GetList()), UnorderedElementsAre(browser2.get()));

	browser2.reset();
	EXPECT_THAT(GeneratorToVector(browserList.GetList()), IsEmpty());
}
