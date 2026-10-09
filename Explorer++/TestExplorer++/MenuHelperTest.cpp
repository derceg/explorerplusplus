// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/MenuHelper.h"
#include <gtest/gtest.h>
#include <wil/resource.h>

using namespace testing;

TEST(MenuHelperTest, StyleFlags)
{
	auto menu = MenuHelper::CheckedCreatePopupMenu();

	MenuHelper::AddStyleFlags(menu.get(), MNS_NOTIFYBYPOS);
	EXPECT_TRUE(WI_IsFlagSet(MenuHelper::GetStyleFlags(menu.get()), MNS_NOTIFYBYPOS));
}

TEST(MenuHelperTest, RemoveDuplicateSeparators)
{
	const UINT item1Id = 1;
	const UINT item2Id = 2;

	auto menu = MenuHelper::CheckedCreatePopupMenu();
	MenuHelper::AddStringItem(menu.get(), item1Id, L"Item 1");
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddStringItem(menu.get(), item2Id, L"Item 2");

	// This should leave a single separator, along with the two other menu items.
	MenuHelper::RemoveDuplicateSeparators(menu.get());
	EXPECT_EQ(GetMenuItemCount(menu.get()), 3);
	EXPECT_EQ(GetMenuItemID(menu.get(), 0), item1Id);
	EXPECT_EQ(GetMenuItemID(menu.get(), 2), item2Id);
}

TEST(MenuHelperTest, RemoveTrailingSeparators)
{
	const UINT itemId = 1;

	auto menu = MenuHelper::CheckedCreatePopupMenu();
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());

	MenuHelper::RemoveTrailingSeparators(menu.get());
	EXPECT_EQ(GetMenuItemCount(menu.get()), 1);
	EXPECT_EQ(GetMenuItemID(menu.get(), 0), itemId);
}

TEST(MenuHelperTest, GetItemText)
{
	const UINT itemId = 1;

	auto menu = MenuHelper::CheckedCreatePopupMenu();
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");

	auto text = MenuHelper::GetItemText(menu.get(), itemId, MenuHelper::ItemLookup::ById);
	EXPECT_EQ(text, L"Item");
}

TEST(MenuHelperTest, GetItemIdIncludingSubmenu)
{
	const UINT itemId = 1;
	const UINT submenuItemId = 2;

	auto submenu = MenuHelper::CheckedCreatePopupMenu();

	auto menu = MenuHelper::CheckedCreatePopupMenu();
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");
	MenuHelper::AddSubMenuItem(menu.get(), submenuItemId, L"Submenu", std::move(submenu));

	// Unlike GetMenuItemID(), this function should return the ID set for both items and submenu
	// items.
	EXPECT_EQ(MenuHelper::GetItemIdIncludingSubmenu(menu.get(), 0), itemId);
	EXPECT_EQ(MenuHelper::GetItemIdIncludingSubmenu(menu.get(), 1), submenuItemId);
}

TEST(MenuHelperTest, EnabledState)
{
	const UINT itemId = 1;

	auto menu = MenuHelper::CheckedCreatePopupMenu();
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");
	EXPECT_TRUE(MenuHelper::IsItemEnabled(menu.get(), itemId, MenuHelper::ItemLookup::ById));

	MenuHelper::SetItemEnabled(menu.get(), itemId, MenuHelper::ItemLookup::ById, false);
	EXPECT_FALSE(MenuHelper::IsItemEnabled(menu.get(), itemId, MenuHelper::ItemLookup::ById));
}

TEST(MenuHelperTest, CheckedState)
{
	const UINT itemId = 1;

	auto menu = MenuHelper::CheckedCreatePopupMenu();
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");
	EXPECT_FALSE(MenuHelper::IsItemChecked(menu.get(), itemId, MenuHelper::ItemLookup::ById));

	MenuHelper::SetItemChecked(menu.get(), itemId, MenuHelper::ItemLookup::ById, true);
	EXPECT_TRUE(MenuHelper::IsItemChecked(menu.get(), itemId, MenuHelper::ItemLookup::ById));
}

TEST(MenuHelperTest, TypeFlags)
{
	const UINT itemId = 1;

	auto menu = MenuHelper::CheckedCreatePopupMenu();
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");

	MenuHelper::AddItemTypeFlags(menu.get(), itemId, MenuHelper::ItemLookup::ById, MFT_RADIOCHECK);
	EXPECT_TRUE(
		WI_IsFlagSet(MenuHelper::GetItemTypeFlags(menu.get(), itemId, MenuHelper::ItemLookup::ById),
			MFT_RADIOCHECK));
}

class IsPartOfMenuTest : public Test
{
protected:
	IsPartOfMenuTest()
	{
		UINT idCounter = 1;

		auto nestedSubmenu = MenuHelper::CheckedCreatePopupMenu();
		m_rawNestedSubmenu = nestedSubmenu.get();
		MenuHelper::AddStringItem(nestedSubmenu.get(), idCounter++, L"Nested submenu item");

		auto submenu = MenuHelper::CheckedCreatePopupMenu();
		m_rawSubmenu = submenu.get();
		MenuHelper::AddStringItem(submenu.get(), idCounter++, L"Submenu item");
		MenuHelper::AddSubMenuItem(submenu.get(), idCounter++, L"Nested submenu",
			std::move(nestedSubmenu));

		m_menu = MenuHelper::CheckedCreatePopupMenu();
		MenuHelper::AddStringItem(m_menu.get(), idCounter++, L"Item");
		MenuHelper::AddSubMenuItem(m_menu.get(), idCounter++, L"Submenu", std::move(submenu));
	}

	wil::unique_hmenu m_menu;
	HMENU m_rawSubmenu = nullptr;
	HMENU m_rawNestedSubmenu = nullptr;
};

TEST_F(IsPartOfMenuTest, TopLevelMenu)
{
	EXPECT_TRUE(MenuHelper::IsPartOfMenu(m_menu.get(), m_menu.get()));
}

TEST_F(IsPartOfMenuTest, Submenu)
{
	EXPECT_TRUE(MenuHelper::IsPartOfMenu(m_menu.get(), m_rawSubmenu));
	EXPECT_TRUE(MenuHelper::IsPartOfMenu(m_menu.get(), m_rawNestedSubmenu));
}

TEST_F(IsPartOfMenuTest, SeparateMenu)
{
	auto otherMenu = MenuHelper::CheckedCreatePopupMenu();

	EXPECT_FALSE(MenuHelper::IsPartOfMenu(m_menu.get(), otherMenu.get()));
}
