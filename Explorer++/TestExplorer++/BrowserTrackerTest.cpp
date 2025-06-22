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

	BrowserCommandController *GetCommandController() override
	{
		return nullptr;
	}

	BrowserPane *GetActivePane() const override
	{
		return nullptr;
	}

	TabContainer *GetActiveTabContainer() override
	{
		return nullptr;
	}

	const TabContainer *GetActiveTabContainer() const override
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

	const ShellBrowser *GetActiveShellBrowser() const override
	{
		return nullptr;
	}

	void StartMainToolbarCustomization() override
	{
	}

	std::optional<std::wstring> RequestMenuHelpText(HMENU menu, UINT id) const override
	{
		UNREFERENCED_PARAMETER(menu);
		UNREFERENCED_PARAMETER(id);

		return std::nullopt;
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

	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override
	{
		UNREFERENCED_PARAMETER(observer);

		return {};
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
