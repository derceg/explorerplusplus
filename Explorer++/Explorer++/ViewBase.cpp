// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewBase.h"
#include "../Helper/WindowSubclass.h"

ViewBase::ViewBase(HWND hwnd) : m_hwnd(hwnd)
{
	AddSubclass(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&ViewBase::WndProc, this)));
}

ViewBase::~ViewBase() = default;

HWND ViewBase::GetHWND() const
{
	// If this CHECK is triggered, it means a window has been destroyed early, while a derived class
	// or outside client is still trying to access it.
	CHECK(m_hwnd);

	return m_hwnd;
}

void ViewBase::AddSubclass(std::unique_ptr<WindowSubclass> subclass)
{
	m_windowSubclasses.push_back(std::move(subclass));
}

LRESULT ViewBase::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCDESTROY:
		OnNcDestroy();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ViewBase::OnNcDestroy()
{
	// A derived class might subclass:
	//
	// 1. The window itself.
	// 2. One or more of the window's children.
	// 3. The parent window.
	//
	// Case 3 is risky, since this class is designed to be held for the lifetime of the parent. That
	// means that the window being managed here can be destroyed while this class temporarily
	// remains live (and the other children of the parent window are destroyed). Which then means
	// that a parent window subclass can continue to receive messages even after the window has been
	// destroyed. Attempting to call GetHWND() in that situation is going to trigger a CHECK
	// failure, since the window is gone.
	//
	// Therefore, clearing the subclasses here ensures that a derived class isn't going to continue
	// to process window messages after the window is destroyed.
	m_windowSubclasses.clear();

	m_hwnd = nullptr;
}
