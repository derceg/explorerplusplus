// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "PopupMenuView.h"
#include "TestResources.h"
#include "../Helper/ImageHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class PopupMenuViewTest : public Test
{
protected:
	void CheckAppendItem(UINT itemId, const std::wstring &text, wil::unique_hbitmap bitmap)
	{
		auto rawBitmap = bitmap.get();

		m_popupMenu.AppendItem(itemId, text, std::move(bitmap));
		m_appendItemCount++;

		EXPECT_EQ(m_popupMenu.GetItemCountForTesting(), m_appendItemCount);
		EXPECT_EQ(m_popupMenu.GetItemIdForTesting(m_appendItemCount - 1), itemId);
		EXPECT_EQ(m_popupMenu.GetItemTextForTesting(itemId), text);
		EXPECT_EQ(m_popupMenu.GetItemBitmapForTesting(itemId), rawBitmap);
	}

	void GetBasicBitmap(wil::unique_hbitmap &outputBitmap) const
	{
		auto png = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), IDB_BASIC_PNG);
		ASSERT_NE(png, nullptr);

		auto bitmap = ImageHelper::GdiplusBitmapToBitmap(png.get());
		ASSERT_NE(bitmap, nullptr);

		outputBitmap = std::move(bitmap);
	}

	PopupMenuView m_popupMenu;

private:
	int m_appendItemCount = 0;
};

TEST_F(PopupMenuViewTest, AppendItem)
{
	UINT idCounter = 100;

	wil::unique_hbitmap bitmap;
	GetBasicBitmap(bitmap);

	CheckAppendItem(idCounter++, L"Item 1", std::move(bitmap));
	CheckAppendItem(idCounter++, L"Item 2", nullptr);
	CheckAppendItem(idCounter++, L"Item 3", nullptr);
}

TEST_F(PopupMenuViewTest, SetBitmapForItem)
{
	UINT id = 1;
	m_popupMenu.AppendItem(id, L"Item", nullptr);

	wil::unique_hbitmap bitmap;
	GetBasicBitmap(bitmap);
	auto rawBitmap = bitmap.get();
	m_popupMenu.SetBitmapForItem(id, std::move(bitmap));

	auto retrievedBitmap = m_popupMenu.GetItemBitmapForTesting(id);
	EXPECT_EQ(retrievedBitmap, rawBitmap);
}
