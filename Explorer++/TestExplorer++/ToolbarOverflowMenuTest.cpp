// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ToolbarOverflowMenu.h"
#include "AcceleratorManager.h"
#include "MenuViewTestApi.h"
#include "RootMenuViewTestApi.h"
#include "TestMenuView.h"
#include "TestToolbarButton.h"
#include "ToolbarView.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

class TestMenu : public MenuBase
{
public:
	TestMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager) :
		MenuBase(menuView, acceleratorManager)
	{
		m_rootMenuView->AppendItem(MenuItemParams(L"Test dropdown item"));
	}
};

class TestToolbarDropDownButton : public TestToolbarButton
{
public:
	TestToolbarDropDownButton(const AcceleratorManager *acceleratorManager) :
		m_acceleratorManager(acceleratorManager)
	{
	}

	bool IsDropDownButton() const override
	{
		return true;
	}

	MenuFactory MaybeGetMenuFactory() const override
	{
		return [this](MenuView *menuView)
		{
			return std::make_unique<TestMenu>(menuView, m_acceleratorManager);
		};
	}

private:
	const AcceleratorManager *const m_acceleratorManager;
};

}

class ToolbarOverflowMenuTest : public Test
{
protected:
	AcceleratorManager m_acceleratorManager;
};

TEST_F(ToolbarOverflowMenuTest, CheckItems)
{
	TestToolbarButton button1;
	TestToolbarButton button2;
	TestToolbarButton button3;

	button2.SetEnabled(false);
	button3.SetChecked(true);

	std::vector<ToolbarItem> items = {
		ToolbarItem::Button(&button1),
		ToolbarItem::Separator(),
		ToolbarItem::Button(&button2),
		ToolbarItem::Separator(),
		ToolbarItem::Button(&button3),
	};

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager, items, {});

	ASSERT_EQ(static_cast<size_t>(menuView.GetNumItems()), items.size());

	for (int i = 0; i < static_cast<int>(items.size()); i++)
	{
		const auto &item = items[i];

		if (item.IsButton())
		{
			const auto *button = item.GetButton();
			UINT id = MenuViewTestApi::GetItemId(&menuView, i);
			EXPECT_EQ(MenuViewTestApi::GetItemFullText(&menuView, id), button->GetText());
			EXPECT_EQ(MenuViewTestApi::GetItemHelpText(&menuView, id), button->GetTooltipText());
			EXPECT_EQ(MenuViewTestApi::IsItemEnabled(&menuView, id), button->IsEnabled());
			EXPECT_EQ(MenuViewTestApi::IsItemChecked(&menuView, id), button->IsChecked());
		}
		else
		{
			EXPECT_TRUE(MenuViewTestApi::IsItemASeparator(&menuView, i));
		}
	}
}

TEST_F(ToolbarOverflowMenuTest, SubMenuItem)
{
	TestToolbarDropDownButton button(&m_acceleratorManager);

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager, { ToolbarItem::Button(&button) },
		{});

	UINT id = MenuViewTestApi::GetItemId(&menuView, 0);
	EXPECT_EQ(MenuViewTestApi::GetItemFullText(&menuView, id), button.GetText());

	auto *subMenuView = MenuViewTestApi::GetSubMenuView(&menuView, id);
	ASSERT_EQ(subMenuView->GetNumItems(), 1);
	EXPECT_EQ(
		MenuViewTestApi::GetItemFullText(subMenuView, MenuViewTestApi::GetItemId(subMenuView, 0)),
		L"Test dropdown item");
}

TEST_F(ToolbarOverflowMenuTest, Selection)
{
	MockFunction<void(const MouseEvent &event)> clickCallback;
	TestToolbarButton button(clickCallback.AsStdFunction());

	MockFunction<void(const MouseEvent &event)> middleClickCallback;
	button.SetMiddleClickedCallback(middleClickCallback.AsStdFunction());

	MockFunction<void(const MouseEvent &event)> rightClickCallback;
	button.SetRightClickedCallback(rightClickCallback.AsStdFunction());

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager, { ToolbarItem::Button(&button) },
		{});

	EXPECT_CALL(clickCallback, Call(MouseEvent{ { 0, 0 }, false, false }));
	RootMenuViewTestApi::SelectItemAtIndex(&menuView, 0, false, false);

	EXPECT_CALL(middleClickCallback, Call(MouseEvent{ { 0, 0 }, false, false }));
	RootMenuViewTestApi::MiddleClickItemAtIndex(&menuView, 0, false, false);

	EXPECT_CALL(rightClickCallback,
		Call(Field(&MouseEvent::ptScreen, AllOf(Field(&POINT::x, 12), Field(&POINT::y, 34)))));
	RootMenuViewTestApi::RightClickItemAtIndex(&menuView, 0, { 12, 34 });
}

TEST_F(ToolbarOverflowMenuTest, SelectionAfterButtonDestruction)
{
	MockFunction<void(const MouseEvent &event)> clickCallback;
	auto button = std::make_unique<TestToolbarButton>(clickCallback.AsStdFunction());

	MockFunction<void(const MouseEvent &event)> middleClickCallback;
	button->SetMiddleClickedCallback(middleClickCallback.AsStdFunction());

	MockFunction<void(const MouseEvent &event)> rightClickCallback;
	button->SetRightClickedCallback(rightClickCallback.AsStdFunction());

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager,
		{ ToolbarItem::Button(button.get()) }, {});

	button.reset();

	// If the underlying button has been removed, selecting the associated menu item should have no
	// effect.
	EXPECT_CALL(clickCallback, Call(_)).Times(0);
	RootMenuViewTestApi::SelectItemAtIndex(&menuView, 0, false, false);

	EXPECT_CALL(middleClickCallback, Call(_)).Times(0);
	RootMenuViewTestApi::MiddleClickItemAtIndex(&menuView, 0, false, false);

	EXPECT_CALL(rightClickCallback, Call(_)).Times(0);
	RootMenuViewTestApi::RightClickItemAtIndex(&menuView, 0, { 0, 0 });
}

TEST_F(ToolbarOverflowMenuTest, SubMenuItemEvents)
{
	TestToolbarDropDownButton button(&m_acceleratorManager);

	MockFunction<void(const MouseEvent &event)> middleClickCallback;
	button.SetMiddleClickedCallback(middleClickCallback.AsStdFunction());

	MockFunction<void(const MouseEvent &event)> rightClickCallback;
	button.SetRightClickedCallback(rightClickCallback.AsStdFunction());

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager, { ToolbarItem::Button(&button) },
		{});

	EXPECT_CALL(middleClickCallback, Call(MouseEvent{ { 0, 0 }, false, false }));
	RootMenuViewTestApi::MiddleClickItemAtIndex(&menuView, 0, false, false);

	EXPECT_CALL(rightClickCallback,
		Call(Field(&MouseEvent::ptScreen, AllOf(Field(&POINT::x, 10), Field(&POINT::y, 45)))));
	RootMenuViewTestApi::RightClickItemAtIndex(&menuView, 0, { 10, 45 });
}
