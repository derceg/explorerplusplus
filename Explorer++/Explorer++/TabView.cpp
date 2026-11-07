// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabView.h"
#include "Icon.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "SystemFontHelper.h"
#include "TabViewDelegate.h"
#include "ThemeManager.h"
#include "../Helper/Controls.h"
#include "../Helper/DoubleBufferedPaint.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/LayoutHelper.h"
#include "../Helper/ScopedDCState.h"
#include "../Helper/TabHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <wil/resource.h>
#include <windowsx.h>

void TabViewItem::SetParent(TabView *parent)
{
	m_parent = parent;
}

void TabViewItem::SetDoubleClickedCallback(MouseEventCallback doubleClickedCallback)
{
	m_doubleClickedCallback = std::move(doubleClickedCallback);
}

void TabViewItem::SetMiddleClickedCallback(MouseEventCallback middleClickedCallback)
{
	m_middleClickedCallback = std::move(middleClickedCallback);
}

void TabViewItem::SetRightClickedCallback(MouseEventCallback rightClickedCallback)
{
	m_rightClickedCallback = std::move(rightClickedCallback);
}

void TabViewItem::SetClosedCallback(ClosedCallback closedCallback)
{
	m_closedCallback = std::move(closedCallback);
}

void TabViewItem::OnDoubleClicked(const MouseEvent &event)
{
	if (m_doubleClickedCallback)
	{
		m_doubleClickedCallback(event);
	}
}

void TabViewItem::OnMiddleClicked(const MouseEvent &event)
{
	if (m_middleClickedCallback)
	{
		m_middleClickedCallback(event);
	}
}

void TabViewItem::OnRightClicked(const MouseEvent &event)
{
	if (m_rightClickedCallback)
	{
		m_rightClickedCallback(event);
	}
}

void TabViewItem::OnClosed()
{
	if (m_closedCallback)
	{
		m_closedCallback();
	}
}

void TabViewItem::NotifyParentOfUpdate() const
{
	if (m_parent)
	{
		m_parent->UpdateTab(this);
	}
}

TabView::TabView(HWND parent, DWORD style, const ThemeManager *themeManager, const Config *config,
	const ResourceLoader *resourceLoader) :
	ViewBase(CreateTabControl(parent, style)),
	m_themeManager(themeManager),
	m_resourceLoader(resourceLoader),
	m_fontSetter(GetHWND(), config, GetDefaultSystemFontForDefaultDpi())
{
	auto tooltipControl = TabCtrl_GetToolTips(GetHWND());

	if (tooltipControl)
	{
		m_tooltipFontSetter = std::make_unique<MainFontSetter>(tooltipControl, config);
	}

	UpdateDpiDependentResources();

	AddSubclass(
		std::make_unique<WindowSubclass>(GetHWND(), std::bind_front(&TabView::WndProc, this)));
	AddSubclass(
		std::make_unique<WindowSubclass>(parent, std::bind_front(&TabView::ParentWndProc, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(std::bind_front(&TabView::OnFontUpdated, this));
}

TabView::~TabView() = default;

void TabView::SetDelegate(TabViewDelegate *delegate)
{
	m_delegate = delegate;
}

int TabView::AddTab(std::unique_ptr<TabViewItem> tabItem, int index)
{
	std::wstring text = tabItem->GetText();
	auto iconIndex = tabItem->GetIconIndex();

	TCITEM tcItem = {};
	tcItem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
	tcItem.pszText = text.data();
	tcItem.iImage = iconIndex ? *iconIndex : -1;
	tcItem.lParam = reinterpret_cast<LPARAM>(tabItem.get());
	int insertedIndex = TabCtrl_InsertItem(GetHWND(), index, &tcItem);
	CHECK_NE(insertedIndex, -1);

	tabItem->SetParent(this);
	m_tabs.push_back(std::move(tabItem));

	return insertedIndex;
}

void TabView::UpdateTab(const TabViewItem *tabItem)
{
	int index = GetIndexOfTabItem(tabItem);

	auto originalIconIndex = MaybeGetTabIconIndex(index);

	auto text = tabItem->GetText();
	auto iconIndex = tabItem->GetIconIndex();

	TCITEM tcItem = {};
	tcItem.mask = TCIF_TEXT | TCIF_IMAGE;
	tcItem.pszText = text.data();
	tcItem.iImage = iconIndex ? *iconIndex : -1;
	auto res = TabCtrl_SetItem(GetHWND(), index, &tcItem);
	CHECK(res);

	if (originalIconIndex && *originalIconIndex != iconIndex)
	{
		MaybeRemoveIcon(*originalIconIndex);
	}
}

int TabView::MoveTab(int fromIndex, int toIndex)
{
	CHECK(IsValidIndex(fromIndex));
	int finalIndex = TabHelper::MoveItem(GetHWND(), fromIndex, toIndex);

	if (finalIndex != fromIndex && m_delegate)
	{
		m_delegate->OnTabMoved(fromIndex, finalIndex);
	}

	return finalIndex;
}

int TabView::GetIndexOfTabItem(const TabViewItem *tabItem) const
{
	int numTabs = GetNumTabs();

	for (int i = 0; i < numTabs; i++)
	{
		if (GetTabAtIndex(i) == tabItem)
		{
			return i;
		}
	}

	LOG(FATAL) << "Couldn't determine index for tab";
}

void TabView::RemoveTab(int index)
{
	CHECK(IsValidIndex(index));

	auto itr =
		std::ranges::find_if(m_tabs, [tabItem = GetTabAtIndex(index)](const auto &currentTabItem)
			{ return currentTabItem.get() == tabItem; });
	CHECK(itr != m_tabs.end());
	m_tabs.erase(itr);

	auto iconIndex = MaybeGetTabIconIndex(index);

	auto res = TabCtrl_DeleteItem(GetHWND(), index);
	CHECK(res);

	if (iconIndex)
	{
		MaybeRemoveIcon(*iconIndex);
	}
}

std::optional<int> TabView::MaybeGetTabIconIndex(int index) const
{
	TCITEM tcItem = {};
	tcItem.mask = TCIF_IMAGE;
	auto res = TabCtrl_GetItem(GetHWND(), index, &tcItem);
	CHECK(res);

	if (tcItem.iImage == -1)
	{
		return std::nullopt;
	}

	return tcItem.iImage;
}

void TabView::MaybeRemoveIcon(int iconIndex)
{
	if (m_delegate && m_delegate->ShouldRemoveIcon(iconIndex))
	{
		TabCtrl_RemoveImage(GetHWND(), iconIndex);
	}
}

void TabView::SetImageList(HIMAGELIST imageList)
{
	// It only makes sense to set the image list once (i.e. there shouldn't be any previous image
	// list set).
	auto previousImageList = TabCtrl_SetImageList(GetHWND(), imageList);
	DCHECK(!previousImageList);
}

TabViewItem *TabView::GetTabAtIndex(int index) const
{
	TCITEM tcItem = {};
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(GetHWND(), index, &tcItem);
	CHECK(res) << "Tab lookup failed for tab at index " << index;
	return reinterpret_cast<TabViewItem *>(tcItem.lParam);
}

void TabView::SelectTabAtIndex(int index)
{
	CHECK(IsValidIndex(index));

	if (index == GetSelectedIndex())
	{
		return;
	}

	int previousTab = TabCtrl_SetCurSel(GetHWND(), index);
	CHECK_NE(previousTab, -1);

	OnSelectionChanged();
}

int TabView::GetSelectedIndex() const
{
	auto index = MaybeGetSelectedIndex();
	CHECK(index) << "No selected tab";
	return *index;
}

std::optional<int> TabView::MaybeGetSelectedIndex() const
{
	int index = TabCtrl_GetCurSel(GetHWND());

	if (index == -1)
	{
		return std::nullopt;
	}

	return index;
}

LRESULT TabView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);

	case WM_LBUTTONDOWN:
		if (OnLeftButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }))
		{
			return 0;
		}
		break;

	case WM_LBUTTONUP:
		OnLeftButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_MOUSEMOVE:
		OnMouseMove({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_MOUSELEAVE:
		if (m_hotItemDetails)
		{
			m_hotItemDetails.reset();
			InvalidateRect(GetHWND(), nullptr, false);
		}
		break;

	case WM_CAPTURECHANGED:
		OnCaptureChanged(reinterpret_cast<HWND>(lParam));
		break;

	case WM_LBUTTONDBLCLK:
		OnLeftButtonDoubleClick({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			static_cast<UINT>(wParam));
		break;

	case WM_MBUTTONDOWN:
		OnMiddleButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			static_cast<UINT>(wParam));
		break;

	case WM_MBUTTONUP:
		OnMiddleButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, static_cast<UINT>(wParam));
		break;

	case WM_RBUTTONUP:
		OnRightButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, static_cast<UINT>(wParam));
		break;

	case WM_DPICHANGED_AFTERPARENT:
		UpdateDpiDependentResources();
		InvalidateRect(GetHWND(), nullptr, false);
		sizeUpdatedSignal.m_signal();
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		OnPaint();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT TabView::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == GetHWND())
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case TCN_SELCHANGE:
				OnSelectionChanged();
				break;
			}
		}
		else if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == TabCtrl_GetToolTips(GetHWND()))
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case TTN_GETDISPINFO:
				OnGetDispInfo(reinterpret_cast<NMTTDISPINFO *>(lParam));
				break;
			}
		}
		break;
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
	HWND upDownControl = FindWindowEx(GetHWND(), nullptr, UPDOWN_CLASS, nullptr);

	// It's valid for the control not to exist if all the tabs fit within the window.
	if (!upDownControl)
	{
		return;
	}

	int lowerLimit;
	int upperLimit;
	SendMessage(upDownControl, UDM_GETRANGE32, reinterpret_cast<WPARAM>(&lowerLimit),
		reinterpret_cast<WPARAM>(&upperLimit));

	auto position = SendMessage(upDownControl, UDM_GETPOS32, 0, 0);

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

	SendMessage(GetHWND(), WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), 0);
}

bool TabView::OnLeftButtonDown(const POINT &pt)
{
	auto index = MaybeGetIndexOfTabAtPoint(pt);

	if (!index)
	{
		return false;
	}

	DCHECK(!m_leftButtonDownPoint);
	DCHECK(!m_pressedCloseButtonIndex);
	DCHECK(!m_tabDragState);

	SetCapture(GetHWND());

	auto layout = GetTabContentLayout(*index);

	if (PtInRect(&layout.closeButton, pt))
	{
		// In this case, the left button has gone down on one of the close buttons. The associated
		// tab shouldn't be selected, so the event is consumed.
		m_pressedCloseButtonIndex = *index;
		return true;
	}

	m_leftButtonDownPoint = pt;

	return false;
}

void TabView::OnLeftButtonUp(const POINT &pt)
{
	MaybeHandleCloseClick(pt);

	ReleaseCapture();
}

void TabView::MaybeHandleCloseClick(const POINT &pt)
{
	auto index = MaybeGetIndexOfTabAtPoint(pt);

	if (!index || index != m_pressedCloseButtonIndex)
	{
		return;
	}

	auto layout = GetTabContentLayout(*index);

	if (!PtInRect(&layout.closeButton, pt))
	{
		return;
	}

	auto *tabItem = GetTabAtIndex(*index);
	tabItem->OnClosed();
}

void TabView::OnMouseMove(const POINT &pt)
{
	// This ensures that WM_MOUSELEAVE messages are dispatched, which is important for keeping track
	// of which close button (if any) is hot.
	TRACKMOUSEEVENT trackEvent = {};
	trackEvent.cbSize = sizeof(trackEvent);
	trackEvent.dwFlags = TME_LEAVE;
	trackEvent.hwndTrack = GetHWND();
	auto res = TrackMouseEvent(&trackEvent);
	CHECK(res);

	UpdateHotItemDetails(pt);
	MaybeStartTabDrag(pt);

	if (m_tabDragState)
	{
		UpdateTabDrag(pt);
	}
}

void TabView::UpdateHotItemDetails(const POINT &pt)
{
	std::optional<HotItemDetails> hotItemDetails;
	auto hotItemIndex = MaybeGetIndexOfTabAtPoint(pt);

	if (hotItemIndex)
	{
		auto layout = GetTabContentLayout(*hotItemIndex);
		hotItemDetails = HotItemDetails{ *hotItemIndex,
			PtInRect(&layout.closeButton, pt) ? TabPart::CloseButton : TabPart::Body };
	}

	if (hotItemDetails == m_hotItemDetails)
	{
		return;
	}

	m_hotItemDetails = hotItemDetails;
	InvalidateRect(GetHWND(), nullptr, false);
}

void TabView::MaybeStartTabDrag(const POINT &pt)
{
	if (!m_leftButtonDownPoint)
	{
		return;
	}

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(GetHWND());

	RECT rect = { m_leftButtonDownPoint->x, m_leftButtonDownPoint->y, m_leftButtonDownPoint->x,
		m_leftButtonDownPoint->y };
	InflateRect(&rect, dpiCompat.GetSystemMetricsForDpi(SM_CXDRAG, dpi),
		dpiCompat.GetSystemMetricsForDpi(SM_CYDRAG, dpi));

	if (PtInRect(&rect, pt))
	{
		return;
	}

	m_leftButtonDownPoint.reset();

	DCHECK(!m_tabDragState);
	m_tabDragState = { TabDragAnchor::None };
}

void TabView::UpdateTabDrag(const POINT &pt)
{
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

	TabHelper::SwapItems(GetHWND(), draggedTabIndex, targetIndex);

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
	if (target != GetHWND())
	{
		m_leftButtonDownPoint.reset();
		m_pressedCloseButtonIndex.reset();
		m_tabDragState.reset();
		m_middleClickItemIndex.reset();
	}
}

void TabView::OnLeftButtonDoubleClick(const POINT &pt, UINT keysDown)
{
	auto index = MaybeGetIndexOfTabAtPoint(pt);

	if (!index)
	{
		return;
	}

	POINT ptScreen = pt;
	auto res = ClientToScreen(GetHWND(), &ptScreen);
	CHECK(res);

	auto *tabItem = GetTabAtIndex(*index);
	tabItem->OnDoubleClicked(
		{ ptScreen, WI_IsFlagSet(keysDown, MK_SHIFT), WI_IsFlagSet(keysDown, MK_CONTROL) });
}

void TabView::OnMiddleButtonDown(const POINT &pt, UINT keysDown)
{
	UNREFERENCED_PARAMETER(keysDown);

	auto index = MaybeGetIndexOfTabAtPoint(pt);

	if (!index)
	{
		return;
	}

	SetCapture(GetHWND());

	m_middleClickItemIndex = *index;
}

void TabView::OnMiddleButtonUp(const POINT &pt, UINT keysDown)
{
	if (!m_middleClickItemIndex)
	{
		return;
	}

	auto releaseCapture = wil::scope_exit([] { ReleaseCapture(); });

	auto index = MaybeGetIndexOfTabAtPoint(pt);

	if (!index || *index != *m_middleClickItemIndex)
	{
		return;
	}

	POINT ptScreen = pt;
	auto res = ClientToScreen(GetHWND(), &ptScreen);
	CHECK(res);

	auto *tabItem = GetTabAtIndex(*index);
	tabItem->OnMiddleClicked(
		{ ptScreen, WI_IsFlagSet(keysDown, MK_SHIFT), WI_IsFlagSet(keysDown, MK_CONTROL) });
}

void TabView::OnRightButtonUp(const POINT &pt, UINT keysDown)
{
	auto index = MaybeGetIndexOfTabAtPoint(pt);

	if (!index)
	{
		return;
	}

	POINT ptScreen = pt;
	auto res = ClientToScreen(GetHWND(), &ptScreen);
	CHECK(res);

	auto *tabItem = GetTabAtIndex(*index);
	tabItem->OnRightClicked(
		{ ptScreen, WI_IsFlagSet(keysDown, MK_SHIFT), WI_IsFlagSet(keysDown, MK_CONTROL) });
}

void TabView::UpdateDpiDependentResources()
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(GetHWND());
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	m_closeIconImageList.reset(
		ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 1, 0));
	CHECK(m_closeIconImageList);

	wil::unique_hbitmap closeIcon =
		m_resourceLoader->LoadBitmapFromPNGAndScale(Icon::CloseButton, iconWidth, iconHeight);
	m_closeIconImageIndex = ImageList_Add(m_closeIconImageList.get(), closeIcon.get(), nullptr);
	CHECK_NE(m_closeIconImageIndex, -1);

	m_horizontalPadding = dpiCompat.GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 3;
	DCHECK_NE(m_horizontalPadding, 0);

	int verticalPadding = dpiCompat.GetSystemMetricsForDpi(SM_CYEDGE, dpi) * 3 / 2;
	DCHECK_NE(verticalPadding, 0);

	// It's assumed that the control will apply the padding 3 times:
	//
	// padding | icon | padding | text | padding
	//
	// In the custom layout drawn below, the following is required:
	//
	// padding | icon | padding | text | padding | close button | padding
	//
	// So, 4 * padding + close button width will be represented by the 3 * padding that the control
	// actually uses.
	//
	// Note that the result is round up as per https://stackoverflow.com/a/2745086.
	int finalHorizontalPadding = (4 * m_horizontalPadding + iconWidth + 2) / 3;
	TabCtrl_SetPadding(GetHWND(), finalHorizontalPadding, verticalPadding);
}

void TabView::OnPaint()
{
	DoubleBufferedPaint doubleBufferedPaint(GetHWND());
	Paint(doubleBufferedPaint.GetMemoryDC(), doubleBufferedPaint.GetPaintRect());
}

void TabView::Paint(HDC hdc, const RECT &paintRect)
{
	auto style = GetWindowLongPtr(GetHWND(), GWL_STYLE);

	// These styles aren't handled and it's not expected that they'll be set.
	CHECK(WI_AreAllFlagsClear(style, TCS_BUTTONS | TCS_VERTICAL));

	// Conversely, it's expected that these styles will always be set and there is no handling for
	// any case where they're not set.
	CHECK(WI_AreAllFlagsSet(style, TCS_FOCUSNEVER | TCS_SINGLELINE));

	// Fill in the background from the parent control. This is what the control does normally.
	HRESULT hr = DrawThemeParentBackground(GetHWND(), hdc, &paintRect);
	DCHECK(SUCCEEDED(hr));

	RECT clientRect;
	auto res = GetClientRect(GetHWND(), &clientRect);
	DCHECK(res);

	auto *colorProvider = m_themeManager->GetCurrentColorProvider();

	RECT bottomBorderRect = { clientRect.left, clientRect.bottom - 2, clientRect.right,
		clientRect.bottom - 1 };
	int frameRes = FrameRect(hdc, &bottomBorderRect, colorProvider->GetBorderBrush());
	DCHECK_NE(frameRes, 0);

	RECT bottomEdgeRect = { clientRect.left, clientRect.bottom - 1, clientRect.right,
		clientRect.bottom };
	frameRes = FrameRect(hdc, &bottomEdgeRect, colorProvider->GetSelectedItemBackgroundBrush());
	DCHECK_NE(frameRes, 0);

	int modeRes = SetBkMode(hdc, TRANSPARENT);
	DCHECK_NE(modeRes, 0);

	auto font = reinterpret_cast<HFONT>(SendMessage(GetHWND(), WM_GETFONT, 0, 0));
	wil::unique_select_object selectFont;

	if (font)
	{
		selectFont = wil::SelectObject(hdc, font);
	}

	HWND upDownControl = FindWindowEx(GetHWND(), nullptr, UPDOWN_CLASS, nullptr);

	ScopedDCState dcState(hdc);
	DCHECK(dcState.IsValid());

	if (upDownControl && IsWindowVisible(upDownControl))
	{
		RECT upDownRect;
		res = GetWindowRect(upDownControl, &upDownRect);
		CHECK(res);

		MapWindowPoints(HWND_DESKTOP, GetHWND(), reinterpret_cast<POINT *>(&upDownRect), 2);

		// This matches the native behavior of the control, which will draw tabs up to the left edge
		// of the up-down control. Doing this is important, since without this, a tab to the right
		// can be shown underneath the up-down control. When that tab is scrolled to the left, the
		// background color can remain in place behind the up-down control, as the tab control
		// doesn't expect that anything will be drawn in that area.
		IntersectClipRect(hdc, clientRect.left, clientRect.top, upDownRect.left, clientRect.bottom);
	}

	int numTabs = static_cast<int>(m_tabs.size());

	for (int i = 0; i < numTabs; i++)
	{
		DrawTab(hdc, i);
	}
}

void TabView::DrawTab(HDC hdc, int index)
{
	bool isHot = (m_hotItemDetails && index == m_hotItemDetails->index);
	int selectedIndex = TabCtrl_GetCurSel(GetHWND());
	bool isSelected = (index == selectedIndex);

	auto itemRect = GetTabRectForPainting(index);
	RECT backgroundRect = itemRect;

	if (isSelected)
	{
		// There's a bottom border drawn directly underneath the tabs. The background for the
		// selected tab is drawn over that border.
		backgroundRect.bottom += 1;
	}

	auto *colorProvider = m_themeManager->GetCurrentColorProvider();

	auto backgroundBrush = isSelected ? colorProvider->GetSelectedItemBackgroundBrush()
		: isHot                       ? colorProvider->GetHotItemBackgroundBrush()
									  : colorProvider->GetTabBackgroundBrush();
	int res = FillRect(hdc, &backgroundRect, backgroundBrush);
	DCHECK_NE(res, 0);

	Gdiplus::Graphics graphics(hdc);
	Gdiplus::Color borderColor;
	borderColor.SetFromCOLORREF(colorProvider->GetBorderColor());
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

	auto layout = GetTabContentLayout(index);

	auto imageList = TabCtrl_GetImageList(GetHWND());

	TCITEM tcItem = {};
	tcItem.mask = TCIF_IMAGE;
	auto getItemRes = TabCtrl_GetItem(GetHWND(), index, &tcItem);
	CHECK(getItemRes);

	if (imageList && tcItem.iImage != -1)
	{
		auto drawRes = ImageList_Draw(imageList, tcItem.iImage, hdc, layout.icon.left,
			layout.icon.top, ILD_NORMAL);
		DCHECK(drawRes);
	}

	auto colorRes = SetTextColor(hdc,
		isSelected ? colorProvider->GetTextColor() : colorProvider->GetBackgroundTextColor());
	DCHECK_NE(colorRes, CLR_INVALID);

	auto text = TabHelper::GetItemText(GetHWND(), index);
	res = DrawText(hdc, text.c_str(), -1, &layout.text, DT_VCENTER | DT_SINGLELINE);
	DCHECK_NE(res, 0);

	IMAGELISTDRAWPARAMS params = {};
	params.cbSize = sizeof(params);
	params.himl = m_closeIconImageList.get();
	params.i = m_closeIconImageIndex;
	params.hdcDst = hdc;
	params.x = layout.closeButton.left;
	params.y = layout.closeButton.top;
	params.rgbBk = CLR_NONE;
	params.fStyle = ILD_TRANSPARENT;
	params.fState =
		(isHot && m_hotItemDetails->part == TabPart::CloseButton) ? ILS_NORMAL : ILS_SATURATE;
	res = ImageList_DrawIndirect(&params);
	DCHECK(res);
}

TabView::TabContentLayout TabView::GetTabContentLayout(int index) const
{
	auto itemRect = GetTabRectForPainting(index);
	InflateRect(&itemRect, -m_horizontalPadding, 0);

	HorizontalLayout row(itemRect);
	TabContentLayout layout = {};

	auto imageList = TabCtrl_GetImageList(GetHWND());

	TCITEM tcItem = {};
	tcItem.mask = TCIF_IMAGE;
	auto getItemRes = TabCtrl_GetItem(GetHWND(), index, &tcItem);
	CHECK(getItemRes);

	if (imageList && tcItem.iImage != -1)
	{
		int iconWidth;
		int iconHeight;
		auto res = ImageList_GetIconSize(imageList, &iconWidth, &iconHeight);
		CHECK(res);

		layout.icon = CenterVertically(row.TakeLeft(iconWidth), iconHeight);
		row.TakeLeft(m_horizontalPadding);
	}

	int closeIconWidth;
	int closeIconHeight;
	auto res = ImageList_GetIconSize(m_closeIconImageList.get(), &closeIconWidth, &closeIconHeight);
	CHECK(res);

	layout.closeButton = CenterVertically(row.TakeRight(closeIconWidth), closeIconHeight);
	row.TakeRight(m_horizontalPadding);

	layout.text = row.Remaining();
	return layout;
}

RECT TabView::GetTabRectForPainting(int index) const
{
	auto itemRect = GetTabRect(index);
	bool isSelected = (index == TabCtrl_GetCurSel(GetHWND()));

	if (isSelected)
	{
		// Each tab in the control is at a slight vertical offset. The selected tab, however, will
		// align with the top of the viewport.
		itemRect.top = 0;
	}

	return itemRect;
}

void TabView::OnGetDispInfo(NMTTDISPINFO *dispInfo)
{
	static wchar_t tabToolTip[512];

	auto layout = GetTabContentLayout(static_cast<int>(dispInfo->hdr.idFrom));

	DWORD cursorPos = GetMessagePos();
	POINT ptCursor = { GET_X_LPARAM(cursorPos), GET_Y_LPARAM(cursorPos) };
	ScreenToClient(GetHWND(), &ptCursor);

	if (PtInRect(&layout.closeButton, ptCursor))
	{
		auto tooltip = m_resourceLoader->LoadString(IDS_TAB_CLOSE_BUTTON_TOOLTIP);
		StringCchCopy(tabToolTip, std::size(tabToolTip), tooltip.c_str());
	}
	else
	{
		auto *tabItem = GetTabAtIndex(static_cast<int>(dispInfo->hdr.idFrom));
		auto tooltip = tabItem->GetTooltipText();
		StringCchCopy(tabToolTip, std::size(tabToolTip), tooltip.c_str());
	}

	dispInfo->lpszText = tabToolTip;
}

void TabView::OnSelectionChanged()
{
	if (m_delegate)
	{
		m_delegate->OnSelectionChanged();
	}
}

bool TabView::IsValidIndex(int index) const
{
	return index >= 0 && index < GetNumTabs();
}

int TabView::GetNumTabs() const
{
	return static_cast<int>(m_tabs.size());
}

RECT TabView::GetTabRect(int index) const
{
	RECT tabRect;
	auto res = TabCtrl_GetItemRect(GetHWND(), index, &tabRect);
	CHECK(res);
	return tabRect;
}

std::optional<int> TabView::MaybeGetIndexOfTabAtPoint(const POINT &pt) const
{
	TCHITTESTINFO hitTestInfo = {};
	hitTestInfo.pt = pt;
	int index = TabCtrl_HitTest(GetHWND(), &hitTestInfo);

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
