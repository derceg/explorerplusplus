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
#include <unordered_set>

using namespace testing;

namespace
{

class MenuDelegateFake : public MenuDelegate
{
public:
	bool IsItemEnabled(UINT id) const override
	{
		return !m_disabledItems.contains(id);
	}

	bool IsItemChecked(UINT id) const override
	{
		return m_checkedItems.contains(id);
	}

	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override
	{
		UNREFERENCED_PARAMETER(id);
		UNREFERENCED_PARAMETER(isCtrlKeyDown);
		UNREFERENCED_PARAMETER(isShiftKeyDown);
	}

	void SetItemEnabled(UINT id, bool enabled)
	{
		if (enabled)
		{
			m_disabledItems.erase(id);
		}
		else
		{
			m_disabledItems.insert(id);
		}
	}

	void SetItemChecked(UINT id, bool checked)
	{
		if (checked)
		{
			m_checkedItems.insert(id);
		}
		else
		{
			m_checkedItems.erase(id);
		}
	}

private:
	std::unordered_set<UINT> m_disabledItems;
	std::unordered_set<UINT> m_checkedItems;
};

}

class MenuViewTest : public Test
{
protected:
	MenuViewTest() : m_menu(MenuHelper::CheckedCreatePopupMenu()), m_menuView(m_menu.get())
	{
	}

	void CheckAppendItem(UINT itemId, const std::wstring &text, const std::wstring &helpText,
		const std::optional<std::wstring> &acceleratorText = std::nullopt)
	{
		m_menuView.AppendItem(nullptr, itemId, text, {}, helpText, acceleratorText);
		m_appendItemCount++;

		EXPECT_EQ(m_menuView.GetNumItems(), m_appendItemCount);
		EXPECT_EQ(m_menuView.GetItemIdForTesting(m_appendItemCount - 1), itemId);
		EXPECT_EQ(m_menuView.GetItemTextForTesting(itemId),
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

	auto *subMenu = m_menuView.AppendSubMenu(nullptr, 1, L"SubMenu");
	EXPECT_FALSE(subMenu->IsRoot());
}

TEST_F(MenuViewTest, AppendItem)
{
	UINT idCounter = 100;
	CheckAppendItem(idCounter++, L"Item 1", L"Help text for item 1", L"Ctrl+A");
	CheckAppendItem(idCounter++, L"Item 2", L"Help text for item 2", L"Ctrl+Shift+T");
	CheckAppendItem(idCounter++, L"Item 3", L"Help text for item 3");
}

TEST_F(MenuViewTest, AppendRadioItem)
{
	UINT idCounter = 100;

	UINT itemId = idCounter++;
	m_menuView.AppendRadioItem(nullptr, itemId, L"Radio item");

	EXPECT_TRUE(WI_IsFlagSet(
		MenuHelper::GetMenuItemType(m_menuView.GetNativeMenuForTesting(), itemId, false),
		MFT_RADIOCHECK));
}

TEST_F(MenuViewTest, AppendSubMenu)
{
	UINT subMenuItemId = 1;
	m_menuView.AppendSubMenu(nullptr, subMenuItemId, L"SubMenu");
	ASSERT_EQ(m_menuView.GetNumItems(), 1);
	EXPECT_EQ(m_menuView.GetItemIdForTesting(0), subMenuItemId);
	EXPECT_EQ(m_menuView.GetItemTextForTesting(subMenuItemId), L"SubMenu");
}

TEST_F(MenuViewTest, ItemStates)
{
	MenuDelegateFake delegate;
	UINT idCounter = 100;

	UINT itemId1 = idCounter++;
	m_menuView.AppendItem(nullptr, itemId1, L"Item 1");

	UINT itemId2 = idCounter++;
	m_menuView.AppendItem(&delegate, itemId2, L"Item 2");

	UINT itemId3 = idCounter++;
	m_menuView.AppendItem(&delegate, itemId3, L"Item 3");

	delegate.SetItemEnabled(itemId2, false);
	delegate.SetItemChecked(itemId3, true);

	m_menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	HMENU nativeMenu = m_menuView.GetNativeMenuForTesting();

	// Item 1 has no delegate, so it should be enabled and unchecked by default.
	EXPECT_TRUE(MenuHelper::IsMenuItemEnabled(nativeMenu, itemId1, false));
	EXPECT_FALSE(MenuHelper::IsMenuItemChecked(nativeMenu, itemId1, false));

	EXPECT_FALSE(MenuHelper::IsMenuItemEnabled(nativeMenu, itemId2, false));
	EXPECT_FALSE(MenuHelper::IsMenuItemChecked(nativeMenu, itemId2, false));
	EXPECT_TRUE(MenuHelper::IsMenuItemEnabled(nativeMenu, itemId3, false));
	EXPECT_TRUE(MenuHelper::IsMenuItemChecked(nativeMenu, itemId3, false));
}

TEST_F(MenuViewTest, ItemStatesAfterShow)
{
	MenuDelegateFake delegate;
	UINT idCounter = 100;

	UINT itemId1 = idCounter++;
	m_menuView.AppendItem(nullptr, itemId1, L"Item 1");

	m_menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	UINT itemId2 = idCounter++;
	UINT itemId3 = idCounter++;
	delegate.SetItemEnabled(itemId2, false);
	delegate.SetItemChecked(itemId3, true);

	m_menuView.AppendItem(&delegate, itemId2, L"Item 2");
	m_menuView.AppendItem(&delegate, itemId3, L"Item 3");

	// Items that are dynamically added, whilst the menu is being shown, should have their states
	// set correctly.
	HMENU nativeMenu = m_menuView.GetNativeMenuForTesting();
	EXPECT_FALSE(MenuHelper::IsMenuItemEnabled(nativeMenu, itemId2, false));
	EXPECT_FALSE(MenuHelper::IsMenuItemChecked(nativeMenu, itemId2, false));
	EXPECT_TRUE(MenuHelper::IsMenuItemEnabled(nativeMenu, itemId3, false));
	EXPECT_TRUE(MenuHelper::IsMenuItemChecked(nativeMenu, itemId3, false));
}

TEST_F(MenuViewTest, ClearEmptyMenu)
{
	// Clearing an empty menu should have no effect, but also shouldn't cause any issues.
	m_menuView.ClearMenu();
}

TEST_F(MenuViewTest, ClearMenu)
{
	m_menuView.AppendItem(nullptr, 1, L"Item");

	m_menuView.ClearMenu();
	EXPECT_EQ(m_menuView.GetNumItems(), 0);
}

using MenuViewDeathTest = MenuViewTest;

TEST_F(MenuViewDeathTest, RetrieveHelpTextAfterClearingMenu)
{
	UINT itemId = 1;
	m_menuView.AppendItem(nullptr, itemId, L"Item", {}, L"Help text");

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

			menuView->AppendItem(nullptr, m_idCounter++, std::format(L"Item {}", i),
				std::move(iconModel));
		}
	}

	ShellIconLoaderFake m_shellIconLoader;
	wil::unique_hmenu m_menu;

private:
	UINT m_idCounter = 100;
};

TEST_F(MenuViewIconTest, InitialState)
{
	MenuView menuView(m_menu.get());
	AddItemsToMenu(&menuView, 1);

	// Even though an image was assigned to the item, no image should be added to the menu until the
	// menu is shown (so that DPI scaling can be applied).
	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
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

	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, OnShowWithNoImage)
{
	MenuView menuView(m_menu.get());
	AddItemsToMenu(&menuView, 1, ImageOption::Exclude);

	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	// The item didn't have any image set, so showing the menu shouldn't result in any image being
	// assigned.
	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
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

	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
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

	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
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

	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
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

	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, AddItemAfterClose)
{
	MenuView menuView(m_menu.get());
	menuView.OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);
	menuView.OnPopupClosedForTesting();

	AddItemsToMenu(&menuView, 1);

	auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(0));
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
		auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(i));
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
		auto bitmap = menuView.GetItemBitmapForTesting(menuView.GetItemIdForTesting(i));
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
