// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TaskbarTabManager.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "../Helper/WinRTBaseWrapper.h"
#include "../Helper/WindowHelper.h"
#include <gtest/gtest.h>
#include <wil/com.h>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace testing;

namespace
{

class TaskbarListFake : public winrt::implements<TaskbarListFake, ITaskbarList4, winrt::non_agile>
{
public:
	// ITaskbarList4
	IFACEMETHODIMP SetTabProperties(HWND hwndTab, STPFLAG stpFlags)
	{
		UNREFERENCED_PARAMETER(hwndTab);
		UNREFERENCED_PARAMETER(stpFlags);

		return E_NOTIMPL;
	}

	// ITaskbarList3
	IFACEMETHODIMP SetProgressValue(HWND hwnd, ULONGLONG ullCompleted, ULONGLONG ullTotal)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(ullCompleted);
		UNREFERENCED_PARAMETER(ullTotal);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP SetProgressState(HWND hwnd, TBPFLAG tbpFlags)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(tbpFlags);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP RegisterTab(HWND hwndTab, HWND hwndMDI)
	{
		auto &topLevelWindow = m_topLevelWindows[hwndMDI];
		topLevelWindow.hwnd = hwndMDI;
		CHECK(std::ranges::find(topLevelWindow.tabs, hwndTab) == topLevelWindow.tabs.end());
		topLevelWindow.tabs.push_back(hwndTab);

		return S_OK;
	}

	IFACEMETHODIMP UnregisterTab(HWND hwndTab)
	{
		auto *topLevelWindow = GetTopLevelWindowForTab(hwndTab);

		auto itr = std::ranges::find(topLevelWindow->tabs, hwndTab);
		CHECK(itr != topLevelWindow->tabs.end());

		topLevelWindow->tabs.erase(itr);

		if (topLevelWindow->activeTab == hwndTab)
		{
			topLevelWindow->activeTab = nullptr;
		}

		if (topLevelWindow->tabs.empty())
		{
			m_topLevelWindows.erase(topLevelWindow->hwnd);
		}

		return S_OK;
	}

	IFACEMETHODIMP SetTabOrder(HWND hwndTab, HWND hwndInsertBefore)
	{
		auto *topLevelWindow = GetTopLevelWindowForTab(hwndTab);

		auto itr = std::ranges::find(topLevelWindow->tabs, hwndTab);
		CHECK(itr != topLevelWindow->tabs.end());
		topLevelWindow->tabs.erase(itr);

		if (hwndInsertBefore)
		{
			auto insertItr = std::ranges::find(topLevelWindow->tabs, hwndInsertBefore);
			CHECK(insertItr != topLevelWindow->tabs.end());
			topLevelWindow->tabs.insert(insertItr, hwndTab);
		}
		else
		{
			topLevelWindow->tabs.push_back(hwndTab);
		}

		return S_OK;
	}

	IFACEMETHODIMP SetTabActive(HWND hwndTab, HWND hwndMDI, DWORD dwReserved)
	{
		UNREFERENCED_PARAMETER(dwReserved);

		auto itr = m_topLevelWindows.find(hwndMDI);
		CHECK(itr != m_topLevelWindows.end());

		CHECK(std::ranges::find(itr->second.tabs, hwndTab) != itr->second.tabs.end());
		itr->second.activeTab = hwndTab;

		return S_OK;
	}

	IFACEMETHODIMP ThumbBarAddButtons(HWND hwnd, UINT cButtons, LPTHUMBBUTTON pButton)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(cButtons);
		UNREFERENCED_PARAMETER(pButton);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP ThumbBarUpdateButtons(HWND hwnd, UINT cButtons, LPTHUMBBUTTON pButton)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(cButtons);
		UNREFERENCED_PARAMETER(pButton);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP ThumbBarSetImageList(HWND hwnd, HIMAGELIST himl)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(himl);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP SetOverlayIcon(HWND hwnd, HICON hIcon, LPCWSTR pszDescription)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(hIcon);
		UNREFERENCED_PARAMETER(pszDescription);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP SetThumbnailTooltip(HWND hwnd, LPCWSTR pszTip)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(pszTip);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP SetThumbnailClip(HWND hwnd, RECT *prcClip)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(prcClip);

		return E_NOTIMPL;
	}

	// ITaskbarList2
	IFACEMETHODIMP MarkFullscreenWindow(HWND hwnd, BOOL fFullscreen)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(fFullscreen);

		return E_NOTIMPL;
	}

	// ITaskbarList
	IFACEMETHODIMP HrInit()
	{
		return S_OK;
	}

	IFACEMETHODIMP AddTab(HWND hwnd)
	{
		UNREFERENCED_PARAMETER(hwnd);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP DeleteTab(HWND hwnd)
	{
		UNREFERENCED_PARAMETER(hwnd);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP ActivateTab(HWND hwnd)
	{
		UNREFERENCED_PARAMETER(hwnd);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP SetActiveAlt(HWND hwnd)
	{
		UNREFERENCED_PARAMETER(hwnd);

		return E_NOTIMPL;
	}

	bool AreAnyTabsRegisteredForWindow(HWND hwndMDI) const
	{
		return m_topLevelWindows.contains(hwndMDI);
	}

	const std::vector<HWND> &GetTabsForWindow(HWND hwndMDI) const
	{
		auto itr = m_topLevelWindows.find(hwndMDI);
		CHECK(itr != m_topLevelWindows.end());
		return itr->second.tabs;
	}

	HWND GetActiveTabForWindow(HWND hwndMDI) const
	{
		auto itr = m_topLevelWindows.find(hwndMDI);
		CHECK(itr != m_topLevelWindows.end());
		return itr->second.activeTab;
	}

private:
	struct TopLevelWindow
	{
		HWND hwnd;
		std::vector<HWND> tabs;
		HWND activeTab = nullptr;
	};

	TopLevelWindow *GetTopLevelWindowForTab(HWND hwndTab)
	{
		auto itr = std::ranges::find_if(m_topLevelWindows, [hwndTab](const auto &entry)
			{ return std::ranges::find(entry.second.tabs, hwndTab) != entry.second.tabs.end(); });
		CHECK(itr != m_topLevelWindows.end());
		return &itr->second;
	}

	std::unordered_map<HWND, TopLevelWindow> m_topLevelWindows;
};

}

class TaskbarTabManagerTest : public BrowserTestBase
{
protected:
	enum class AutoSendTaskbarButtonCreated
	{
		Yes,
		No
	};

	TaskbarTabManagerTest(AutoSendTaskbarButtonCreated autoSendTaskbarButtonCreated =
							  AutoSendTaskbarButtonCreated::Yes) :
		m_taskbarButtonCreatedMessage(RegisterWindowMessage(L"TaskbarButtonCreated")),
		m_browser(AddBrowser()),
		m_taskbarList(winrt::make_self<TaskbarListFake>().get()),
		m_taskbarTabManager(std::make_unique<TaskbarTabManager>(m_browser, &m_appServices,
			[this]() { return m_taskbarList; }))
	{
		m_config.showTaskbarThumbnails = true;

		if (autoSendTaskbarButtonCreated == AutoSendTaskbarButtonCreated::Yes)
		{
			SendTaskbarButtonCreatedMessage();
		}
	}

	void SendTaskbarButtonCreatedMessage()
	{
		SendMessage(m_browser->GetHWND(), m_taskbarButtonCreatedMessage, 0, 0);
	}

	std::vector<const Tab *> GetTabsForProxyWindows() const
	{
		std::vector<const Tab *> tabs;

		for (HWND proxy : m_taskbarList->GetTabsForWindow(m_browser->GetHWND()))
		{
			tabs.push_back(m_taskbarTabManager->GetTabForProxyWindowForTesting(proxy));
		}

		return tabs;
	}

	void VerifyProxyWindowTitles()
	{
		for (HWND proxy : m_taskbarList->GetTabsForWindow(m_browser->GetHWND()))
		{
			auto *tab = m_taskbarTabManager->GetTabForProxyWindowForTesting(proxy);
			EXPECT_EQ(GetWindowString(proxy), tab->GetName());
		}
	}

	UINT m_taskbarButtonCreatedMessage;

	BrowserWindowFake *const m_browser;

	wil::com_ptr_nothrow<TaskbarListFake> m_taskbarList;
	std::unique_ptr<TaskbarTabManager> m_taskbarTabManager;
};

TEST_F(TaskbarTabManagerTest, TabCreation)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	auto *tabContainer = m_browser->GetActiveTabContainer();
	EXPECT_THAT(GetTabsForProxyWindows(), Pointwise(Eq(), tabContainer->GetAllTabsInOrder()));
}

TEST_F(TaskbarTabManagerTest, TabRemoval)
{
	m_browser->AddTab(L"c:\\");
	auto *tab2 = m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	auto *tabContainer = m_browser->GetActiveTabContainer();
	tabContainer->CloseTab(*tab2);
	EXPECT_THAT(GetTabsForProxyWindows(), Pointwise(Eq(), tabContainer->GetAllTabsInOrder()));
}

TEST_F(TaskbarTabManagerTest, TabActivation)
{
	m_browser->AddTab(L"c:\\");
	auto *tab2 = m_browser->AddTab(L"d:\\");
	auto *tab3 = m_browser->AddTab(L"e:\\");

	auto *tabContainer = m_browser->GetActiveTabContainer();
	tabContainer->SelectTab(*tab2);
	auto *activeTab = m_taskbarTabManager->GetTabForProxyWindowForTesting(
		m_taskbarList->GetActiveTabForWindow(m_browser->GetHWND()));
	EXPECT_EQ(activeTab, tab2);

	tabContainer->SelectTab(*tab3);
	activeTab = m_taskbarTabManager->GetTabForProxyWindowForTesting(
		m_taskbarList->GetActiveTabForWindow(m_browser->GetHWND()));
	EXPECT_EQ(activeTab, tab3);
}

TEST_F(TaskbarTabManagerTest, TabMove)
{
	auto *tab1 = m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	auto *tab3 = m_browser->AddTab(L"e:\\");

	auto *tabContainer = m_browser->GetActiveTabContainer();
	tabContainer->MoveTab(*tab1, 2);
	EXPECT_THAT(GetTabsForProxyWindows(), Pointwise(Eq(), tabContainer->GetAllTabsInOrder()));

	tabContainer->MoveTab(*tab3, 0);
	EXPECT_THAT(GetTabsForProxyWindows(), Pointwise(Eq(), tabContainer->GetAllTabsInOrder()));
}

TEST_F(TaskbarTabManagerTest, ProxyWindowTitle)
{
	m_browser->AddTab(L"c:\\");
	auto *tab2 = m_browser->AddTab(L"d:\\");
	auto *tab3 = m_browser->AddTab(L"e:\\");

	VerifyProxyWindowTitles();

	NavigateTab(tab2, L"f:\\");
	VerifyProxyWindowTitles();

	tab3->SetCustomName(L"Custom name");
	VerifyProxyWindowTitles();
}

TEST_F(TaskbarTabManagerTest, ActivationHandling)
{
	m_browser->AddTab(L"c:\\");
	auto *tab2 = m_browser->AddTab(L"d:\\");

	const auto &proxyWindows = m_taskbarList->GetTabsForWindow(m_browser->GetHWND());
	ASSERT_EQ(proxyWindows.size(), 2u);

	SendMessage(proxyWindows[1], WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);
	auto *tabContainer = m_browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->IsTabSelected(*tab2));
}

TEST_F(TaskbarTabManagerTest, CloseTabHandling)
{
	auto *tab1 = m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	auto *tab3 = m_browser->AddTab(L"e:\\");

	const auto &proxyWindows = m_taskbarList->GetTabsForWindow(m_browser->GetHWND());
	ASSERT_EQ(proxyWindows.size(), 3u);

	SendMessage(proxyWindows[1], WM_CLOSE, 0, 0);
	EXPECT_THAT(m_browser->GetActiveTabContainer()->GetAllTabsInOrder(), ElementsAre(tab1, tab3));
}

TEST_F(TaskbarTabManagerTest, TabsReAddedAfterReEnabling)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	m_config.showTaskbarThumbnails = false;
	EXPECT_FALSE(m_taskbarList->AreAnyTabsRegisteredForWindow(m_browser->GetHWND()));

	m_config.showTaskbarThumbnails = true;
	auto *tabContainer = m_browser->GetActiveTabContainer();
	EXPECT_THAT(GetTabsForProxyWindows(), Pointwise(Eq(), tabContainer->GetAllTabsInOrder()));
}

TEST_F(TaskbarTabManagerTest, TabsClearedOnDestruction)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	m_taskbarTabManager.reset();
	EXPECT_FALSE(m_taskbarList->AreAnyTabsRegisteredForWindow(m_browser->GetHWND()));
}

TEST_F(TaskbarTabManagerTest, NoTabsRegisteredWhenDisabled)
{
	m_config.showTaskbarThumbnails = false;

	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");
	EXPECT_FALSE(m_taskbarList->AreAnyTabsRegisteredForWindow(m_browser->GetHWND()));
}

class TaskbarTabManagerDelayedTaskbarButtonTest : public TaskbarTabManagerTest
{
protected:
	TaskbarTabManagerDelayedTaskbarButtonTest() :
		TaskbarTabManagerTest(AutoSendTaskbarButtonCreated::No)
	{
	}
};

TEST_F(TaskbarTabManagerDelayedTaskbarButtonTest, TabsCreatedBeforeTaskbarButton)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");
	EXPECT_FALSE(m_taskbarList->AreAnyTabsRegisteredForWindow(m_browser->GetHWND()));

	SendTaskbarButtonCreatedMessage();
	auto *tabContainer = m_browser->GetActiveTabContainer();
	EXPECT_THAT(GetTabsForProxyWindows(), Pointwise(Eq(), tabContainer->GetAllTabsInOrder()));
}
