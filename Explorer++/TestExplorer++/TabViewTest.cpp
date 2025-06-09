// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabView.h"
#include "Config.h"
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <CommCtrl.h>
#include <memory>
#include <string>
#include <vector>

using namespace testing;

namespace
{

class TestTabViewItem : public TabViewItem
{
public:
	TestTabViewItem(const std::wstring &title) : m_title(title)
	{
	}

	std::wstring GetText() const override
	{
		return m_title;
	}

	std::optional<int> GetIconIndex() const override
	{
		return std::nullopt;
	}

private:
	const std::wstring m_title;
};

class TestTabView : public TabView
{
public:
	static TestTabView *Create(HWND parent, const Config *config)
	{
		return new TestTabView(parent, config);
	}

private:
	TestTabView(HWND parent, const Config *config) : TabView(parent, WS_CHILD, config)
	{
	}
};

}

class TabViewTest : public Test
{
protected:
	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		m_view = TestTabView::Create(m_parentWindow.get(), &m_config);
	}

	std::vector<std::wstring> GetTabTitles() const
	{
		return { L"Tab 1", L"Tab 2", L"Tab 3" };
	}

	std::vector<TestTabViewItem *> AddTabs(const std::vector<std::wstring> &tabTitles)
	{
		std::vector<TestTabViewItem *> rawTabItems;

		for (size_t i = 0; i < tabTitles.size(); i++)
		{
			auto tabItem = std::make_unique<TestTabViewItem>(tabTitles[i]);
			rawTabItems.push_back(tabItem.get());
			m_view->AddTab(std::move(tabItem), static_cast<int>(i));
		}

		return rawTabItems;
	}

	void VerifyTabs(std::vector<TestTabViewItem *> rawTabItems)
	{
		for (size_t i = 0; i < rawTabItems.size(); i++)
		{
			EXPECT_EQ(m_view->GetTabAtIndex(static_cast<int>(i)), rawTabItems[i]);
		}
	}

	Config m_config;

	wil::unique_hwnd m_parentWindow;
	TestTabView *m_view = nullptr;
};

TEST_F(TabViewTest, AddTab)
{
	std::vector<std::wstring> tabTitles = GetTabTitles();
	auto rawTabItems = AddTabs(tabTitles);
	VerifyTabs(rawTabItems);
}

TEST_F(TabViewTest, RemoveTab)
{
	std::vector<std::wstring> tabTitles = GetTabTitles();
	auto rawTabItems = AddTabs(tabTitles);

	m_view->RemoveTab(1);
	rawTabItems.erase(rawTabItems.begin() + 1);
	VerifyTabs(rawTabItems);

	m_view->RemoveTab(0);
	rawTabItems.erase(rawTabItems.begin());
	VerifyTabs(rawTabItems);
}

TEST_F(TabViewTest, GetNumTabs)
{
	EXPECT_EQ(m_view->GetNumTabs(), 0);

	std::vector<std::wstring> tabTitles = GetTabTitles();
	AddTabs(tabTitles);
	EXPECT_EQ(m_view->GetNumTabs(), 3);

	m_view->RemoveTab(2);
	EXPECT_EQ(m_view->GetNumTabs(), 2);

	m_view->RemoveTab(0);
	EXPECT_EQ(m_view->GetNumTabs(), 1);

	m_view->RemoveTab(0);
	EXPECT_EQ(m_view->GetNumTabs(), 0);
}
