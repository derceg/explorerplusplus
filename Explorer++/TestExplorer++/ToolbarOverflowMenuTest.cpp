// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ToolbarOverflowMenu.h"
#include "AcceleratorManager.h"
#include "MenuViewTestApi.h"
#include "RootMenuViewTestApi.h"
#include "TestMenuView.h"
#include "ToolbarView.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

class TestToolbarButton : public ToolbarButton
{
public:
	TestToolbarButton(MouseEventCallback clickedCallback = {}) :
		ToolbarButton(std::move(clickedCallback)),
		m_index(m_indexCounter++)
	{
	}

	std::wstring GetText() const override
	{
		return L"Test button " + std::to_wstring(m_index);
	}

	std::wstring GetTooltipText() const override
	{
		return L"Help text " + std::to_wstring(m_index);
	}

	std::optional<int> GetImageIndex() const override
	{
		return std::nullopt;
	}

private:
	static inline int m_indexCounter = 0;
	const int m_index;
};

}

class ToolbarOverflowMenuTest : public Test
{
protected:
	AcceleratorManager m_acceleratorManager;
};

TEST_F(ToolbarOverflowMenuTest, CheckItems)
{
	std::vector<TestToolbarButton> ownedButtons(3);
	std::vector<WeakPtr<ToolbarButton>> buttons;

	for (auto &button : ownedButtons)
	{
		buttons.push_back(button.GetWeakPtr());
	}

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager, buttons, {});

	ASSERT_EQ(static_cast<size_t>(menuView.GetNumItems()), buttons.size());

	for (int i = 0; i < static_cast<int>(buttons.size()); i++)
	{
		const auto &button = buttons[i];
		UINT id = MenuViewTestApi::GetItemId(&menuView, i);

		EXPECT_EQ(MenuViewTestApi::GetItemFullText(&menuView, id), button->GetText());
		EXPECT_EQ(MenuViewTestApi::GetItemHelpText(&menuView, id), button->GetTooltipText());
	}
}

TEST_F(ToolbarOverflowMenuTest, Selection)
{
	MockFunction<void(const MouseEvent &event)> clickCallback;
	TestToolbarButton button(clickCallback.AsStdFunction());

	MockFunction<void(const MouseEvent &event)> middleClickCallback;
	button.SetMiddleClickedCallback(middleClickCallback.AsStdFunction());

	MockFunction<void(const MouseEvent &event)> rightClickCallback;
	button.SetRightClickedCallback(rightClickCallback.AsStdFunction());

	std::vector<WeakPtr<ToolbarButton>> buttons;
	buttons.push_back(button.GetWeakPtr());

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager, buttons, {});

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

	std::vector<WeakPtr<ToolbarButton>> buttons;
	buttons.push_back(button->GetWeakPtr());

	TestMenuView menuView;
	ToolbarOverflowMenu menu(&menuView, &m_acceleratorManager, buttons, {});

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
