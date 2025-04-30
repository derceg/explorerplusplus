// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DarkModeTabControlPainter.h"
#include "DarkModeColorProvider.h"
#include "ImageTestHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/GdiplusHelper.h"
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <memory>
#include <optional>

using namespace testing;

class DarkModeTabControlPainterTest : public Test
{
protected:
	enum class ChangeType
	{
		ChangeExpected,
		ChangeNotExpected
	};

	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP | SS_GRAYRECT, 0, 0, 0, 0,
			nullptr, nullptr, GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		m_tabControl.reset(
			CreateTabControl(m_parentWindow.get(), WS_CHILD | TCS_FOCUSNEVER | TCS_SINGLELINE));
		ASSERT_NE(m_tabControl, nullptr);

		AddInitialTabs();

		// This is explicitly called after the initial tabs have been added, to ensure that the
		// height includes enough space for the tabs.
		RECT displayRect = { 0, 0, 0, 0 };
		TabCtrl_AdjustRect(m_tabControl.get(), true, &displayRect);
		int height = std::abs(displayRect.top);

		const int width = 500;
		auto res = MoveWindow(m_parentWindow.get(), 0, 0, width, height, false);
		ASSERT_TRUE(res);

		res = MoveWindow(m_tabControl.get(), 0, 0, width, height, false);
		ASSERT_TRUE(res);

		m_painter = std::make_unique<DarkModeTabControlPainter>(m_tabControl.get(),
			&m_darkModeColorProvider);
	}

	void AddInitialTabs()
	{
		AppendTab(L"First tab");
		AppendTab(L"Second tab");
		AppendTab(L"Third tab");

		int res = TabCtrl_SetCurSel(m_tabControl.get(), 0);
		ASSERT_NE(res, -1);
	}

	void AppendTab(const std::wstring &text)
	{
		int index = TabCtrl_GetItemCount(m_tabControl.get());

		TCITEM tcItem = {};
		tcItem.mask = TCIF_TEXT;
		tcItem.pszText = const_cast<wchar_t *>(text.c_str());
		int insertedIndex = TabCtrl_InsertItem(m_tabControl.get(), index, &tcItem);
		ASSERT_EQ(insertedIndex, index);
	}

	void PerformPaintTest(ChangeType changeType)
	{
		RECT clientRect;
		auto res = GetClientRect(m_tabControl.get(), &clientRect);
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
		m_painter->Paint(hdc, clientRect);
		graphics.ReleaseHDC(hdc);

		EXPECT_EQ(AreGdiplusBitmapsEquivalent(bitmap.get(), m_previousBitmap.get()),
			changeType == ChangeType::ChangeNotExpected);

		m_previousBitmap = GdiplusHelper::DeepCopyBitmap(bitmap.get());
		ASSERT_NE(m_previousBitmap, nullptr);
	}

	DarkModeColorProvider m_darkModeColorProvider;
	wil::unique_hwnd m_parentWindow;
	wil::unique_hwnd m_tabControl;
	std::unique_ptr<DarkModeTabControlPainter> m_painter;
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
TEST_F(DarkModeTabControlPainterTest, Paint)
{
	PerformPaintTest(ChangeType::ChangeExpected);

	// Previously, there was no hot item, whereas item 1 is now marked as hot. The contents of the
	// control should now be different.
	m_painter->SetHotItem(1);
	PerformPaintTest(ChangeType::ChangeExpected);

	// The selection has changed, so the contents of the control should have changed again.
	ASSERT_NE(TabCtrl_SetCurSel(m_tabControl.get(), 1), -1);
	m_painter->ClearHotItem();
	PerformPaintTest(ChangeType::ChangeExpected);

	// Item 1 is selected. Its appearance shouldn't change if it becomes hot. So, the contents of
	// the control shouldn't change here.
	m_painter->SetHotItem(1);
	PerformPaintTest(ChangeType::ChangeNotExpected);

	AppendTab(L"Fourth tab");
	PerformPaintTest(ChangeType::ChangeExpected);

	ASSERT_TRUE(TabCtrl_DeleteItem(m_tabControl.get(), 3));
	PerformPaintTest(ChangeType::ChangeExpected);
}
