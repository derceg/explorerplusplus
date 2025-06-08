// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabView.h"
#include "SystemFontHelper.h"
#include "TabViewDelegate.h"
#include "../Helper/Controls.h"
#include "../Helper/TabHelper.h"
#include "../Helper/WindowHelper.h"
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

void TabView::SetDelegate(TabViewDelegate *delegate)
{
	m_delegate = delegate;
}

LRESULT TabView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);

	case WM_LBUTTONDOWN:
		OnLeftButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_LBUTTONUP:
		OnLeftButtonUp();
		break;

	case WM_MOUSEMOVE:
		OnMouseMove({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_CAPTURECHANGED:
		OnCaptureChanged(reinterpret_cast<HWND>(lParam));
		break;

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

void TabView::OnLeftButtonDown(const POINT &pt)
{
	auto index = MaybeGetIndexOfTabAtPoint(pt);

	if (!index)
	{
		return;
	}

	SetCapture(m_hwnd);

	DCHECK(!m_tabDragState);
	m_tabDragState = { TabDragAnchor::None };
}

void TabView::OnLeftButtonUp()
{
	if (!m_tabDragState)
	{
		return;
	}

	ReleaseCapture();
}

void TabView::OnMouseMove(const POINT &pt)
{
	if (!m_tabDragState)
	{
		return;
	}

	// The tab being dragged will always be the selected tab.
	int draggedTabIndex = GetSelectedIndex();

	if (MaybeGetIndexOfTabAtPoint(pt) == draggedTabIndex)
	{
		m_tabDragState->anchor = TabDragAnchor::None;
		return;
	}

	auto tabBounds = GetTabBoundsForDrag(draggedTabIndex, m_tabDragState->anchor);

	int targetIndex = draggedTabIndex;

	if (pt.x < tabBounds.left)
	{
		targetIndex--;
	}
	else if (pt.x > tabBounds.right)
	{
		targetIndex++;
	}

	if (!IsValidIndex(targetIndex) || targetIndex == draggedTabIndex)
	{
		return;
	}

	if (targetIndex < draggedTabIndex)
	{
		m_tabDragState->anchor = TabDragAnchor::Right;
	}
	else
	{
		m_tabDragState->anchor = TabDragAnchor::Left;
	}

	TabHelper::SwapItems(m_hwnd, draggedTabIndex, targetIndex);

	if (m_delegate)
	{
		m_delegate->OnTabMoved(draggedTabIndex, targetIndex);
	}
}

TabView::TabDragBounds TabView::GetTabBoundsForDrag(int tabIndex, TabDragAnchor anchor) const
{
	RECT tabRect = GetTabRect(tabIndex);
	TabDragBounds tabBounds = { tabRect.left, tabRect.right };

	if (anchor == TabDragAnchor::Left && IsValidIndex(tabIndex - 1))
	{
		RECT previousTabRect = GetTabRect(tabIndex - 1);
		tabBounds.left = previousTabRect.left + GetRectWidth(&tabRect);
	}
	else if (anchor == TabDragAnchor::Right && IsValidIndex(tabIndex + 1))
	{
		RECT nextTabRect = GetTabRect(tabIndex + 1);
		tabBounds.right = nextTabRect.right - GetRectWidth(&tabRect);
	}

	return tabBounds;
}

void TabView::OnCaptureChanged(HWND target)
{
	if (target != m_hwnd)
	{
		m_tabDragState.reset();
	}
}

int TabView::GetSelectedIndex() const
{
	int index = TabCtrl_GetCurSel(m_hwnd);
	CHECK_NE(index, -1) << "No selected tab";
	return index;
}

bool TabView::IsValidIndex(int index) const
{
	return index >= 0 && index < GetNumTabs();
}

int TabView::GetNumTabs() const
{
	return TabCtrl_GetItemCount(m_hwnd);
}

RECT TabView::GetTabRect(int index) const
{
	RECT tabRect;
	auto res = TabCtrl_GetItemRect(m_hwnd, index, &tabRect);
	CHECK(res);
	return tabRect;
}

std::optional<int> TabView::MaybeGetIndexOfTabAtPoint(const POINT &pt) const
{
	TCHITTESTINFO hitTestInfo = {};
	hitTestInfo.pt = pt;
	int index = TabCtrl_HitTest(m_hwnd, &hitTestInfo);

	if (index < 0)
	{
		return std::nullopt;
	}

	return index;
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
