// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/WindowSubclass.h"
#include <gtest/gtest.h>
#include <CommCtrl.h>

using namespace testing;

class WindowSubclassTest : public Test
{
protected:
	static constexpr UINT WM_APP_PRIVATE = WM_APP;

	void SetUp() override
	{
		m_window.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_window, nullptr);

		m_subclass = std::make_unique<WindowSubclass>(m_window.get(),
			std::bind_front(&WindowSubclassTest::WindowProc, this));
	}

	LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_APP_PRIVATE:
			m_privateMethod.Call();
			break;
		}

		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	wil::unique_hwnd m_window;
	std::unique_ptr<WindowSubclass> m_subclass;
	MockFunction<void()> m_privateMethod;
};

TEST_F(WindowSubclassTest, SubclassAddedAndRemoved)
{
	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(m_privateMethod, Call());
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(m_privateMethod, Call()).Times(0);
	}

	SendMessage(m_window.get(), WM_APP_PRIVATE, 0, 0);

	check.Call(1);

	// This should result in the window subclass being removed, which means the message shouldn't be
	// handled.
	m_subclass.reset();
	SendMessage(m_window.get(), WM_APP_PRIVATE, 0, 0);
}

TEST_F(WindowSubclassTest, InstanceDestroyedAfterWindowDestroyed)
{
	// It should be explicitly safe to destroy the WindowSubclass instance after the associated
	// window has been destroyed.
	m_window.reset();
	m_subclass.reset();
}
