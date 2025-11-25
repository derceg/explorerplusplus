// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "GTestHelper.h"
#include "MenuViewFake.h"
#include "PidlTestHelper.h"
#include "ShellIconLoaderFake.h"
#include "ShellIconModel.h"
#include <gtest/gtest.h>

using namespace testing;

class MenuViewTest : public Test
{
protected:
	void CheckAppendItem(UINT itemId, const std::wstring &text, const std::wstring &helpText,
		const std::optional<std::wstring> &acceleratorText = std::nullopt)
	{
		m_menuView.AppendItem(itemId, text, {}, helpText, acceleratorText);
		m_appendItemCount++;

		EXPECT_EQ(m_menuView.GetItemCount(), m_appendItemCount);
		EXPECT_EQ(m_menuView.GetItemId(m_appendItemCount - 1), itemId);
		EXPECT_EQ(m_menuView.GetItemText(itemId),
			acceleratorText ? text + L"\t" + *acceleratorText : text);
		EXPECT_EQ(m_menuView.GetItemHelpText(itemId), helpText);
	}

	MenuViewFake m_menuView;

private:
	int m_appendItemCount = 0;
};

TEST_F(MenuViewTest, AppendItem)
{
	UINT idCounter = 100;

	auto pidl1 = CreateSimplePidlForTest(L"C:\\Fake1");
	CheckAppendItem(idCounter++, L"Item 1", L"Help text for item 1", L"Ctrl+A");

	auto pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	CheckAppendItem(idCounter++, L"Item 2", L"Help text for item 2", L"Ctrl+Shift+T");

	CheckAppendItem(idCounter++, L"Item 3", L"Help text for item 3");
}

TEST_F(MenuViewTest, ClearEmptyMenu)
{
	// Clearing an empty menu should have no effect, but also shouldn't cause any issues.
	m_menuView.ClearMenu();
}

TEST_F(MenuViewTest, ClearMenu)
{
	m_menuView.AppendItem(1, L"Item");

	m_menuView.ClearMenu();
	EXPECT_EQ(m_menuView.GetItemCount(), 0);
}

using MenuViewDeathTest = MenuViewTest;

TEST_F(MenuViewDeathTest, RetrieveHelpTextAfterClearingMenu)
{
	UINT itemId = 1;
	m_menuView.AppendItem(itemId, L"Item", {}, L"Help text");

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

			menuView->AppendItem(m_idCounter++, std::format(L"Item {}", i), std::move(iconModel));
		}
	}

	ShellIconLoaderFake m_shellIconLoader;

private:
	UINT m_idCounter = 100;
};

TEST_F(MenuViewIconTest, InitialState)
{
	MenuViewFake menuView;
	AddItemsToMenu(&menuView, 1);

	// Even though an image was assigned to the item, no image should be added to the menu until the
	// menu is shown (so that DPI scaling can be applied).
	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, nullptr);
}

TEST_F(MenuViewIconTest, OnShow)
{
	MenuViewFake menuView;
	AddItemsToMenu(&menuView, 1);

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	// The item image should be set once the menu is shown.
	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, OnShowWithNoImage)
{
	MenuViewFake menuView;
	AddItemsToMenu(&menuView, 1, ImageOption::Exclude);

	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);

	// The item didn't have any image set, so showing the menu shouldn't result in any image being
	// assigned.
	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, nullptr);
}

TEST_F(MenuViewIconTest, MenuBeingShown)
{
	MenuViewFake menuView;
	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	// The menu is being shown, so the item image should be immediately added.
	AddItemsToMenu(&menuView, 1);
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, ShowAfterDpiChange)
{
	MenuViewFake menuView;

	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);
	AddItemsToMenu(&menuView, 1);
	menuView.OnMenuClosed();

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	// The menu is being shown again, but at a different DPI level, so the image should be updated.
	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI * 2);
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, ShowAtSameDpi)
{
	MenuViewFake menuView;

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);
	AddItemsToMenu(&menuView, 1);
	menuView.OnMenuClosed();
	EXPECT_NE(generatedBitmap, nullptr);

	// The menu is being shown again, at the same DPI level, so the image shouldn't be updated.
	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);

	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, IconUpdate)
{
	MenuViewFake menuView;
	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);

	AddItemsToMenu(&menuView, 1);

	HBITMAP generatedBitmap = nullptr;
	m_shellIconLoader.SetBitmapGeneratedCallback(
		[&generatedBitmap](HBITMAP bitmap) { generatedBitmap = bitmap; });

	m_shellIconLoader.TriggerPendingUpdateCallbacks();
	EXPECT_NE(generatedBitmap, nullptr);

	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, generatedBitmap);
}

TEST_F(MenuViewIconTest, AddItemAfterClose)
{
	MenuViewFake menuView;
	menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);
	menuView.OnMenuClosed();

	AddItemsToMenu(&menuView, 1);

	auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(0));
	EXPECT_EQ(bitmap, nullptr);
}

TEST_F(MenuViewIconTest, IconRetrievalAfterMenuRebuilt)
{
	MenuViewFake menuView;
	AddItemsToMenu(&menuView, 3);

	// When the menu is rebuilt, the view will provide update callbacks to the icon loader.
	// Instructing the loader to ignore those callbacks will ensure that the only callbacks present
	// are the ones for the original items.
	m_shellIconLoader.SetStoreUpdateCallbacks(false);

	menuView.ClearMenu();

	AddItemsToMenu(&menuView, 3);

	std::vector<HBITMAP> originalBitmaps;

	for (int i = 0; i < menuView.GetItemCount(); i++)
	{
		auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(i));
		originalBitmaps.push_back(bitmap);
	}

	// This will trigger the update callbacks for the original menu items (i.e. the menu items that
	// existed before the menu was rebuilt). This call should have no effect, since the original
	// menu items no longer exist.
	m_shellIconLoader.TriggerPendingUpdateCallbacks();

	// As the callbacks that were triggered were for the original items on the menu, the images for
	// the new items shouldn't have changed.
	for (int i = 0; i < menuView.GetItemCount(); i++)
	{
		auto bitmap = menuView.GetItemBitmap(menuView.GetItemId(i));
		EXPECT_EQ(bitmap, originalBitmaps[i]);
	}
}

TEST_F(MenuViewIconTest, IconRetrievalAfterMenuDestroyed)
{
	auto menuView = std::make_unique<MenuViewFake>();
	AddItemsToMenu(menuView.get(), 3);

	menuView.reset();

	// If one or more icons are retrieved after the menu has been closed and destroyed, the menu
	// can't be updated, but that should still be a safe operation.
	m_shellIconLoader.TriggerPendingUpdateCallbacks();
}

namespace
{

class MenuHelpTextHostFake : public MenuHelpTextHost
{
public:
	void MenuItemSelected(HMENU menu, UINT itemId, UINT flags) override
	{
		UNREFERENCED_PARAMETER(menu);
		UNREFERENCED_PARAMETER(itemId);
		UNREFERENCED_PARAMETER(flags);
	}

	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override
	{
		return m_menuHelpTextRequestSignal.connect(observer);
	}

	std::optional<std::wstring> TriggerHelpTextRequest(HMENU menu, UINT id)
	{
		return m_menuHelpTextRequestSignal(menu, id);
	}

private:
	MenuHelpTextRequestSignal m_menuHelpTextRequestSignal;
};

}

class MenuViewHelpTextRequestTest : public Test
{
protected:
	MenuViewHelpTextRequestTest() : m_menuView(&m_menuHelpTextHost)
	{
	}

	MenuHelpTextHostFake m_menuHelpTextHost;
	MenuViewFake m_menuView;
};

TEST_F(MenuViewHelpTextRequestTest, HelpTextRequest)
{
	UINT itemId = 1;
	std::wstring helpText = L"Help text";
	m_menuView.AppendItem(itemId, L"Item", {}, helpText);

	// The menu isn't being shown, so no help text should be returned.
	auto retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(m_menuView.GetMenu(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);

	m_menuView.OnMenuWillShowForDpi(USER_DEFAULT_SCREEN_DPI);

	// The menu is now being shown, so help text should be returned.
	retrievedHelpText = m_menuHelpTextHost.TriggerHelpTextRequest(m_menuView.GetMenu(), itemId);
	EXPECT_EQ(retrievedHelpText, helpText);

	m_menuView.OnMenuClosed();

	// The menu has been closed, so, again, no help text should be returned.
	retrievedHelpText = m_menuHelpTextHost.TriggerHelpTextRequest(m_menuView.GetMenu(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);
}
