// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/MenuHelper.h"
#include <gtest/gtest.h>
#include <wil/resource.h>

using namespace testing;

TEST(RemoveDuplicateSeperatorsTest, RemoveDuplicates)
{
	const UINT item1Id = 1;
	const UINT item2Id = 2;

	wil::unique_hmenu menu(CreatePopupMenu());
	MenuHelper::AddStringItem(menu.get(), item1Id, L"Item 1");
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddStringItem(menu.get(), item2Id, L"Item 2");

	// This should leave a single separator, along with the two other menu items.
	MenuHelper::RemoveDuplicateSeperators(menu.get());
	EXPECT_EQ(GetMenuItemCount(menu.get()), 3);
	EXPECT_EQ(GetMenuItemID(menu.get(), 0), item1Id);
	EXPECT_EQ(GetMenuItemID(menu.get(), 2), item2Id);
}

TEST(RemoveTrailingSeparatorsTest, RemoveTrailing)
{
	const UINT itemId = 1;

	wil::unique_hmenu menu(CreatePopupMenu());
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());
	MenuHelper::AddSeparator(menu.get());

	MenuHelper::RemoveTrailingSeparators(menu.get());
	EXPECT_EQ(GetMenuItemCount(menu.get()), 1);
	EXPECT_EQ(GetMenuItemID(menu.get(), 0), itemId);
}

TEST(FindParentMenuTest, FindSubItem)
{
	UINT idCounter = 1;

	wil::unique_hmenu nestedSubmenu(CreatePopupMenu());
	auto rawNestedSubmenu = nestedSubmenu.get();
	auto nestedItemId = idCounter++;
	MenuHelper::AddStringItem(nestedSubmenu.get(), nestedItemId, L"Nested item");

	wil::unique_hmenu submenu(CreatePopupMenu());
	MenuHelper::AddSubMenuItem(submenu.get(), idCounter++, L"Nested submenu",
		std::move(nestedSubmenu));

	wil::unique_hmenu menu(CreatePopupMenu());
	MenuHelper::AddSubMenuItem(menu.get(), idCounter++, L"Submenu", std::move(submenu));

	EXPECT_EQ(MenuHelper::FindParentMenu(menu.get(), nestedItemId), rawNestedSubmenu);
}

TEST(GetMenuItemStringTest, GetString)
{
	const UINT itemId = 1;

	wil::unique_hmenu menu(CreatePopupMenu());
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");

	auto text = MenuHelper::GetMenuItemString(menu.get(), itemId, false);
	EXPECT_EQ(text, L"Item");
}

TEST(GetMenuItemIDIncludingSubmenuTest, GetId)
{
	const UINT itemId = 1;
	const UINT submenuItemId = 2;

	wil::unique_hmenu submenu(CreatePopupMenu());

	wil::unique_hmenu menu(CreatePopupMenu());
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");
	MenuHelper::AddSubMenuItem(menu.get(), submenuItemId, L"Submenu", std::move(submenu));

	// Unlike GetMenuItemID(), this function should return the ID set for both items and submenu
	// items.
	EXPECT_EQ(MenuHelper::GetMenuItemIDIncludingSubmenu(menu.get(), 0), itemId);
	EXPECT_EQ(MenuHelper::GetMenuItemIDIncludingSubmenu(menu.get(), 1), submenuItemId);
}

class IsPartOfMenuTest : public Test
{
protected:
	IsPartOfMenuTest()
	{
		UINT idCounter = 1;

		wil::unique_hmenu nestedSubmenu(CreatePopupMenu());
		m_rawNestedSubmenu = nestedSubmenu.get();
		MenuHelper::AddStringItem(nestedSubmenu.get(), idCounter++, L"Nested submenu item");

		wil::unique_hmenu submenu(CreatePopupMenu());
		m_rawSubmenu = submenu.get();
		MenuHelper::AddStringItem(submenu.get(), idCounter++, L"Submenu item");
		MenuHelper::AddSubMenuItem(submenu.get(), idCounter++, L"Nested submenu",
			std::move(nestedSubmenu));

		m_menu.reset(CreatePopupMenu());
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
	wil::unique_hmenu otherMenu(CreatePopupMenu());

	EXPECT_FALSE(MenuHelper::IsPartOfMenu(m_menu.get(), otherMenu.get()));
}

TEST(IsMenuItemEnabledTest, CheckEnabled)
{
	const UINT itemId = 1;

	wil::unique_hmenu menu(CreatePopupMenu());
	MenuHelper::AddStringItem(menu.get(), itemId, L"Item");
	EXPECT_TRUE(MenuHelper::IsMenuItemEnabled(menu.get(), itemId, false));

	MenuHelper::EnableItem(menu.get(), itemId, false);
	EXPECT_FALSE(MenuHelper::IsMenuItemEnabled(menu.get(), itemId, false));
}
