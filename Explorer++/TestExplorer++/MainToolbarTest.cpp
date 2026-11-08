// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainToolbar.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "CoreInterface.h"
#include "MainToolbarView.h"
#include "ShellIconLoaderFake.h"

namespace
{

class CoreInterfaceFake : public CoreInterface
{
public:
	ShellBrowserImpl *GetActiveShellBrowserImpl() const override
	{
		return nullptr;
	}

	TabBar *GetTabBar() const override
	{
		return nullptr;
	}

	BOOL CanPaste(PasteType pasteType) const override
	{
		UNREFERENCED_PARAMETER(pasteType);

		return FALSE;
	}
};

}

class MainToolbarTest : public BrowserTestBase
{
protected:
	MainToolbarTest() : m_browser(AddBrowser())
	{
	}

	BrowserWindowFake *const m_browser;
	ShellIconLoaderFake m_shellIconLoader;
	CoreInterfaceFake m_coreInterface;
};

TEST_F(MainToolbarTest, GetLayoutForStorage)
{
	MainToolbarLayout initialLayout({
		MainToolbarButtonId::Bookmarks,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Search,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Back,
		MainToolbarButtonId::Forward,
		MainToolbarButtonId::Up,
		MainToolbarButtonId::Separator,
		MainToolbarButtonId::Properties,
		MainToolbarButtonId::Refresh,
	});

	MainToolbarView mainToolbarView(m_browser->GetView(), m_browser, &m_config);
	MainToolbar mainToolbar(&mainToolbarView, initialLayout, m_browser, &m_appServices,
		&m_coreInterface, &m_shellIconLoader);

	auto layout = mainToolbar.GetLayoutForStorage();
	EXPECT_EQ(layout.GetButtons(), initialLayout.GetButtons());
}
