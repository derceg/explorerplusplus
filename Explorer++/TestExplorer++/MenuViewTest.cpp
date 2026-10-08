// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuView.h"
#include "GTestHelper.h"
#include "PidlTestHelper.h"
#include "ShellIconLoaderFake.h"
#include "ShellIconModel.h"
#include "../Helper/MenuHelper.h"
#include <gtest/gtest.h>
#include <wil/resource.h>

using namespace testing;

class MenuViewTest : public Test
{
protected:
	MenuViewTest() : m_menu(MenuHelper::CheckedCreatePopupMenu()), m_menuView(m_menu.get())
	{
	}

	void CheckAppendItem(const std::wstring &text, const std::wstring &helpText,
		const std::optional<std::wstring> &acceleratorText = std::nullopt)
	{
		m_menuView.AppendItem(
			MenuItemParams(text).SetHelpText(helpText).SetAcceleratorText(acceleratorText));
		m_appendItemCount++;

		UINT itemId = m_menuView.GetItemIdForTesting(m_appendItemCount - 1);

		EXPECT_EQ(m_menuView.GetNumItems(), m_appendItemCount);
		EXPECT_EQ(
			MenuHelper::GetItemText(m_menuView.GetNativeMenuForTesting(), itemId, false),
			acceleratorText ? text + L"\t" + *acceleratorText : text);
		EXPECT_EQ(m_menuView.GetItemHelpText(itemId), helpText);
	}

	wil::unique_hmenu m_menu;
	MenuView m_menuView;

private:
	int m_appendItemCount = 0;
};

TEST_F(MenuViewTest, IsRoot)
{
	EXPECT_TRUE(m_menuView.IsRoot());

	auto *subMenu = m_menuView.AppendSubMenu(SubMenuItemParams(L"SubMenu"));
	EXPECT_FALSE(subMenu->IsRoot());
}

TEST_F(MenuViewTest, AppendItem)
{
	CheckAppendItem(L"Item 1", L"Help text for item 1", L"Ctrl+A");
	CheckAppendItem(L"Item 2", L"Help text for item 2", L"Ctrl+Shift+T");
	CheckAppendItem(L"Item 3", L"Help text for item 3");
}

TEST_F(MenuViewTest, SetCheckStyle)
{
	m_menuView.AppendItem(MenuItemParams(L"Radio item").SetCheckStyle(MenuItemCheckStyle::Radio));

	EXPECT_TRUE(
		WI_IsFlagSet(MenuHelper::GetItemTypeFlags(m_menuView.GetNativeMenuForTesting(), 0, true),
			MFT_RADIOCHECK));
}

TEST_F(MenuViewTest, AppendSubMenu)
{
	enum class Item
	{
		SubMenu
	};

	m_menuView.AppendSubMenu(SubMenuItemParams(L"SubMenu").SetKey(Item::SubMenu));
	ASSERT_EQ(m_menuView.GetNumItems(), 1);

	auto id = m_menuView.MaybeGetIdForKeyForTesting(Item::SubMenu);
	ASSERT_NE(id, std::nullopt);
	EXPECT_EQ(MenuHelper::GetItemText(m_menuView.GetNativeMenuForTesting(), *id, false),
		L"SubMenu");
}

TEST_F(MenuViewTest, ItemStates)
{
	m_menuView.AppendItem(MenuItemParams(L"Item 1"));
	m_menuView.AppendItem(MenuItemParams(L"Item 2").SetEnabledCallback([] { return false; }));
	m_menuView.AppendItem(MenuItemParams(L"Item 3").SetCheckedCallback([] { return true; }));

	m_menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	HMENU nativeMenu = m_menuView.GetNativeMenuForTesting();

	// If an item provides no enabled/checked callbacks, it should be enabled and unchecked by
	// default.
	EXPECT_TRUE(MenuHelper::IsItemEnabled(nativeMenu, 0, true));
	EXPECT_FALSE(MenuHelper::IsItemChecked(nativeMenu, 0, true));

	EXPECT_FALSE(MenuHelper::IsItemEnabled(nativeMenu, 1, true));
	EXPECT_FALSE(MenuHelper::IsItemChecked(nativeMenu, 1, true));
	EXPECT_TRUE(MenuHelper::IsItemEnabled(nativeMenu, 2, true));
	EXPECT_TRUE(MenuHelper::IsItemChecked(nativeMenu, 2, true));
}

TEST_F(MenuViewTest, ItemStatesAfterShow)
{
	m_menuView.AppendItem(MenuItemParams(L"Item 1"));

	m_menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	m_menuView.AppendItem(MenuItemParams(L"Item 2").SetEnabledCallback([] { return false; }));
	m_menuView.AppendItem(MenuItemParams(L"Item 3").SetCheckedCallback([] { return true; }));

	HMENU nativeMenu = m_menuView.GetNativeMenuForTesting();

	// Items that are dynamically added, whilst the menu is being shown, should have their states
	// set correctly.
	EXPECT_FALSE(MenuHelper::IsItemEnabled(nativeMenu, 1, true));
	EXPECT_FALSE(MenuHelper::IsItemChecked(nativeMenu, 1, true));
	EXPECT_TRUE(MenuHelper::IsItemEnabled(nativeMenu, 2, true));
	EXPECT_TRUE(MenuHelper::IsItemChecked(nativeMenu, 2, true));
}

TEST_F(MenuViewTest, ClearEmptyMenu)
{
	// Clearing an empty menu should have no effect, but also shouldn't cause any issues.
	m_menuView.ClearMenu();
}

TEST_F(MenuViewTest, ClearMenu)
{
	m_menuView.AppendItem(MenuItemParams(L"Item"));

	m_menuView.ClearMenu();
	EXPECT_EQ(m_menuView.GetNumItems(), 0);
}

using MenuViewDeathTest = MenuViewTest;

TEST_F(MenuViewDeathTest, RetrieveHelpTextAfterClearingMenu)
{
	m_menuView.AppendItem(MenuItemParams(L"Item").SetHelpText(L"Help text"));
	UINT itemId = m_menuView.GetItemIdForTesting(0);

	m_menuView.ClearMenu();

	// The menu was cleared, so attempting to retrieve the help text for the previously inserted
	// item should fail.
	EXPECT_CHECK_DEATH(m_menuView.GetItemHelpText(itemId));
}

class MenuViewIconTest : public Test
{
protected:
	enum class ImageOption
	{
		Include,
		Exclude
	};

	MenuViewIconTest() : m_menu(MenuHelper::CheckedCreatePopupMenu())
	{
	}

	void AddItemsToMenu(MenuView *menuView, int numItems,
		ImageOption imageOption = ImageOption::Include)
	{
		for (int i = 0; i < numItems; i++)
		{
			std::unique_ptr<ShellIconModel> iconModel;

			if (imageOption == ImageOption::Include)
			{
				auto pidl = CreateSimplePidlForTest(std::format(L"C:\\Fake{}", i));
				iconModel = std::make_unique<ShellIconModel>(&m_shellIconLoader, pidl.Raw());
			}

			menuView->AppendItem(
				MenuItemParams(std::format(L"Item {}", i)).SetIconModel(std::move(iconModel)));
		}
	}

	ShellIconLoaderFake m_shellIconLoader;
	wil::unique_hmenu m_menu;
};

TEST_F(MenuViewIconTest, InitialState)
{
	MenuView menuView(m_menu.get());
	AddItemsToMenu(&menuView, 1);

	// Even though an image was assigned to the item, no image should be added to the menu until the
	// menu is shown (so that DPI scaling can be applied).
	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, nullptr);
}

TEST_F(MenuViewIconTest, OnShow)
{
	MenuView menuView(m_menu.get());
	AddItemsToMenu(&menuView, 1);

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	// The item image should be set once the menu is shown.
	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, OnShowWithNoImage)
{
	MenuView menuView(m_menu.get());
	AddItemsToMenu(&menuView, 1, ImageOption::Exclude);

	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	// The item didn't have any image set, so showing the menu shouldn't result in any image being
	// assigned.
	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, nullptr);
}

TEST_F(MenuViewIconTest, MenuBeingShown)
{
	MenuView menuView(m_menu.get());
	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	// The menu is being shown, so the item image should be immediately added.
	AddItemsToMenu(&menuView, 1);
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, ShowAfterDpiChange)
{
	MenuView menuView(m_menu.get());

	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);
	AddItemsToMenu(&menuView, 1);
	menuView.OnPopupClosedForTesting();

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	// The menu is being shown again, but at a different DPI level, so the image should be updated.
	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI * 2);
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, ShowAtSameDpi)
{
	MenuView menuView(m_menu.get());

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);
	AddItemsToMenu(&menuView, 1);
	menuView.OnPopupClosedForTesting();
	EXPECT_NE(generatedBitmap, nullptr);

	// The menu is being shown again, at the same DPI level, so the image shouldn't be updated.
	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, IconUpdate)
{
	MenuView menuView(m_menu.get());
	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	AddItemsToMenu(&menuView, 1);

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	m_shellIconLoader.TriggerPendingUpdateCallbacks();
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, AddItemAfterClose)
{
	MenuView menuView(m_menu.get());
	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);
	menuView.OnPopupClosedForTesting();

	AddItemsToMenu(&menuView, 1);

	auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), 0, true);
	EXPECT_EQ(bitmap, nullptr);
}

TEST_F(MenuViewIconTest, IconRetrievalAfterMenuRebuilt)
{
	MenuView menuView(m_menu.get());
	AddItemsToMenu(&menuView, 3);

	// When the menu is rebuilt, the view will provide update callbacks to the icon loader.
	// Instructing the loader to ignore those callbacks will ensure that the only callbacks present
	// are the ones for the original items.
	m_shellIconLoader.SetStoreUpdateCallbacks(false);

	menuView.ClearMenu();

	AddItemsToMenu(&menuView, 3);

	std::vector<HBITMAP> originalBitmaps;

	for (int i = 0; i < menuView.GetNumItems(); i++)
	{
		auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), i, true);
		originalBitmaps.push_back(bitmap);
	}

	// This will trigger the update callbacks for the original menu items (i.e. the menu items that
	// existed before the menu was rebuilt). This call should have no effect, since the original
	// menu items no longer exist.
	m_shellIconLoader.TriggerPendingUpdateCallbacks();

	// As the callbacks that were triggered were for the original items on the menu, the images for
	// the new items shouldn't have changed.
	for (int i = 0; i < menuView.GetNumItems(); i++)
	{
		auto bitmap = MenuHelper::GetItemBitmap(menuView.GetNativeMenuForTesting(), i, true);
		EXPECT_EQ(bitmap, originalBitmaps[i]);
	}
}

TEST_F(MenuViewIconTest, IconRetrievalAfterMenuDestroyed)
{
	auto menuView = std::make_unique<MenuView>(m_menu.get());
	AddItemsToMenu(menuView.get(), 3);

	menuView.reset();

	// If one or more icons are retrieved after the menu has been closed and destroyed, the menu
	// can't be updated, but that should still be a safe operation.
	m_shellIconLoader.TriggerPendingUpdateCallbacks();
}
