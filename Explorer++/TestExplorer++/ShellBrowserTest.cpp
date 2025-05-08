// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/ShellBrowser.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>

class ShellBrowserTest : public BrowserTestBase
{
protected:
	ShellBrowserTest() : m_browser(AddBrowser()), m_tab(m_browser->AddTab())
	{
	}

	void NavigateTabAndCheckDirectory(Tab *tab, const std::wstring &path)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
		EXPECT_EQ(tab->GetShellBrowser()->GetDirectory(), pidl);
	}

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
};

TEST_F(ShellBrowserTest, GetDirectory)
{
	NavigateTabAndCheckDirectory(m_tab, L"c:\\path\\to\\folder");
	NavigateTabAndCheckDirectory(m_tab, L"e:\\documents");
	NavigateTabAndCheckDirectory(m_tab, L"g:\\compressed.zip");
}
