// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContent.h"

TabContent::TabContent(HWND window) : m_window(window)
{
}

TabContent::~TabContent() = default;

HWND TabContent::GetHWND() const
{
	return m_window;
}

void TabContent::AttachToTab(HWND replacedWindow)
{
	RECT bounds{};
	GetWindowRect(replacedWindow, &bounds);
	MapWindowPoints(nullptr, GetParent(m_window), reinterpret_cast<POINT *>(&bounds), 2);

	SetBounds(bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
		false);
	ShowWindow(replacedWindow, SW_HIDE);
}

void TabContent::SetBounds(int x, int y, int width, int height, bool visible)
{
	UINT flags = SWP_NOZORDER | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
	SetWindowPos(m_window, nullptr, x, y, width, height, flags);
}

void TabContent::Focus()
{
	SetFocus(m_window);
}

std::optional<std::wstring> TabContent::GetName() const
{
	return std::nullopt;
}

std::optional<std::wstring> TabContent::GetTooltipText() const
{
	return std::nullopt;
}

std::optional<TabContent::Icon> TabContent::GetIcon() const
{
	return std::nullopt;
}

TabContent::MessageResult TabContent::ProcessMessage(const MSG *msg)
{
	UNREFERENCED_PARAMETER(msg);
	return MessageResult::NotHandled;
}

void TabContent::SetUpdatedCallback(std::function<void()> updatedCallback)
{
	m_updatedCallback = std::move(updatedCallback);
}

void TabContent::NotifyUpdated()
{
	if (m_updatedCallback)
	{
		m_updatedCallback();
	}
}
