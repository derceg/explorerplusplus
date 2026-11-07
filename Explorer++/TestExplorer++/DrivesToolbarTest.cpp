// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DrivesToolbar.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "DrivesToolbarView.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

class DrivesToolbarTest : public BrowserTestBase
{
protected:
	DrivesToolbarTest() :
		m_browser(AddBrowser()),
		m_drivesToolbarView(m_browser->GetHWND(), m_browser, &m_config)
	{
		m_driveWatcher.AddDrive(L"C:\\");
		m_driveWatcher.AddDrive(L"F:\\");
		m_driveWatcher.AddDrive(L"H:\\");

		m_drivesToolbar = std::make_unique<DrivesToolbar>(&m_drivesToolbarView, &m_driveModel,
			m_browser, &m_acceleratorManager, &m_resourceLoader);
	}

	void VerifyToolbarButtons()
	{
		auto toolbarItems = m_drivesToolbarView.GetItems();
		const auto &drives = m_driveModel.GetDrives();
		ASSERT_EQ(toolbarItems.size(), drives.size());

		// TODO: This should use std::views::zip once C++23 support is available.
		for (const auto &[toolbarItem, drive] : boost::combine(toolbarItems, drives))
		{
			ASSERT_TRUE(toolbarItem.IsButton());

			auto *button = toolbarItem.GetButton();
			EXPECT_EQ(button->GetText(), drive.substr(0, 1));
		}
	}

	BrowserWindowFake *const m_browser;
	DrivesToolbarView m_drivesToolbarView;
	std::unique_ptr<DrivesToolbar> m_drivesToolbar;
};

TEST_F(DrivesToolbarTest, InitialDrives)
{
	VerifyToolbarButtons();
}

TEST_F(DrivesToolbarTest, AddDrives)
{
	m_driveWatcher.AddDrive(L"B:\\");
	m_driveWatcher.AddDrive(L"D:\\");
	m_driveWatcher.AddDrive(L"T:\\");

	VerifyToolbarButtons();
}

TEST_F(DrivesToolbarTest, RemoveDrives)
{
	m_driveWatcher.RemoveDrive(L"F:\\");
	m_driveWatcher.RemoveDrive(L"H:\\");

	VerifyToolbarButtons();
}

TEST_F(DrivesToolbarTest, OpenOnClick)
{
	auto *tab = m_browser->AddTab(L"c:\\original\\path");

	auto toolbarItems = m_drivesToolbarView.GetItems();
	const auto &drives = m_driveModel.GetDrives();
	ASSERT_EQ(toolbarItems.size(), drives.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[toolbarItem, drive] : boost::combine(toolbarItems, drives))
	{
		ASSERT_TRUE(toolbarItem.IsButton());

		auto *button = toolbarItem.GetButton();
		button->OnClicked(MouseEvent{ { 0, 0 }, false, false });
		EXPECT_EQ(tab->GetShellBrowser()->GetDirectory(), CreateSimplePidlForTest(drive));
	}
}

TEST_F(DrivesToolbarTest, OpenOnMiddleClick)
{
	auto toolbarItems = m_drivesToolbarView.GetItems();
	const auto &drives = m_driveModel.GetDrives();
	ASSERT_EQ(toolbarItems.size(), drives.size());

	auto *tabBar = m_browser->GetActiveTabBar();
	int numTabs = tabBar->GetNumTabs();

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[toolbarItem, drive] : boost::combine(toolbarItems, drives))
	{
		ASSERT_TRUE(toolbarItem.IsButton());

		auto *button = toolbarItem.GetButton();
		button->OnMiddleClicked(MouseEvent{ { 0, 0 }, false, false });

		numTabs++;
		ASSERT_EQ(tabBar->GetNumTabs(), numTabs);
		EXPECT_EQ(tabBar->GetTabByIndex(numTabs - 1).GetShellBrowser()->GetDirectory(),
			CreateSimplePidlForTest(drive));
	}
}
