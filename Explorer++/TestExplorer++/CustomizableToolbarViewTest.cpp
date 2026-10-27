// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "CustomizableToolbarView.h"
#include "Config.h"
#include "NoOpMenuHelpTextHost.h"
#include "TestToolbarButton.h"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

class CustomizableToolbarViewTest : public Test
{
protected:
	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		m_view = std::make_unique<CustomizableToolbarView>(m_parentWindow.get(), WS_CHILD, 0,
			NoOpMenuHelpTextHost::GetInstance(), &m_config);

		auto button1 = std::make_unique<TestToolbarButton>();
		auto button2 = std::make_unique<TestToolbarButton>();
		auto button3 = std::make_unique<TestToolbarButton>();

		m_button1 = button1.get();
		m_button2 = button2.get();
		m_button3 = button3.get();

		std::vector<std::unique_ptr<ToolbarButton>> buttons;
		buttons.push_back(std::move(button1));
		buttons.push_back(std::move(button2));
		buttons.push_back(std::move(button3));

		for (const auto &button : buttons)
		{
			m_availableButtons.push_back(button.get());
		}

		m_view->InitializeAvailableButtons(std::move(buttons));
	}

	bool SendGetButtonInfo(int index, std::wstring &outputText)
	{
		wchar_t text[256] = {};

		NMTOOLBAR notification = {};
		notification.hdr.hwndFrom = m_view->GetHWND();
		notification.hdr.code = TBN_GETBUTTONINFO;
		notification.iItem = index;
		notification.pszText = text;
		notification.cchText = static_cast<int>(std::size(text));
		bool result = static_cast<bool>(SendMessage(m_parentWindow.get(), WM_NOTIFY, 0,
			reinterpret_cast<LPARAM>(&notification)));

		if (result)
		{
			outputText = text;
		}

		return result;
	}

	void SendToolbarReset()
	{
		NMHDR notification = {};
		notification.hwndFrom = m_view->GetHWND();
		notification.code = TBN_RESET;
		SendMessage(m_parentWindow.get(), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&notification));
	}

	Config m_config;

	wil::unique_hwnd m_parentWindow;
	std::unique_ptr<CustomizableToolbarView> m_view;

	TestToolbarButton *m_button1 = nullptr;
	TestToolbarButton *m_button2 = nullptr;
	TestToolbarButton *m_button3 = nullptr;
	std::vector<ToolbarButton *> m_availableButtons;
};

TEST_F(CustomizableToolbarViewTest, SetItems)
{
	std::vector<ToolbarItem> items = { ToolbarItem::Button(m_button2), ToolbarItem::Separator(),
		ToolbarItem::Button(m_button1) };
	m_view->SetItems(items);
	EXPECT_EQ(m_view->GetItems(), items);
}

TEST_F(CustomizableToolbarViewTest, GetButtonInfo)
{
	for (int i = 0; i < static_cast<int>(m_availableButtons.size()); i++)
	{
		std::wstring text;
		bool res = SendGetButtonInfo(i, text);
		ASSERT_TRUE(res);
		EXPECT_EQ(text, m_availableButtons[i]->GetText());
	}
}

TEST_F(CustomizableToolbarViewTest, GetButtonInfoPastEnd)
{
	// Information should be returned for each of the available buttons. When requesting data beyond
	// that, the request should fail.
	std::wstring text;
	bool res = SendGetButtonInfo(static_cast<int>(m_availableButtons.size()), text);
	EXPECT_FALSE(res);
}

TEST_F(CustomizableToolbarViewTest, ResetItems)
{
	std::vector<ToolbarItem> defaultItems = { ToolbarItem::Button(m_button1),
		ToolbarItem::Button(m_button3), ToolbarItem::Separator(), ToolbarItem::Button(m_button2) };
	m_view->SetDefaultItems(defaultItems);

	std::vector<ToolbarItem> items = { ToolbarItem::Button(m_button3),
		ToolbarItem::Button(m_button1) };
	m_view->SetItems(items);

	SendToolbarReset();
	EXPECT_EQ(m_view->GetItems(), defaultItems);
}
