// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuController.h"
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

	void MenuItemSelected(HMENU menu, WORD indexOrId, UINT flags) override
	{
		UNREFERENCED_PARAMETER(menu);
		UNREFERENCED_PARAMETER(indexOrId);
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
	MockFunction<void()> selectedCallback1;
	MockFunction<void()> selectedCallback2;
	m_view.AppendItem(
		MenuItemParams(L"Item 1").SetSelectedCallback(selectedCallback1.AsStdFunction()));
	m_view.AppendItem(
		MenuItemParams(L"Item 2").SetSelectedCallback(selectedCallback2.AsStdFunction()));

	EXPECT_CALL(selectedCallback1, Call());
	m_controller.SelectItem(m_view.GetItemIdForTesting(0), false, false);

	EXPECT_CALL(selectedCallback2, Call());
	m_controller.SelectItem(m_view.GetItemIdForTesting(1), false, false);
}

TEST_F(MenuControllerTest, DisabledItemSelection)
{
	MockFunction<void()> selectedCallback;
	m_view.AppendItem(MenuItemParams(L"Item 1")
			.SetEnabledCallback([] { return false; })
			.SetSelectedCallback(selectedCallback.AsStdFunction()));

	// Selecting a disabled item should have no effect.
	EXPECT_CALL(selectedCallback, Call()).Times(0);
	m_controller.SelectItem(m_view.GetItemIdForTesting(0), false, false);
}

TEST_F(MenuControllerTest, SelectionWithSubmenus)
{
	MockFunction<void()> selectedCallback1;
	m_view.AppendItem(
		MenuItemParams(L"Item 1").SetSelectedCallback(selectedCallback1.AsStdFunction()));

	auto *subMenuView1 = m_view.AppendSubMenu(SubMenuItemParams(L"Submenu 1"));

	MockFunction<void()> selectedCallback2;
	subMenuView1->AppendItem(
		MenuItemParams(L"Item 2").SetSelectedCallback(selectedCallback2.AsStdFunction()));

	auto *subMenuView2 = subMenuView1->AppendSubMenu(SubMenuItemParams(L"Submenu 2"));

	MockFunction<void()> selectedCallback3;
	subMenuView2->AppendItem(
		MenuItemParams(L"Item 3").SetSelectedCallback(selectedCallback3.AsStdFunction()));

	EXPECT_CALL(selectedCallback1, Call());
	m_controller.SelectItem(m_view.GetItemIdForTesting(0), false, false);

	EXPECT_CALL(selectedCallback2, Call());
	m_controller.SelectItem(subMenuView1->GetItemIdForTesting(0), false, false);

	EXPECT_CALL(selectedCallback3, Call());
	m_controller.SelectItem(subMenuView2->GetItemIdForTesting(0), false, false);
}

TEST_F(MenuControllerTest, MiddleClickSelection)
{
	MockFunction<void()> middleClickCallback1;
	m_view.AppendItem(
		MenuItemParams(L"Item 1").SetMiddleClickedCallback(middleClickCallback1.AsStdFunction()));

	auto *subMenuView = m_view.AppendSubMenu(SubMenuItemParams(L"Submenu"));

	MockFunction<void()> middleClickCallback2;
	subMenuView->AppendItem(
		MenuItemParams(L"Item 2").SetMiddleClickedCallback(middleClickCallback2.AsStdFunction()));

	EXPECT_CALL(middleClickCallback1, Call());
	m_controller.MiddleClickItem(m_view.GetItemIdForTesting(0), false, false);

	EXPECT_CALL(middleClickCallback2, Call());
	m_controller.MiddleClickItem(subMenuView->GetItemIdForTesting(0), true, true);
}

TEST_F(MenuControllerTest, RightClick)
{
	MockFunction<void(const POINT &ptScreen)> rightClickCallback1;
	m_view.AppendItem(
		MenuItemParams(L"Item 1").SetRightClickedCallback(rightClickCallback1.AsStdFunction()));

	auto *subMenuView = m_view.AppendSubMenu(SubMenuItemParams(L"Submenu"));

	MockFunction<void(const POINT &ptScreen)> rightClickCallback2;
	subMenuView->AppendItem(
		MenuItemParams(L"Item 2").SetRightClickedCallback(rightClickCallback2.AsStdFunction()));

	EXPECT_CALL(rightClickCallback1, Call(PointEq(100, 200)));
	m_controller.RightClickItem(m_view.GetItemIdForTesting(0), { 100, 200 });

	EXPECT_CALL(rightClickCallback2, Call(PointEq(50, 270)));
	m_controller.RightClickItem(subMenuView->GetItemIdForTesting(0), { 50, 270 });
}

TEST_F(MenuControllerTest, HelpTextRequest)
{
	std::wstring helpText = L"Help text";
	m_view.AppendItem(MenuItemParams(L"Item").SetHelpText(helpText));

	UINT itemId = m_view.GetItemIdForTesting(0);

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
	auto *subMenuView = m_view.AppendSubMenu(SubMenuItemParams(L"SubMenu"));

	std::wstring helpText = L"Help text";
	subMenuView->AppendItem(MenuItemParams(L"SubMenu Item").SetHelpText(helpText));

	UINT itemId = subMenuView->GetItemIdForTesting(0);

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
	MockFunction<wil::com_ptr_nothrow<IDropTarget>(MenuDropPosition position)> dropTargetCallback1;
	MockFunction<wil::com_ptr_nothrow<IDropTarget>(MenuDropPosition position)> dropTargetCallback2;
	MockFunction<wil::com_ptr_nothrow<IDropTarget>(MenuDropPosition position)> dropTargetCallback3;
	m_view.AppendItem(MenuItemParams(L"Item 1").SetMaybeGetDropTargetCallback(
		dropTargetCallback1.AsStdFunction()));
	m_view.AppendItem(MenuItemParams(L"Item 2").SetMaybeGetDropTargetCallback(
		dropTargetCallback2.AsStdFunction()));
	m_view.AppendItem(MenuItemParams(L"Item 3").SetMaybeGetDropTargetCallback(
		dropTargetCallback3.AsStdFunction()));

	InSequence seq;

	EXPECT_CALL(dropTargetCallback2, Call(MenuDropPosition::On));
	SendMenuGetObject(m_ownerWindow.get(), 0, 1);

	EXPECT_CALL(dropTargetCallback1, Call(MenuDropPosition::Before));
	SendMenuGetObject(m_ownerWindow.get(), MNGOF_TOPGAP, 0);

	EXPECT_CALL(dropTargetCallback3, Call(MenuDropPosition::Before));
	SendMenuGetObject(m_ownerWindow.get(), MNGOF_TOPGAP, 2);

	EXPECT_CALL(dropTargetCallback3, Call(MenuDropPosition::After));
	SendMenuGetObject(m_ownerWindow.get(), MNGOF_TOPGAP, 3);
}
