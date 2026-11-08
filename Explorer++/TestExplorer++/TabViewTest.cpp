// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabView.h"
#include "Config.h"
#include "ControlTestHelper.h"
#include "DarkModeManagerFake.h"
#include "ImageTestHelper.h"
#include "TabViewTestApi.h"
#include "TestParentView.h"
#include "ThemeManager.h"
#include "Win32ResourceLoader.h"
#include "../Helper/GdiplusHelper.h"
#include "../Helper/ImageHelper.h"
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <CommCtrl.h>
#include <gdiplus.h>
#include <memory>
#include <string>
#include <vector>

using namespace testing;

namespace
{

class TestTabViewItem : public TabViewItem
{
public:
	TestTabViewItem(const std::wstring &title, const std::optional<int> &iconIndex = std::nullopt) :
		m_title(title),
		m_iconIndex(iconIndex)
	{
	}

	std::wstring GetText() const override
	{
		return m_title;
	}

	std::wstring GetTooltipText() const override
	{
		return L"";
	}

	std::optional<int> GetIconIndex() const override
	{
		return m_iconIndex;
	}

private:
	const std::wstring m_title;
	const std::optional<int> m_iconIndex;
};

}

class TabViewTest : public Test
{
protected:
	TabViewTest() :
		m_themeManager(&m_darkModeManager),
		m_resourceLoader(GetModuleHandle(nullptr), IconSet::Color, nullptr, nullptr),
		m_view(&m_parentView, WS_CHILD | TCS_FOCUSNEVER | TCS_SINGLELINE, &m_themeManager,
			&m_config, &m_resourceLoader),
		m_testApi(&m_view)
	{
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
			m_view.AddTab(std::move(tabItem), static_cast<int>(i));
		}

		return rawTabItems;
	}

	void VerifyTabs(std::vector<TestTabViewItem *> rawTabItems)
	{
		ASSERT_EQ(static_cast<size_t>(m_view.GetNumTabs()), rawTabItems.size());

		for (size_t i = 0; i < rawTabItems.size(); i++)
		{
			EXPECT_EQ(m_view.GetTabAtIndex(static_cast<int>(i)), rawTabItems[i]);
		}
	}

	void ResizeWindowToFitTabs()
	{
		RECT displayRect = { 0, 0, 0, 0 };
		TabCtrl_AdjustRect(m_view.GetHWND(), true, &displayRect);
		int height = std::abs(displayRect.top);

		int numTabs = m_view.GetNumTabs();
		ASSERT_NE(numTabs, 0);

		auto lastTabRect = m_testApi.GetTabRect(numTabs - 1);

		auto res = MoveWindow(m_parentView.GetHWND(), 0, 0, lastTabRect.right, height, false);
		ASSERT_TRUE(res);

		res = MoveWindow(m_view.GetHWND(), 0, 0, lastTabRect.right, height, false);
		ASSERT_TRUE(res);
	}

	Config m_config;
	DarkModeManagerFake m_darkModeManager;
	ThemeManager m_themeManager;
	Win32ResourceLoader m_resourceLoader;

	TestParentView m_parentView;
	TabView m_view;
	TabViewTestApi m_testApi;
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

	m_view.RemoveTab(1);
	rawTabItems.erase(rawTabItems.begin() + 1);
	VerifyTabs(rawTabItems);

	m_view.RemoveTab(0);
	rawTabItems.erase(rawTabItems.begin());
	VerifyTabs(rawTabItems);
}

TEST_F(TabViewTest, SelectedIndex)
{
	EXPECT_EQ(m_view.MaybeGetSelectedIndex(), std::nullopt);

	m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab"), 0);
	EXPECT_EQ(m_view.MaybeGetSelectedIndex(), 0);
	EXPECT_EQ(m_view.GetSelectedIndex(), 0);
}

TEST_F(TabViewTest, GetNumTabs)
{
	EXPECT_EQ(m_view.GetNumTabs(), 0);

	std::vector<std::wstring> tabTitles = GetTabTitles();
	AddTabs(tabTitles);
	EXPECT_EQ(m_view.GetNumTabs(), 3);

	m_view.RemoveTab(2);
	EXPECT_EQ(m_view.GetNumTabs(), 2);

	m_view.RemoveTab(0);
	EXPECT_EQ(m_view.GetNumTabs(), 1);

	m_view.RemoveTab(0);
	EXPECT_EQ(m_view.GetNumTabs(), 0);
}

TEST_F(TabViewTest, SelectOnClick)
{
	m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 1"), 0);
	m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 2"), 1);
	m_view.SelectTabAtIndex(0);

	// For the tab control to select a tab on click, the tab needs to be in the visible portion of
	// the control, which is why the window is resized here. Without this, the test would fail,
	// since the control will ignore clicks that occur outside of the client area.
	ResizeWindowToFitTabs();

	// If a tab is clicked outside of the close button, the tab should be selected.
	auto layout = m_testApi.GetTabContentLayout(1);
	SimulateClick(m_view.GetHWND(), { layout.text.left, layout.text.top });
	EXPECT_EQ(m_view.GetSelectedIndex(), 1);

	// If the close button is clicked, the tab shouldn't be selected.
	layout = m_testApi.GetTabContentLayout(0);
	SimulateClick(m_view.GetHWND(), { layout.closeButton.left, layout.closeButton.top });
	EXPECT_EQ(m_view.GetSelectedIndex(), 1);
}

TEST_F(TabViewTest, CloseButtonClick)
{
	m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 1"), 0);

	auto tab2 = std::make_unique<TestTabViewItem>(L"Tab 2");

	MockFunction<void()> closedCallback;
	tab2->SetClosedCallback(closedCallback.AsStdFunction());

	m_view.AddTab(std::move(tab2), 1);

	ResizeWindowToFitTabs();

	auto layout = m_testApi.GetTabContentLayout(1);

	EXPECT_CALL(closedCallback, Call());
	SimulateClick(m_view.GetHWND(), { layout.closeButton.left, layout.closeButton.top });
}

TEST_F(TabViewTest, IgnoredCloseButtonClick)
{
	m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 1"), 0);

	auto tab2 = std::make_unique<TestTabViewItem>(L"Tab 2");

	MockFunction<void()> closedCallback;
	tab2->SetClosedCallback(closedCallback.AsStdFunction());

	m_view.AddTab(std::move(tab2), 1);

	ResizeWindowToFitTabs();

	auto layout = m_testApi.GetTabContentLayout(1);

	EXPECT_CALL(closedCallback, Call()).Times(0);

	// The mouse button goes down on the close button, but up elsewhere on the tab, so the close
	// callback shouldn't be invoked.
	SimulateClick(m_view.GetHWND(), { layout.closeButton.left, layout.closeButton.top },
		{ layout.text.left, layout.text.top });

	// Here, the mouse button goes up on the close button, but down somewhere else, so the close
	// callback shouldn't be invoked in this case either.
	SimulateClick(m_view.GetHWND(), { layout.text.left, layout.text.top },
		{ layout.closeButton.left, layout.closeButton.top });
}

class TabViewPaintTest : public TabViewTest
{
protected:
	enum class ChangeType
	{
		ChangeExpected,
		ChangeNotExpected
	};

	void SetUp() override
	{
		TabViewTest::SetUp();

		int imageSize = 16;
		m_imageList.reset(ImageList_Create(imageSize, imageSize, ILC_COLOR32 | ILC_MASK, 0, 1));
		ASSERT_NE(m_imageList, nullptr);

		std::unique_ptr<Gdiplus::Bitmap> gdiPlusBitmap;
		BuildTestGdiplusBitmap(imageSize, imageSize, gdiPlusBitmap);

		auto bitmap = ImageHelper::GdiplusBitmapToBitmap(gdiPlusBitmap.get());
		ASSERT_NE(bitmap, nullptr);

		m_imageIndex = ImageList_Add(m_imageList.get(), bitmap.get(), nullptr);
		ASSERT_NE(m_imageIndex, -1);

		m_view.SetImageList(m_imageList.get());

		m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 1", m_imageIndex), 0);
		m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 2", m_imageIndex), 1);
		m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 3", m_imageIndex), 2);

		m_view.SelectTabAtIndex(0);

		ResizeWindowToFitTabs();
	}

	void PerformPaintTest(ChangeType changeType)
	{
		RECT clientRect;
		auto res = GetClientRect(m_view.GetHWND(), &clientRect);
		ASSERT_TRUE(res);

		std::unique_ptr<Gdiplus::Bitmap> bitmap;
		BuildTestGdiplusBitmap(clientRect.right, clientRect.bottom, bitmap);

		if (!m_previousBitmap)
		{
			m_previousBitmap = GdiplusHelper::DeepCopyBitmap(bitmap.get());
			ASSERT_NE(m_previousBitmap, nullptr);
		}

		Gdiplus::Graphics graphics(bitmap.get());

		HDC hdc = graphics.GetHDC();
		m_testApi.Paint(hdc, clientRect);
		graphics.ReleaseHDC(hdc);

		EXPECT_EQ(AreGdiplusBitmapsEquivalent(bitmap.get(), m_previousBitmap.get()),
			changeType == ChangeType::ChangeNotExpected);

		m_previousBitmap = GdiplusHelper::DeepCopyBitmap(bitmap.get());
		ASSERT_NE(m_previousBitmap, nullptr);
	}

	wil::unique_himagelist m_imageList;
	int m_imageIndex;
	std::unique_ptr<Gdiplus::Bitmap> m_previousBitmap;
};

// Ultimately, checking that the tab control is being rendered "correctly" is difficult. Performing
// a pixel-level comparison on the generated content and reference content isn't going to work for a
// few reasons:
//
// - The underlying tab control is still responsible for calculating the size of every tab and
//   laying them out. Minor differences in tab size or layout would cause an exact comparison to
//   fail.
// - Different systems can use different DPI levels, causing the control to be rendered at different
//   sizes.
// - Text rendering may differ slightly on different versions of Windows or different systems.
//
// Potentially, a visual comparison could be performed with some allowable percentage difference.
// But that's more complicated and would still potentially have false positives and false negatives.
//
// Therefore, the approach taken here is simply to draw the control into a DC and check that at
// least something has been drawn. This is at least somewhat useful, since the rendering code has
// various DCHECKs/CHECKs, so invoking the code in an automated way helps to verify that the
// functions that are being called are executing correctly.
//
// There is also some verification that what's drawn changes as the state of the control changes.
TEST_F(TabViewPaintTest, Paint)
{
	PerformPaintTest(ChangeType::ChangeExpected);
	PerformPaintTest(ChangeType::ChangeNotExpected);

	// Previously, there was no hot item, whereas item 1 is now marked as hot. The contents of the
	// control should now be different.
	m_testApi.SetHotItemDetails(1, TabViewTestApi::TabPart::Body);
	PerformPaintTest(ChangeType::ChangeExpected);

	// The selection has changed, so the contents of the control should have changed again.
	m_view.SelectTabAtIndex(1);
	m_testApi.ClearHotItemDetails();
	PerformPaintTest(ChangeType::ChangeExpected);

	// Item 1 is selected. Its appearance shouldn't change if it becomes hot. So, the contents of
	// the control shouldn't change here.
	m_testApi.SetHotItemDetails(1, TabViewTestApi::TabPart::Body);
	PerformPaintTest(ChangeType::ChangeNotExpected);

	// The close button on item 1 is now hot, so it should be drawn differently.
	m_testApi.SetHotItemDetails(1, TabViewTestApi::TabPart::CloseButton);
	PerformPaintTest(ChangeType::ChangeExpected);

	m_view.AddTab(std::make_unique<TestTabViewItem>(L"Tab 4", m_imageIndex), 3);
	ResizeWindowToFitTabs();
	PerformPaintTest(ChangeType::ChangeExpected);

	m_view.RemoveTab(3);
	PerformPaintTest(ChangeType::ChangeExpected);
}
