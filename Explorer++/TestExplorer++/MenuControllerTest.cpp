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

MATCHER_P2(IsGapBetween, beforeId, afterId, "")
{
	return (arg.id == beforeId && arg.position == MenuDropLocation::Position::After)
		|| (arg.id == afterId && arg.position == MenuDropLocation::Position::Before);
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
	MOCK_METHOD(wil::com_ptr_nothrow<IDropTarget>, MaybeGetDropTargetForLocation,
		(const MenuDropLocation &dropLocation), (override));
};

}

class MenuControllerTest : public Test
{
protected:
	MenuControllerTest() :
		m_ownerWindow(CreateOwnerWindow()),
		m_ownedMenu(MenuHelper::CheckedCreatePopupMenu()),
		m_view(m_ownedMenu.get()),
		m_controller(&m_view, m_ownerWindow.get(), &m_menuHelpTextHost)
	{
	}

	wil::unique_hwnd CreateOwnerWindow()
	{
		wil::unique_hwnd hwnd(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		CHECK(hwnd);
		return hwnd;
	}

	void SendMenuGetObject(HWND hwnd, DWORD flags, UINT index)
	{
		IID iid = IID_IDropTarget;

		MENUGETOBJECTINFO info = {};
		info.dwFlags = flags;
		info.uPos = index;
		info.hmenu = m_view.GetNativeMenuForTesting();
		info.riid = &iid;
		SendMessage(hwnd, WM_MENUGETOBJECT, 0, reinterpret_cast<LPARAM>(&info));
	}

	MenuHelpTextHostFake m_menuHelpTextHost;
	wil::unique_hwnd m_ownerWindow;
	wil::unique_hmenu m_ownedMenu;
	MenuView m_view;
	MenuController m_controller;
};

TEST_F(MenuControllerTest, Selection)
{
	MenuDelegateMock delegate;

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(&delegate, itemId1, L"Item 1");

	UINT itemId2 = idCounter++;
	m_view.AppendItem(&delegate, itemId2, L"Item 2");

	EXPECT_CALL(delegate, OnItemSelected(itemId1, false, false));
	m_controller.SelectItem(itemId1, false, false);

	EXPECT_CALL(delegate, OnItemSelected(itemId2, true, true));
	m_controller.SelectItem(itemId2, true, true);
}

TEST_F(MenuControllerTest, DisabledItemSelection)
{
	MenuDelegateMock delegate;

	UINT itemId1 = 1;
	m_view.AppendItem(&delegate, itemId1, L"Item 1");
	m_view.EnableItem(itemId1, false);

	// Selecting a disabled item should have no effect.
	EXPECT_CALL(delegate, OnItemSelected(_, _, _)).Times(0);
	m_controller.SelectItem(itemId1, false, false);
}

TEST_F(MenuControllerTest, SelectionWithSubmenus)
{
	MenuDelegateMock delegate;

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(&delegate, itemId1, L"Item 1");

	auto *subMenuView1 = m_view.AppendSubMenu(&delegate, idCounter++, L"Submenu 1");

	UINT itemId2 = idCounter++;
	subMenuView1->AppendItem(&delegate, itemId2, L"Item 2");

	auto *subMenuView2 = subMenuView1->AppendSubMenu(&delegate, idCounter++, L"Submenu 2");

	UINT itemId3 = idCounter++;
	subMenuView2->AppendItem(&delegate, itemId3, L"Item 3");

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

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(&rootDelegate, itemId1, L"Item 1");

	auto *subMenuView1 = m_view.AppendSubMenu(&rootDelegate, idCounter++, L"Submenu 1");

	UINT itemId2 = idCounter++;
	subMenuView1->AppendItem(&rootDelegate, itemId2, L"Item 2");

	auto *subMenuView2 = subMenuView1->AppendSubMenu(&rootDelegate, idCounter++, L"Submenu 2");

	MenuDelegateMock subMenuView2Delegate;

	UINT itemId3 = idCounter++;
	subMenuView2->AppendItem(&subMenuView2Delegate, itemId3, L"Item 3");

	auto *subMenuView3 =
		subMenuView2->AppendSubMenu(&subMenuView2Delegate, idCounter++, L"Submenu 3");

	UINT itemId4 = idCounter++;
	subMenuView3->AppendItem(&subMenuView2Delegate, itemId4, L"Item 4");

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

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(&rootDelegate, itemId1, L"Item 1");

	auto *subMenuView = m_view.AppendSubMenu(&rootDelegate, idCounter++, L"Submenu");

	MenuDelegateMock subMenuViewDelegate;

	UINT itemId2 = idCounter++;
	subMenuView->AppendItem(&subMenuViewDelegate, itemId2, L"Item 2");

	EXPECT_CALL(rootDelegate, OnItemMiddleClicked(itemId1, false, false));
	m_controller.MiddleClickItem(itemId1, false, false);

	EXPECT_CALL(subMenuViewDelegate, OnItemMiddleClicked(itemId2, true, true));
	m_controller.MiddleClickItem(itemId2, true, true);
}

TEST_F(MenuControllerTest, RightClick)
{
	MenuDelegateMock rootDelegate;

	UINT idCounter = 1;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(&rootDelegate, itemId1, L"Item 1");

	auto *subMenuView = m_view.AppendSubMenu(&rootDelegate, idCounter++, L"Submenu");

	MenuDelegateMock subMenuViewDelegate;

	UINT itemId2 = idCounter++;
	subMenuView->AppendItem(&subMenuViewDelegate, itemId2, L"Item 2");

	EXPECT_CALL(rootDelegate, OnItemRightClicked(itemId1, PointEq(100, 200)));
	m_controller.RightClickItem(itemId1, { 100, 200 });

	EXPECT_CALL(subMenuViewDelegate, OnItemRightClicked(itemId2, PointEq(50, 270)));
	m_controller.RightClickItem(itemId2, { 50, 270 });
}

TEST_F(MenuControllerTest, HelpTextRequest)
{
	UINT itemId = 1;
	std::wstring helpText = L"Help text";
	m_view.AppendItem(nullptr, itemId, L"Item", {}, helpText);

	// The menu isn't being shown, so no help text should be returned.
	auto retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(m_view.GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);

	m_controller.NotifyMenuOpenedForTesting();

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

	auto *subMenuView = m_view.AppendSubMenu(nullptr, idCounter++, L"SubMenu");

	UINT itemId = idCounter++;
	std::wstring helpText = L"Help text";
	subMenuView->AppendItem(nullptr, itemId, L"SubMenu Item", {}, helpText);

	auto retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(subMenuView->GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);

	m_controller.NotifyMenuOpenedForTesting();

	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(subMenuView->GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, helpText);

	m_controller.NotifyMenuClosedForTesting();

	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(subMenuView->GetNativeMenuForTesting(), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);
}

TEST_F(MenuControllerTest, DropLocation)
{
	MenuDelegateMock delegate;

	UINT idCounter = 100;

	UINT itemId1 = idCounter++;
	m_view.AppendItem(&delegate, itemId1, L"Item 1");

	UINT itemId2 = idCounter++;
	m_view.AppendItem(&delegate, itemId2, L"Item 2");

	UINT itemId3 = idCounter++;
	m_view.AppendItem(&delegate, itemId3, L"Item 3");

	InSequence seq;

	EXPECT_CALL(delegate,
		MaybeGetDropTargetForLocation(MenuDropLocation{ itemId2, MenuDropLocation::Position::On }));
	SendMenuGetObject(m_ownerWindow.get(), 0, 1);

	EXPECT_CALL(delegate,
		MaybeGetDropTargetForLocation(
			MenuDropLocation{ itemId1, MenuDropLocation::Position::Before }));
	SendMenuGetObject(m_ownerWindow.get(), MNGOF_TOPGAP, 0);

	EXPECT_CALL(delegate, MaybeGetDropTargetForLocation(IsGapBetween(itemId2, itemId3)));
	SendMenuGetObject(m_ownerWindow.get(), MNGOF_TOPGAP, 2);

	EXPECT_CALL(delegate,
		MaybeGetDropTargetForLocation(
			MenuDropLocation{ itemId3, MenuDropLocation::Position::After }));
	SendMenuGetObject(m_ownerWindow.get(), MNGOF_TOPGAP, 3);
}
