// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DrivesToolbar.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "Config.h"
#include "DriveEnumeratorFake.h"
#include "DriveModel.h"
#include "DriveWatcherFake.h"
#include "DrivesToolbarView.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>

using namespace testing;

class DrivesToolbarTest : public BrowserTestBase
{
protected:
	DrivesToolbarTest() :
		m_driveModel(std::make_unique<DriveEnumeratorFake>(
						 std::set<std::wstring>{ L"C:\\", L"F:\\", L"H:\\" }),
			&m_driveWatcher),
		m_browser(AddBrowser()),
		m_drivesToolbarView(DrivesToolbarView::Create(m_browser->GetHWND(), &m_config)),
		m_drivesToolbar(
			DrivesToolbar::Create(m_drivesToolbarView, &m_driveModel, m_browser, nullptr))
	{
	}

	void VerifyToolbarButtons()
	{
		const auto &buttons = m_drivesToolbarView->GetButtons();
		const auto &drives = m_driveModel.GetDrives();
		ASSERT_EQ(buttons.size(), drives.size());

		// TODO: This should use std::views::zip once C++23 support is available.
		for (const auto &[button, drive] : boost::combine(buttons, drives))
		{
			EXPECT_EQ(button->GetText(), drive.substr(0, 1));
		}
	}

	Config m_config;

	DriveWatcherFake m_driveWatcher;
	DriveModel m_driveModel;

	BrowserWindowFake *const m_browser;
	DrivesToolbarView *const m_drivesToolbarView;
	DrivesToolbar *const m_drivesToolbar;
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

	const auto &buttons = m_drivesToolbarView->GetButtons();
	const auto &drives = m_driveModel.GetDrives();
	ASSERT_EQ(buttons.size(), drives.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[button, drive] : boost::combine(buttons, drives))
	{
		button->OnClicked(MouseEvent{ { 0, 0 }, false, false });

		auto *currentEntry = tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
		ASSERT_NE(currentEntry, nullptr);
		EXPECT_EQ(currentEntry->GetPidl(), CreateSimplePidlForTest(drive));
	}
}
