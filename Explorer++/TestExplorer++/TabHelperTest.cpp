// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/TabHelper.h"
#include "../Helper/Controls.h"
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <commctrl.h>

using namespace testing;

class TabHelperTest : public Test
{
protected:
	struct TabData
	{
		std::wstring text;
		int imageIndex;
		LPARAM lParam;
	};

	void SetUp() override
	{
		m_tabControl.reset(CreateTabControl(nullptr, WS_POPUP));
		ASSERT_NE(m_tabControl, nullptr);
	}

	void PerformGetItemTextTest(const TabData &tab)
	{
		int index = AppendTab(tab);
		EXPECT_EQ(TabHelper::GetItemText(m_tabControl.get(), index), tab.text.c_str());
	}

	void AppendTabs(std::span<const TabData> tabs)
	{
		for (const auto &tab : tabs)
		{
			AppendTab(tab);
		}
	}

	int AppendTab(const TabData &tab)
	{
		int index;
		AppendTab(tab, index);
		return index;
	}

	void CheckTabs(std::span<const TabData> tabs)
	{
		int numTabs = TabCtrl_GetItemCount(m_tabControl.get());
		ASSERT_EQ(static_cast<size_t>(numTabs), tabs.size());

		for (int i = 0; i < numTabs; i++)
		{
			CheckTab(i, tabs[i]);
		}
	}

	void CheckTab(int index, const TabData &tab)
	{
		TCITEM tcItem = {};
		tcItem.mask = TCIF_IMAGE | TCIF_PARAM;
		auto res = TabCtrl_GetItem(m_tabControl.get(), index, &tcItem);
		ASSERT_TRUE(res);

		auto text = TabHelper::GetItemText(m_tabControl.get(), index);

		EXPECT_EQ(tab.text, text);
		EXPECT_EQ(tab.imageIndex, tcItem.iImage);
		EXPECT_EQ(tab.lParam, tcItem.lParam);
	}

	std::vector<TabData> GetTestTabs()
	{
		return { { L"First tab", 100, 3 }, { L"Second tab", 7319, 14 }, { L"Third tab", 76, 1 } };
	}

	wil::unique_hwnd m_tabControl;

private:
	void AppendTab(const TabData &tab, int &insertedIndex)
	{
		int index = TabCtrl_GetItemCount(m_tabControl.get());

		TCITEM tcItem = {};
		tcItem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
		tcItem.pszText = const_cast<wchar_t *>(tab.text.c_str());
		tcItem.iImage = tab.imageIndex;
		tcItem.lParam = tab.lParam;
		insertedIndex = TabCtrl_InsertItem(m_tabControl.get(), index, &tcItem);
		ASSERT_EQ(insertedIndex, index);
	}
};

TEST_F(TabHelperTest, GetItemTextNone)
{
	TCITEM tcItem = {};
	int index = TabCtrl_InsertItem(m_tabControl.get(), 0, &tcItem);
	ASSERT_NE(index, -1);

	EXPECT_EQ(TabHelper::GetItemText(m_tabControl.get(), index), L"");
}

TEST_F(TabHelperTest, GetItemText)
{
	PerformGetItemTextTest({ L"Item text" });
	PerformGetItemTextTest({ std::wstring(1000, 'a') });
}

TEST_F(TabHelperTest, SetItemText)
{
	int index = AppendTab({ L"Initial text" });

	std::wstring updatedText = L"Updated text";
	TabHelper::SetItemText(m_tabControl.get(), index, updatedText);
	EXPECT_EQ(TabHelper::GetItemText(m_tabControl.get(), index), updatedText);
}

TEST_F(TabHelperTest, MoveItem)
{
	auto tabs = GetTestTabs();
	AppendTabs(tabs);

	TabHelper::MoveItem(m_tabControl.get(), 0, 2);
	auto firstTab = tabs[0];
	tabs.erase(tabs.begin());
	tabs.push_back(firstTab);

	CheckTabs(tabs);
}

TEST_F(TabHelperTest, SwapItems)
{
	auto tabs = GetTestTabs();
	AppendTabs(tabs);

	TabHelper::SwapItems(m_tabControl.get(), 0, 2);
	std::swap(tabs[0], tabs[2]);

	CheckTabs(tabs);
}
