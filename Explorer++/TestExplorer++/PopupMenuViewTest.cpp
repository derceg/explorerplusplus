// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "PopupMenuView.h"
#include "GTestHelper.h"
#include "ShellIconLoaderFake.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class PopupMenuViewTest : public Test
{
protected:
	void CheckAppendItem(UINT itemId, const std::wstring &text, const ShellIconModel &shellIcon,
		const std::wstring &helpText,
		const std::optional<std::wstring> &acceleratorText = std::nullopt)
	{
		HBITMAP rawBitmap = nullptr;
		m_shellIconLoader.SetBitmapGeneratedCallback(
			[&rawBitmap](HBITMAP bitmap) { rawBitmap = bitmap; });

		m_popupMenu.AppendItem(itemId, text, shellIcon, helpText, acceleratorText);
		m_appendItemCount++;

		// If a shell icon was specified, the callback above should be invoked and a bitmap
		// generated.
		if (!shellIcon.IsEmpty())
		{
			EXPECT_NE(rawBitmap, nullptr);
		}

		EXPECT_EQ(m_popupMenu.GetItemCountForTesting(), m_appendItemCount);
		EXPECT_EQ(m_popupMenu.GetItemIdForTesting(m_appendItemCount - 1), itemId);
		EXPECT_EQ(m_popupMenu.GetItemTextForTesting(itemId),
			acceleratorText ? text + L"\t" + *acceleratorText : text);
		EXPECT_EQ(m_popupMenu.GetItemBitmapForTesting(itemId), rawBitmap);
		EXPECT_EQ(m_popupMenu.GetHelpTextForItem(itemId), helpText);

		HBITMAP updatedRawBitmap = nullptr;
		m_shellIconLoader.SetBitmapGeneratedCallback(
			[&updatedRawBitmap](HBITMAP bitmap) { updatedRawBitmap = bitmap; });

		m_shellIconLoader.TriggerPendingUpdateCallbacks();

		// As with the above, an updated bitmap should be generated if an icon was originally
		// provided.
		if (!shellIcon.IsEmpty())
		{
			EXPECT_NE(updatedRawBitmap, nullptr);
		}

		EXPECT_EQ(m_popupMenu.GetItemBitmapForTesting(itemId), updatedRawBitmap);

		m_shellIconLoader.ClearBitmapGeneratedCallback();
	}

	ShellIconLoaderFake m_shellIconLoader;
	PopupMenuView m_popupMenu;

private:
	int m_appendItemCount = 0;
};

TEST_F(PopupMenuViewTest, AppendItem)
{
	UINT idCounter = 100;

	auto pidl1 = CreateSimplePidlForTest(L"C:\\Fake1");
	CheckAppendItem(idCounter++, L"Item 1", ShellIconModel(&m_shellIconLoader, pidl1.Raw()),
		L"Help text for item 1", L"Ctrl+A");

	auto pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	CheckAppendItem(idCounter++, L"Item 2", ShellIconModel(&m_shellIconLoader, pidl2.Raw()),
		L"Help text for item 2", L"Ctrl+Shift+T");

	CheckAppendItem(idCounter++, L"Item 3", ShellIconModel(), L"Help text for item 3");
}

TEST_F(PopupMenuViewTest, ClearEmptyMenu)
{
	// Clearing an empty menu should have no effect, but also shouldn't cause any issues.
	m_popupMenu.ClearMenu();
}

TEST_F(PopupMenuViewTest, ClearMenu)
{
	m_popupMenu.AppendItem(1, L"Item");

	m_popupMenu.ClearMenu();
	EXPECT_EQ(m_popupMenu.GetItemCountForTesting(), 0);
}

using PopupMenuViewDeathTest = PopupMenuViewTest;

TEST_F(PopupMenuViewDeathTest, RetrieveHelpTextAfterClearingMenu)
{
	UINT itemId = 1;
	m_popupMenu.AppendItem(itemId, L"Item", {}, L"Help text");

	m_popupMenu.ClearMenu();

	// The menu was cleared, so attempting to retrieve the help text for the previously inserted
	// item should fail.
	EXPECT_CHECK_DEATH(m_popupMenu.GetHelpTextForItem(itemId));
}

class PopupMenuViewIconTest : public Test
{
protected:
	void AddItemsToMenu(PopupMenuView *popupMenu, int numItems)
	{
		for (int i = 0; i < numItems; i++)
		{
			auto pidl = CreateSimplePidlForTest(std::format(L"C:\\Fake{}", i));
			popupMenu->AppendItem(m_idCounter++, std::format(L"Item {}", i),
				ShellIconModel(&m_shellIconLoader, pidl.Raw()));
		}
	}

	ShellIconLoaderFake m_shellIconLoader;

private:
	UINT m_idCounter = 100;
};

TEST_F(PopupMenuViewIconTest, IconRetrievalAfterMenuRebuilt)
{
	PopupMenuView popupMenu;
	AddItemsToMenu(&popupMenu, 3);

	// When the menu is rebuilt, the view will provide update callbacks to the icon loader.
	// Instructing the loader to ignore those callbacks will ensure that the only callbacks present
	// are the ones for the original items.
	m_shellIconLoader.SetStoreUpdateCallbacks(false);

	popupMenu.ClearMenu();

	AddItemsToMenu(&popupMenu, 3);

	std::vector<HBITMAP> originalBitmaps;

	for (int i = 0; i < popupMenu.GetItemCountForTesting(); i++)
	{
		auto bitmap = popupMenu.GetItemBitmapForTesting(popupMenu.GetItemIdForTesting(i));
		originalBitmaps.push_back(bitmap);
	}

	// This will trigger the update callbacks for the original menu items (i.e. the menu items that
	// existed before the menu was rebuilt). This call should have no effect, since the original
	// menu items no longer exist.
	m_shellIconLoader.TriggerPendingUpdateCallbacks();

	// As the callbacks that were triggered were for the original items on the menu, the images for
	// the new items shouldn't have changed.
	for (int i = 0; i < popupMenu.GetItemCountForTesting(); i++)
	{
		auto bitmap = popupMenu.GetItemBitmapForTesting(popupMenu.GetItemIdForTesting(i));
		EXPECT_EQ(bitmap, originalBitmaps[i]);
	}
}

TEST_F(PopupMenuViewIconTest, IconRetrievalAfterMenuDestroyed)
{
	auto popupMenu = std::make_unique<PopupMenuView>();
	AddItemsToMenu(popupMenu.get(), 3);

	popupMenu.reset();

	// If one or more icons are retrieved after the menu has been closed and destroyed, the menu
	// can't be updated, but that should still be a safe operation.
	m_shellIconLoader.TriggerPendingUpdateCallbacks();
}
