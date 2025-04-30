// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeTabControlPainter.h"
#include "DarkModeColorProvider.h"
#include "../Helper/TabHelper.h"
#include "../Helper/WindowHelper.h"

DarkModeTabControlPainter::DarkModeTabControlPainter(HWND hwnd,
	const DarkModeColorProvider *darkModeColorProvider) :
	m_hwnd(hwnd),
	m_darkModeColorProvider(darkModeColorProvider)
{
}

void DarkModeTabControlPainter::SetHotItem(int hotItem)
{
	if (hotItem < 0 || hotItem >= TabCtrl_GetItemCount(m_hwnd))
	{
		m_hotItem.reset();
		return;
	}

	m_hotItem = hotItem;
}

void DarkModeTabControlPainter::ClearHotItem()
{
	m_hotItem.reset();
}

void DarkModeTabControlPainter::Paint(HDC hdc, const RECT &paintRect)
{
	auto style = GetWindowLongPtr(m_hwnd, GWL_STYLE);

	// These styles aren't handled and it's not expected that they'll be set.
	CHECK(WI_AreAllFlagsClear(style, TCS_BUTTONS | TCS_VERTICAL));

	// Conversely, it's expected that these styles will always be set and there is no handling for
	// any case where they're not set.
	CHECK(WI_AreAllFlagsSet(style, TCS_FOCUSNEVER | TCS_SINGLELINE));

	// Fill in the background from the parent control. This is what the control does normally.
	HRESULT hr = DrawThemeParentBackground(m_hwnd, hdc, &paintRect);
	DCHECK(SUCCEEDED(hr));

	RECT clientRect;
	auto res = GetClientRect(m_hwnd, &clientRect);
	DCHECK(res);

	RECT bottomEdgeRect = { clientRect.left, clientRect.bottom - 2, clientRect.right,
		clientRect.bottom - 1 };
	int frameRes = FrameRect(hdc, &bottomEdgeRect, m_darkModeColorProvider->GetBorderBrush());
	DCHECK_NE(frameRes, 0);

	int modeRes = SetBkMode(hdc, TRANSPARENT);
	DCHECK_NE(modeRes, 0);

	auto font = reinterpret_cast<HFONT>(SendMessage(m_hwnd, WM_GETFONT, 0, 0));
	wil::unique_select_object selectFont;

	if (font)
	{
		selectFont = wil::SelectObject(hdc, font);
	}

	int numTabs = TabCtrl_GetItemCount(m_hwnd);

	for (int i = 0; i < numTabs; i++)
	{
		auto itemRect = GetTabRect(i);

		RECT intersectionRect;
		if (!IntersectRect(&intersectionRect, &paintRect, &itemRect))
		{
			continue;
		}

		DrawTab(i, hdc);
	}
}

void DarkModeTabControlPainter::DrawTab(int index, HDC hdc)
{
	bool isHot = (index == m_hotItem);
	int selectedIndex = TabCtrl_GetCurSel(m_hwnd);
	bool isSelected = (index == selectedIndex);

	auto itemRect = GetTabRect(index);
	RECT backgroundRect = itemRect;

	if (isSelected)
	{
		// There's a bottom edge drawn directly underneath the tabs. The background for the selected
		// tab is drawn over that edge.
		backgroundRect.bottom += 1;
	}

	auto backgroundBrush = isSelected ? m_darkModeColorProvider->GetSelectedItemBackgroundBrush()
		: isHot                       ? m_darkModeColorProvider->GetHotItemBackgroundBrush()
									  : m_darkModeColorProvider->GetTabBackgroundBrush();
	int res = FillRect(hdc, &backgroundRect, backgroundBrush);
	DCHECK_NE(res, 0);

	Gdiplus::Graphics graphics(hdc);
	Gdiplus::Color borderColor;
	borderColor.SetFromCOLORREF(DarkModeColorProvider::BORDER_COLOR);
	Gdiplus::Pen borderPen(borderColor);
	Gdiplus::Status status;

	if (index == 0)
	{
		status = graphics.DrawLine(&borderPen, static_cast<int>(itemRect.left), itemRect.top,
			itemRect.left, itemRect.bottom);
		DCHECK_EQ(status, Gdiplus::Ok);
	}

	int rightBorderTop = itemRect.top;

	// If this is the tab before the selected tab, then the border needs to be raised to the top of
	// the viewport (to match the top and right edge for the selected tab).
	if (index == (selectedIndex - 1))
	{
		rightBorderTop = 0;
	}

	status = graphics.DrawLine(&borderPen, static_cast<int>(itemRect.right) - 1, rightBorderTop,
		itemRect.right - 1, itemRect.bottom);
	DCHECK_EQ(status, Gdiplus::Ok);

	status = graphics.DrawLine(&borderPen, static_cast<int>(itemRect.left), itemRect.top,
		itemRect.right - 1, itemRect.top);
	DCHECK_EQ(status, Gdiplus::Ok);

	auto text = TabHelper::GetItemText(m_hwnd, index);

	RECT textRect;
	res = DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_CALCRECT);
	DCHECK_NE(res, 0);

	auto imageList = TabCtrl_GetImageList(m_hwnd);

	TCITEM tcItem = {};
	tcItem.mask = TCIF_IMAGE;
	auto getItemRes = TabCtrl_GetItem(m_hwnd, index, &tcItem);
	CHECK(getItemRes);

	RECT drawRect = itemRect;

	if (imageList && tcItem.iImage != -1)
	{
		int iconWidth;
		int iconHeight;
		res = ImageList_GetIconSize(imageList, &iconWidth, &iconHeight);
		CHECK(res);

		// Although a TCM_SETPADDING message exists, there is no corresponding message to retrieve
		// the padding. Therefore, the padding will be calculated using the same method the control
		// uses. Note that this implicitly assumes that the padding hasn't been customized.
		int xPadding = GetSystemMetrics(SM_CXEDGE) * 3;
		DCHECK_NE(xPadding, 0);

		int contentWidth = iconWidth + xPadding + GetRectWidth(&textRect);
		POINT imageOrigin = { drawRect.left + (GetRectWidth(&drawRect) - contentWidth) / 2,
			drawRect.top + (GetRectHeight(&drawRect) - iconHeight) / 2 };
		auto drawRes =
			ImageList_Draw(imageList, tcItem.iImage, hdc, imageOrigin.x, imageOrigin.y, ILD_NORMAL);
		DCHECK(drawRes);

		drawRect.left += iconWidth + xPadding;
	}

	auto colorRes = SetTextColor(hdc,
		isSelected ? DarkModeColorProvider::TEXT_COLOR
				   : DarkModeColorProvider::TEXT_COLOR_BACKGROUND);
	DCHECK_NE(colorRes, CLR_INVALID);
	res = DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &drawRect,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	DCHECK_NE(res, 0);
}

RECT DarkModeTabControlPainter::GetTabRect(int index)
{
	RECT itemRect;
	auto res = TabCtrl_GetItemRect(m_hwnd, index, &itemRect);
	CHECK(res);

	bool isSelected = (index == TabCtrl_GetCurSel(m_hwnd));

	if (isSelected)
	{
		// Each tab in the control is at a slight vertical offset. The selected tab, however, will
		// align with the top of the viewport.
		itemRect.top = 0;
	}

	return itemRect;
}
