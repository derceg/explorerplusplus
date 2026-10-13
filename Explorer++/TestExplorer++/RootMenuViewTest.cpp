// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RootMenuView.h"
#include "MenuHelpTextHostFake.h"
#include "MenuView.h"
#include "MenuViewTestApi.h"
#include "RootMenuViewTestApi.h"
#include "TestMenuView.h"
#include "../Helper/MenuHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{

MATCHER_P2(PointEq, x, y, "")
{
	return arg.x == x && arg.y == y;
}

class ExternalMenuItemDelegateMock : public ExternalMenuItemDelegate
{
public:
	MOCK_METHOD(std::optional<LRESULT>, MaybeHandleMessage,
		(UINT msg, WPARAM wParam, LPARAM lParam), (override));
	MOCK_METHOD(std::optional<std::wstring>, MaybeGetExternalItemHelpText, (UINT id),
		(const, override));
	MOCK_METHOD(void, OnExternalItemSelected, (UINT id), (override));
};

}

class RootMenuViewTest : public Test
{
protected:
	RootMenuViewTest() :
		m_ownerWindow(CreateOwnerWindow()),
		m_view(m_ownerWindow.get(), &m_menuHelpTextHost)
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
		info.hmenu = MenuViewTestApi::GetNativeMenu(&m_view);
		info.riid = &iid;
		SendMessage(hwnd, WM_MENUGETOBJECT, 0, reinterpret_cast<LPARAM>(&info));
	}

	MenuHelpTextHostFake m_menuHelpTextHost;
	wil::unique_hwnd m_ownerWindow;
	TestMenuView m_view;
};

TEST_F(RootMenuViewTest, Selection)
{
	MockFunction<void()> selectedCallback1;
	MockFunction<void()> selectedCallback2;
	m_view.AppendItem(
		MenuItemParams(L"Item 1").SetSelectedCallback(selectedCallback1.AsStdFunction()));
	m_view.AppendItem(
		MenuItemParams(L"Item 2").SetSelectedCallback(selectedCallback2.AsStdFunction()));

	EXPECT_CALL(selectedCallback1, Call());
	RootMenuViewTestApi::SelectItemAtIndex(&m_view, 0, false, false);

	EXPECT_CALL(selectedCallback2, Call());
	RootMenuViewTestApi::SelectItemAtIndex(&m_view, 1, false, false);
}

TEST_F(RootMenuViewTest, DisabledItemSelection)
{
	MockFunction<void()> selectedCallback;
	m_view.AppendItem(MenuItemParams(L"Item 1")
			.SetEnabledCallback([] { return false; })
			.SetSelectedCallback(selectedCallback.AsStdFunction()));

	// Selecting a disabled item should have no effect.
	EXPECT_CALL(selectedCallback, Call()).Times(0);
	RootMenuViewTestApi::SelectItemAtIndex(&m_view, 0, false, false);
}

TEST_F(RootMenuViewTest, SelectionWithSubmenus)
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
	RootMenuViewTestApi::SelectItemAtIndex(&m_view, 0, false, false);

	EXPECT_CALL(selectedCallback2, Call());
	RootMenuViewTestApi::SelectItem(&m_view, MenuViewTestApi::GetItemId(subMenuView1, 0), false,
		false);

	EXPECT_CALL(selectedCallback3, Call());
	RootMenuViewTestApi::SelectItem(&m_view, MenuViewTestApi::GetItemId(subMenuView2, 0), false,
		false);
}

TEST_F(RootMenuViewTest, ExternalItemSelection)
{
	ExternalMenuItemDelegateMock externalItemDelegate;
	m_view.SetExternalItemDelegate(&externalItemDelegate);

	HMENU menu = m_view.GetNativeMenuForExternalItems();

	UINT idCounter = MenuView::EXTERNAL_ID_MIN;

	UINT itemId1 = idCounter++;
	MenuHelper::AddStringItem(menu, itemId1, L"External item 1");

	UINT itemId2 = idCounter++;
	MenuHelper::AddStringItem(menu, itemId2, L"External item 2");

	InSequence seq;

	EXPECT_CALL(externalItemDelegate, OnExternalItemSelected(itemId1));
	EXPECT_CALL(externalItemDelegate, OnExternalItemSelected(itemId2));

	RootMenuViewTestApi::SelectItem(&m_view, itemId1, false, false);
	RootMenuViewTestApi::SelectItem(&m_view, itemId2, false, false);
}

TEST_F(RootMenuViewTest, MiddleClickSelection)
{
	MockFunction<void()> middleClickCallback1;
	m_view.AppendItem(
		MenuItemParams(L"Item 1").SetMiddleClickedCallback(middleClickCallback1.AsStdFunction()));

	auto *subMenuView = m_view.AppendSubMenu(SubMenuItemParams(L"Submenu"));

	MockFunction<void()> middleClickCallback2;
	subMenuView->AppendItem(
		MenuItemParams(L"Item 2").SetMiddleClickedCallback(middleClickCallback2.AsStdFunction()));

	EXPECT_CALL(middleClickCallback1, Call());
	RootMenuViewTestApi::MiddleClickItemAtIndex(&m_view, 0, false, false);

	EXPECT_CALL(middleClickCallback2, Call());
	RootMenuViewTestApi::MiddleClickItem(&m_view, MenuViewTestApi::GetItemId(subMenuView, 0), true,
		true);
}

TEST_F(RootMenuViewTest, RightClick)
{
	MockFunction<void(const POINT &ptScreen)> rightClickCallback1;
	m_view.AppendItem(
		MenuItemParams(L"Item 1").SetRightClickedCallback(rightClickCallback1.AsStdFunction()));

	auto *subMenuView = m_view.AppendSubMenu(SubMenuItemParams(L"Submenu"));

	MockFunction<void(const POINT &ptScreen)> rightClickCallback2;
	subMenuView->AppendItem(
		MenuItemParams(L"Item 2").SetRightClickedCallback(rightClickCallback2.AsStdFunction()));

	EXPECT_CALL(rightClickCallback1, Call(PointEq(100, 200)));
	RootMenuViewTestApi::RightClickItemAtIndex(&m_view, 0, { 100, 200 });

	EXPECT_CALL(rightClickCallback2, Call(PointEq(50, 270)));
	RootMenuViewTestApi::RightClickItem(&m_view, MenuViewTestApi::GetItemId(subMenuView, 0),
		{ 50, 270 });
}

TEST_F(RootMenuViewTest, HelpTextRequest)
{
	std::wstring helpText = L"Help text";
	m_view.AppendItem(MenuItemParams(L"Item").SetHelpText(helpText));

	UINT itemId = MenuViewTestApi::GetItemId(&m_view, 0);

	// The menu isn't being shown, so no help text should be returned.
	auto retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(MenuViewTestApi::GetNativeMenu(&m_view), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);

	RootMenuViewTestApi::NotifyMenuOpened(&m_view);

	// The menu is now being shown, so help text should be returned.
	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(MenuViewTestApi::GetNativeMenu(&m_view), itemId);
	EXPECT_EQ(retrievedHelpText, helpText);

	RootMenuViewTestApi::NotifyMenuClosed(&m_view);

	// The menu has been closed, so, again, no help text should be returned.
	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(MenuViewTestApi::GetNativeMenu(&m_view), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);
}

TEST_F(RootMenuViewTest, SubMenuHelpTextRequest)
{
	auto *subMenuView = m_view.AppendSubMenu(SubMenuItemParams(L"SubMenu"));

	std::wstring helpText = L"Help text";
	subMenuView->AppendItem(MenuItemParams(L"SubMenu Item").SetHelpText(helpText));

	UINT itemId = MenuViewTestApi::GetItemId(subMenuView, 0);

	auto retrievedHelpText = m_menuHelpTextHost.TriggerHelpTextRequest(
		MenuViewTestApi::GetNativeMenu(subMenuView), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);

	RootMenuViewTestApi::NotifyMenuOpened(&m_view);

	retrievedHelpText = m_menuHelpTextHost.TriggerHelpTextRequest(
		MenuViewTestApi::GetNativeMenu(subMenuView), itemId);
	EXPECT_EQ(retrievedHelpText, helpText);

	RootMenuViewTestApi::NotifyMenuClosed(&m_view);

	retrievedHelpText = m_menuHelpTextHost.TriggerHelpTextRequest(
		MenuViewTestApi::GetNativeMenu(subMenuView), itemId);
	EXPECT_EQ(retrievedHelpText, std::nullopt);
}

TEST_F(RootMenuViewTest, ExternalItemHelpText)
{
	ExternalMenuItemDelegateMock externalItemDelegate;
	m_view.SetExternalItemDelegate(&externalItemDelegate);

	HMENU menu = m_view.GetNativeMenuForExternalItems();

	UINT idCounter = MenuView::EXTERNAL_ID_MIN;

	UINT itemId1 = idCounter++;
	MenuHelper::AddStringItem(menu, itemId1, L"External item 1");

	UINT itemId2 = idCounter++;
	MenuHelper::AddStringItem(menu, itemId2, L"External item 2");

	EXPECT_CALL(externalItemDelegate, MaybeGetExternalItemHelpText(itemId1))
		.WillOnce(Return(std::wstring(L"Help for external item 1")));
	EXPECT_CALL(externalItemDelegate, MaybeGetExternalItemHelpText(itemId2))
		.WillOnce(Return(std::wstring(L"Help for external item 2")));

	RootMenuViewTestApi::NotifyMenuOpened(&m_view);

	auto retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(MenuViewTestApi::GetNativeMenu(&m_view), itemId1);
	EXPECT_EQ(retrievedHelpText, L"Help for external item 1");

	retrievedHelpText =
		m_menuHelpTextHost.TriggerHelpTextRequest(MenuViewTestApi::GetNativeMenu(&m_view), itemId2);
	EXPECT_EQ(retrievedHelpText, L"Help for external item 2");

	RootMenuViewTestApi::NotifyMenuClosed(&m_view);
}

TEST_F(RootMenuViewTest, DropLocation)
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
