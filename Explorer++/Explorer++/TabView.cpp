// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabView.h"
#include "SystemFontHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/WindowSubclass.h"

TabView::TabView(HWND parent, DWORD style, const Config *config) :
	m_hwnd(CreateTabControl(parent, style)),
	m_fontSetter(m_hwnd, config, GetDefaultSystemFontForDefaultDpi())
{
	auto tooltipControl = TabCtrl_GetToolTips(m_hwnd);

	if (tooltipControl)
	{
		m_tooltipFontSetter = std::make_unique<MainFontSetter>(tooltipControl, config);
	}

	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&TabView::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(m_hwnd),
		std::bind_front(&TabView::ParentWndProc, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(std::bind_front(&TabView::OnFontUpdated, this));
}

TabView::~TabView() = default;

HWND TabView::GetHWND() const
{
	return m_hwnd;
}

LRESULT TabView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);

	case WM_NCDESTROY:
		OnNcDestroy();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT TabView::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void TabView::OnMouseWheel(HWND hwnd, int xPos, int yPos, int delta, UINT keys)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(xPos);
	UNREFERENCED_PARAMETER(yPos);
	UNREFERENCED_PARAMETER(keys);

	auto scrollDirection = delta > 0 ? ScrollDirection::Left : ScrollDirection::Right;

	for (int i = 0; i < std::abs(delta / WHEEL_DELTA); i++)
	{
		Scroll(scrollDirection);
	}
}

void TabView::Scroll(ScrollDirection direction)
{
	HWND upDownControl = FindWindowEx(m_hwnd, nullptr, UPDOWN_CLASS, nullptr);

	// It's valid for the control not to exist if all the tabs fit within the window.
	if (!upDownControl)
	{
		return;
	}

	int lowerLimit;
	int upperLimit;
	SendMessage(upDownControl, UDM_GETRANGE32, reinterpret_cast<WPARAM>(&lowerLimit),
		reinterpret_cast<WPARAM>(&upperLimit));

	BOOL positionRetrieved;
	auto position =
		SendMessage(upDownControl, UDM_GETPOS32, 0, reinterpret_cast<LPARAM>(&positionRetrieved));

	if (!positionRetrieved)
	{
		return;
	}

	switch (direction)
	{
	case ScrollDirection::Left:
		position--;
		break;

	case ScrollDirection::Right:
		position++;
		break;
	}

	if (position < lowerLimit || position > upperLimit)
	{
		return;
	}

	SendMessage(m_hwnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), 0);
}

void TabView::OnFontUpdated()
{
	sizeUpdatedSignal.m_signal();
}

void TabView::OnNcDestroy()
{
	windowDestroyedSignal.m_signal();

	delete this;
}
