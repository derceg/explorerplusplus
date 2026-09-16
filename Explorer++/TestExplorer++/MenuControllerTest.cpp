// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuController.h"
#include "MenuDelegate.h"
#include "MenuView.h"
#include "../Helper/MenuHelpTextHost.h"
#include "../Helper/MenuHelper.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

MATCHER_P2(PointEq, x, y, "")
{
	return arg.x == x && arg.y == y;
}

class MenuHelpTextHostFake : public MenuHelpTextHost
{
public:
	void NotifyTopLevelMenuShown() override
	{
	}

	void NotifyTopLevelMenuClosed() override
	{
	}

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

class MenuDelegateMock : public MenuDelegate
{
public:
	MOCK_METHOD(void, OnItemSelected, (UINT id, bool isCtrlKeyDown, bool isShiftKeyDown),
		(override));
	MOCK_METHOD(void, OnItemMiddleClicked, (UINT id, bool isCtrlKeyDown, bool isShiftKeyDown),
		(override));
	MOCK_METHOD(void, OnItemRightClicked, (UINT id, const POINT &ptScreen), (override));
};

}

class MenuControllerTest : public Test
{
protected:
	MenuControllerTest() :
		m_ownedMenu(MenuHelper::CheckedCreatePopupMenu()),
		m_view(m_ownedMenu.get()),
		m_controller(&m_view, &m_menuHelpTextHost)
	{
	}

	MenuHelpTextHostFake m_menuHelpTextHost;
	wil::unique_hmenu m_ownedMenu;
	MenuView m_view;
	MenuController m_controller;
};

TEST_F(MenuControllerTest, Selection)
{
	MenuDelegateMock delegate;
	m_view.SetDelegate(&delegate);

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(itemId1, L"Item 1");

	UINT itemId2 = idCounter++;
	m_view.AppendItem(itemId2, L"Item 2");

	EXPECT_CALL(delegate, OnItemSelected(itemId1, false, false));
	m_controller.SelectItem(itemId1, false, false);

	EXPECT_CALL(delegate, OnItemSelected(itemId2, true, true));
	m_controller.SelectItem(itemId2, true, true);
}

TEST_F(MenuControllerTest, DisabledItemSelection)
{
	MenuDelegateMock delegate;
	m_view.SetDelegate(&delegate);

	UINT itemId1 = 1;
	m_view.AppendItem(itemId1, L"Item 1");
	m_view.EnableItem(itemId1, false);

	// Selecting a disabled item should have no effect.
	EXPECT_CALL(delegate, OnItemSelected(_, _, _)).Times(0);
	m_controller.SelectItem(itemId1, false, false);
}

TEST_F(MenuControllerTest, SelectionWithSubmenus)
{
	MenuDelegateMock delegate;
	m_view.SetDelegate(&delegate);

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(itemId1, L"Item 1");

	auto *subMenuView1 = m_view.AppendSubMenu(idCounter++, L"Submenu 1");

	UINT itemId2 = idCounter++;
	subMenuView1->AppendItem(itemId2, L"Item 2");

	auto *subMenuView2 = subMenuView1->AppendSubMenu(idCounter++, L"Submenu 2");

	UINT itemId3 = idCounter++;
	subMenuView2->AppendItem(itemId3, L"Item 3");

	EXPECT_CALL(delegate, OnItemSelected(itemId1, false, false));
	m_controller.SelectItem(itemId1, false, false);

	EXPECT_CALL(delegate, OnItemSelected(itemId2, false, false));
	m_controller.SelectItem(itemId2, false, false);

	EXPECT_CALL(delegate, OnItemSelected(itemId3, true, true));
	m_controller.SelectItem(itemId3, true, true);
}

TEST_F(MenuControllerTest, SelectionWithSubmenusAndDifferentDelegates)
{
	MenuDelegateMock rootDelegate;
	m_view.SetDelegate(&rootDelegate);

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(itemId1, L"Item 1");

	auto *subMenuView1 = m_view.AppendSubMenu(idCounter++, L"Submenu 1");

	UINT itemId2 = idCounter++;
	subMenuView1->AppendItem(itemId2, L"Item 2");

	auto *subMenuView2 = subMenuView1->AppendSubMenu(idCounter++, L"Submenu 2");

	MenuDelegateMock subMenuView2Delegate;
	subMenuView2->SetDelegate(&subMenuView2Delegate);

	UINT itemId3 = idCounter++;
	subMenuView2->AppendItem(itemId3, L"Item 3");

	auto *subMenuView3 = subMenuView2->AppendSubMenu(idCounter++, L"Submenu 3");

	UINT itemId4 = idCounter++;
	subMenuView3->AppendItem(itemId4, L"Item 4");

	EXPECT_CALL(rootDelegate, OnItemSelected(itemId1, false, false));
	m_controller.SelectItem(itemId1, false, false);

	EXPECT_CALL(rootDelegate, OnItemSelected(itemId2, false, false));
	m_controller.SelectItem(itemId2, false, false);

	EXPECT_CALL(subMenuView2Delegate, OnItemSelected(itemId3, false, false));
	m_controller.SelectItem(itemId3, false, false);

	EXPECT_CALL(subMenuView2Delegate, OnItemSelected(itemId4, false, false));
	m_controller.SelectItem(itemId4, false, false);
}

TEST_F(MenuControllerTest, MiddleClickSelection)
{
	MenuDelegateMock rootDelegate;
	m_view.SetDelegate(&rootDelegate);

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(itemId1, L"Item 1");

	auto *subMenuView = m_view.AppendSubMenu(idCounter++, L"Submenu");

	MenuDelegateMock subMenuViewDelegate;
	subMenuView->SetDelegate(&subMenuViewDelegate);

	UINT itemId2 = idCounter++;
	subMenuView->AppendItem(itemId2, L"Item 2");

	EXPECT_CALL(rootDelegate, OnItemMiddleClicked(itemId1, false, false));
	m_controller.MiddleClickItem(itemId1, false, false);

	EXPECT_CALL(subMenuViewDelegate, OnItemMiddleClicked(itemId2, true, true));
	m_controller.MiddleClickItem(itemId2, true, true);
}

TEST_F(MenuControllerTest, RightClick)
{
	MenuDelegateMock rootDelegate;
	m_view.SetDelegate(&rootDelegate);

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(itemId1, L"Item 1");

	auto *subMenuView = m_view.AppendSubMenu(idCounter++, L"Submenu");

	MenuDelegateMock subMenuViewDelegate;
	subMenuView->SetDelegate(&subMenuViewDelegate);

	UINT itemId2 = idCounter++;
	subMenuView->AppendItem(itemId2, L"Item 2");

	EXPECT_CALL(rootDelegate, OnItemRightClicked(itemId1, PointEq(100, 200)));
	m_controller.RightClickItem(itemId1, { 100, 200 });

	EXPECT_CALL(subMenuViewDelegate, OnItemRightClicked(itemId2, PointEq(50, 270)));
	m_controller.RightClickItem(itemId2, { 50, 270 });
}

TEST_F(MenuControllerTest, HelpTextRequest)
{
	UINT itemId = 1;
	std::wstring helpText = L"Help text";
	m_view.AppendItem(itemId, L"Item", {}, helpText);

	// The menu isn't being shown, so no help text should be returned.
	auto retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(m_view.GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);

	m_controller.NotifyMenuWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	// The menu is now being shown, so help text should be returned.
	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(m_view.GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, helpText);

	m_controller.NotifyMenuClosedForTesting();

	// The menu has been closed, so, again, no help text should be returned.
	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(m_view.GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);
}

TEST_F(MenuControllerTest, SubMenuHelpTextRequest)
{
	UINT idCounter = 1;

	auto *subMenuView = m_view.AppendSubMenu(idCounter++, L"SubMenu");

	UINT itemId = idCounter++;
	std::wstring helpText = L"Help text";
	subMenuView->AppendItem(itemId, L"SubMenu Item", {}, helpText);

	auto retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(subMenuView->GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);

	m_controller.NotifyMenuWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(subMenuView->GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, helpText);

	m_controller.NotifyMenuClosedForTesting();

	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(subMenuView->GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);
}
