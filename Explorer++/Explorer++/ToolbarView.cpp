// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarView.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include <glog/logging.h>
#include <wil/common.h>

ToolbarButton::ToolbarButton(MouseEventCallback clickedCallback) :
	m_clickedCallback(clickedCallback)
{
}

void ToolbarButton::SetParent(ToolbarView *parent)
{
	m_parent = parent;
}

bool ToolbarButton::ShouldSuppressClick() const
{
	return false;
}

void ToolbarButton::SetRightClickedCallback(MouseEventCallback rightClickedCallback)
{
	m_rightClickedCallback = rightClickedCallback;
}

void ToolbarButton::SetMiddleClickedCallback(MouseEventCallback middleClickedCallback)
{
	m_middleClickedCallback = middleClickedCallback;
}

void ToolbarButton::SetDragStartedCallback(DragStartedCallback dragStartedCallback)
{
	m_dragStartedCallback = dragStartedCallback;
}

void ToolbarButton::OnClicked(const MouseEvent &event)
{
	m_clickedCallback(event);
}

void ToolbarButton::OnRightClicked(const MouseEvent &event)
{
	if (m_rightClickedCallback)
	{
		m_rightClickedCallback(event);
	}
}

void ToolbarButton::OnMiddleClicked(const MouseEvent &event)
{
	if (m_middleClickedCallback)
	{
		m_middleClickedCallback(event);
	}
}

void ToolbarButton::OnDragStarted()
{
	if (m_dragStartedCallback)
	{
		m_dragStartedCallback();
	}
}

void ToolbarButton::SetChecked(bool checked)
{
	if (checked == m_checked)
	{
		return;
	}

	m_checked = checked;

	NotifyParentOfUpdate();
}

bool ToolbarButton::GetChecked() const
{
	return m_checked;
}

void ToolbarButton::NotifyParentOfUpdate() const
{
	if (m_parent)
	{
		m_parent->UpdateButton(this);
	}
}

ToolbarMenuButton::ToolbarMenuButton(MouseEventCallback clickedCallback) :
	ToolbarButton(std::bind_front(&ToolbarMenuButton::OnMenuButtonClicked, this)),
	m_clickedCallback(clickedCallback)
{
}

void ToolbarMenuButton::OnMenuButtonClicked(const MouseEvent &event)
{
	// While the menu is shown, the button will be checked. This simply results in the button being
	// displayed in the pressed state. The reason the button isn't simply put into the pressed state
	// directly is that the pressed state changes with user interaction, whereas the checked state
	// doesn't. That is, a button marked as checked will stay checked until it's manually marked as
	// not checked (which fits better with the way toolbar buttons are represented here).
	SetChecked(true);
	m_clickedCallback(event);
	SetChecked(false);

	m_menuLastClosedTime = std::chrono::steady_clock::now();
}

bool ToolbarMenuButton::ShouldSuppressClick() const
{
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	return (now - m_menuLastClosedTime) <= MINIMUM_TIME_BETWEEN_CLICKS;
}

ToolbarView::ToolbarView(HWND parent, DWORD style, DWORD extendedStyle, const Config *config) :
	m_hwnd(CreateToolbar(parent, style, extendedStyle)),
	m_fontSetter(m_hwnd, config)
{
	auto tooltipControl = reinterpret_cast<HWND>(SendMessage(m_hwnd, TB_GETTOOLTIPS, 0, 0));

	if (tooltipControl)
	{
		m_tooltipFontSetter = std::make_unique<MainFontSetter>(tooltipControl, config);
	}

	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&ToolbarView::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(m_hwnd),
		std::bind_front(&ToolbarView::ParentWndProc, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(std::bind(&ToolbarView::OnFontOrDpiUpdated, this));
}

void ToolbarView::SetupSmallShellImageList()
{
	HIMAGELIST smallShellImageList;
	BOOL imageListResult = Shell_GetImageLists(nullptr, &smallShellImageList);

	if (imageListResult)
	{
		int iconWidth;
		int iconHeight;
		ImageList_GetIconSize(smallShellImageList, &iconWidth, &iconHeight);
		auto res = SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));
		DCHECK(res);

		res =
			SendMessage(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(smallShellImageList));

		// The image list should only be set a single time.
		DCHECK(!res);
	}
}

HWND ToolbarView::GetHWND() const
{
	return m_hwnd;
}

void ToolbarView::AddButton(std::unique_ptr<ToolbarButton> button, size_t index)
{
	CHECK(index <= m_buttons.size());

	std::wstring text = button->GetText();

	TBBUTTON tbButton = {};
	tbButton.idCommand = m_idCounter;
	tbButton.iBitmap = button->GetImageIndex() ? *button->GetImageIndex() : I_IMAGENONE;
	tbButton.fsState = GetToolbarButtonState(button.get());
	tbButton.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	tbButton.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hwnd, TB_INSERTBUTTON, index, reinterpret_cast<LPARAM>(&tbButton));

	++m_idCounter;

	button->SetParent(this);
	m_buttons.insert(m_buttons.begin() + index, std::move(button));

	m_toolbarSizeUpdatedSignal();
}

void ToolbarView::UpdateButton(const ToolbarButton *button)
{
	auto itr = std::find_if(m_buttons.begin(), m_buttons.end(),
		[button](const auto &currentEntry) { return currentEntry.get() == button; });
	CHECK(itr != m_buttons.end());

	UpdateButton(itr - m_buttons.begin());
}

void ToolbarView::UpdateButton(size_t index)
{
	auto button = GetButtonFromIndex(index);

	std::wstring text = button->GetText();

	TBBUTTONINFO tbButtonInfo = {};
	tbButtonInfo.cbSize = sizeof(tbButtonInfo);
	tbButtonInfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE | TBIF_TEXT | TBIF_STATE;
	tbButtonInfo.iImage = button->GetImageIndex() ? *button->GetImageIndex() : I_IMAGENONE;
	tbButtonInfo.pszText = text.data();
	tbButtonInfo.fsState = GetToolbarButtonState(button);
	SendMessage(m_hwnd, TB_SETBUTTONINFO, index, reinterpret_cast<LPARAM>(&tbButtonInfo));
}

BYTE ToolbarView::GetToolbarButtonState(const ToolbarButton *button)
{
	// All buttons are enabled.
	BYTE state = TBSTATE_ENABLED;

	if (button->GetChecked())
	{
		state |= TBSTATE_CHECKED;
	}

	return state;
}

void ToolbarView::RemoveButton(size_t index)
{
	CHECK(index < m_buttons.size());
	auto itr = m_buttons.begin() + index;
	m_buttons.erase(itr);

	SendMessage(m_hwnd, TB_DELETEBUTTON, index, 0);

	m_toolbarSizeUpdatedSignal();
}

const std::vector<std::unique_ptr<ToolbarButton>> &ToolbarView::GetButtons() const
{
	return m_buttons;
}

ToolbarButton *ToolbarView::GetButton(size_t index) const
{
	CHECK_LT(index, m_buttons.size());
	return m_buttons[index].get();
}

LRESULT ToolbarView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		if (OnLeftButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }))
		{
			return 0;
		}
		break;

	case WM_MOUSEMOVE:
		OnMouseMove(static_cast<int>(wParam), { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_LBUTTONUP:
		OnLeftButtonUp();
		break;

	case WM_MBUTTONUP:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		if (OnMiddleButtonUp(&pt, static_cast<UINT>(wParam)))
		{
			return 0;
		}
	}
	break;

	case WM_DPICHANGED_AFTERPARENT:
		OnFontOrDpiUpdated();
		break;

	case WM_NCDESTROY:
		OnNcDestroy();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ToolbarView::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_CLICK:
				if (OnClick(reinterpret_cast<NMMOUSE *>(lParam)))
				{
					return TRUE;
				}
				break;

			case NM_RCLICK:
				if (OnRightClick(reinterpret_cast<NMMOUSE *>(lParam)))
				{
					return TRUE;
				}
				break;

			case TBN_GETINFOTIP:
				if (OnGetInfoTip(reinterpret_cast<NMTBGETINFOTIP *>(lParam)))
				{
					return 0;
				}
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool ToolbarView::OnLeftButtonDown(const POINT &pt)
{
	auto button = MaybeGetButtonAtPoint(pt);

	if (!button)
	{
		return false;
	}

	// The cursor point saved below will be used to determine whether or not to enter a drag loop
	// when the mouse moves. Note that although there's a TBN_BEGINDRAG message, it appears that
	// it's sent as soon as a mouse button goes down (even if the mouse hasn't moved while the
	// button is down). Therefore, there's not much point using it.
	// Additionally, that event is also sent when the right button goes down, though a drag should
	// only begin when the left button goes down.
	// There is also a TBN_DRAGOUT message, but that's sent only when the mouse goes down over a
	// button and is then moved outside the button. That is, that message isn't based on how far the
	// mouse moves while it's down, but rather, whether the mouse has moved from over the button to
	// outside it. Which isn't really the desired behavior here.
	m_leftButtonDownPoint = pt;

	// Returning true here will prevent the button press animation. The WM_LBUTTONUP and NM_CLICK
	// messages will still be sent, though, which is why ShouldSuppressClick() is also called in the
	// NM_CLICK handler.
	return button->ShouldSuppressClick();
}

void ToolbarView::OnMouseMove(int keys, const POINT &pt)
{
	if (!m_leftButtonDownPoint || !WI_IsFlagSet(keys, MK_LBUTTON))
	{
		m_leftButtonDownPoint.reset();
		return;
	}

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_hwnd);

	RECT rect = { m_leftButtonDownPoint->x, m_leftButtonDownPoint->y, m_leftButtonDownPoint->x,
		m_leftButtonDownPoint->y };
	InflateRect(&rect, dpiCompat.GetSystemMetricsForDpi(SM_CXDRAG, dpi),
		dpiCompat.GetSystemMetricsForDpi(SM_CYDRAG, dpi));

	if (!PtInRect(&rect, pt))
	{
		// m_leftButtonDownPoint will only be set if the mouse was on a button, so retrieving the
		// button here should succeed.
		auto button = MaybeGetButtonAtPoint(*m_leftButtonDownPoint);
		CHECK(button);

		button->OnDragStarted();

		m_leftButtonDownPoint.reset();
	}
}

void ToolbarView::OnLeftButtonUp()
{
	m_leftButtonDownPoint.reset();
}

bool ToolbarView::OnClick(const NMMOUSE *mouseInfo)
{
	if (mouseInfo->dwItemSpec == static_cast<DWORD_PTR>(-1))
	{
		return false;
	}

	auto index = TransformCommandToIndex(static_cast<int>(mouseInfo->dwItemSpec));
	auto button = GetButtonFromIndex(index);

	if (button->ShouldSuppressClick())
	{
		return false;
	}

	button->OnClicked({ mouseInfo->pt, IsKeyDown(VK_SHIFT), IsKeyDown(VK_CONTROL) });

	return true;
}

bool ToolbarView::OnMiddleButtonUp(const POINT *pt, UINT keysDown)
{
	auto button = MaybeGetButtonAtPoint(*pt);

	if (!button)
	{
		return false;
	}

	button->OnMiddleClicked(
		{ *pt, WI_IsFlagSet(keysDown, MK_SHIFT), WI_IsFlagSet(keysDown, MK_CONTROL) });

	return true;
}

bool ToolbarView::OnRightClick(const NMMOUSE *mouseInfo)
{
	if (mouseInfo->dwItemSpec == static_cast<DWORD_PTR>(-1))
	{
		return false;
	}

	auto index = TransformCommandToIndex(static_cast<int>(mouseInfo->dwItemSpec));
	auto button = GetButtonFromIndex(index);

	button->OnRightClicked({ mouseInfo->pt, IsKeyDown(VK_SHIFT), IsKeyDown(VK_CONTROL) });

	return true;
}

bool ToolbarView::OnGetInfoTip(NMTBGETINFOTIP *infoTip)
{
	auto index = TransformCommandToIndex(infoTip->iItem);
	auto button = GetButtonFromIndex(index);

	StringCchCopy(infoTip->pszText, infoTip->cchTextMax, button->GetTooltipText().c_str());

	return true;
}

ToolbarButton *ToolbarView::MaybeGetButtonAtPoint(const POINT &pt)
{
	auto index = MaybeGetIndexOfButtonAtPoint(pt);

	if (!index)
	{
		return nullptr;
	}

	return GetButtonFromIndex(*index);
}

std::optional<size_t> ToolbarView::MaybeGetIndexOfButtonAtPoint(const POINT &pt)
{
	auto index = SendMessage(m_hwnd, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt));

	if (index < 0)
	{
		return std::nullopt;
	}

	return index;
}

size_t ToolbarView::TransformCommandToIndex(int command)
{
	auto index = SendMessage(m_hwnd, TB_COMMANDTOINDEX, command, 0);
	CHECK_NE(index, -1) << "Invalid command";
	return index;
}

ToolbarButton *ToolbarView::GetButtonFromIndex(size_t index)
{
	CHECK_LT(index, m_buttons.size()) << "Invalid index";
	auto itr = m_buttons.begin() + index;
	return itr->get();
}

ToolbarView::DropLocation ToolbarView::GetDropLocation(const POINT &ptScreen)
{
	POINT ptClient = ptScreen;
	BOOL res = ScreenToClient(m_hwnd, &ptClient);

	// ScreenToClient can likely only fail if the window parameter is invalid, which should never be
	// the case here.
	DCHECK(res);

	auto index = MaybeGetIndexOfButtonAtPoint(ptClient);

	if (index)
	{
		return { true, *index };
	}
	else
	{
		return { false, FindNextButtonIndex(ptClient) };
	}
}

void ToolbarView::ShowInsertMark(size_t index)
{
	size_t finalIndex = index;
	DWORD flags = 0;

	if (index == m_buttons.size())
	{
		finalIndex--;
		flags = TBIMHT_AFTER;
	}

	TBINSERTMARK insertMark;
	insertMark.iButton = static_cast<int>(finalIndex);
	insertMark.dwFlags = flags;
	SendMessage(m_hwnd, TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&insertMark));
}

void ToolbarView::RemoveInsertMark()
{
	TBINSERTMARK insertMark;
	insertMark.iButton = -1;
	SendMessage(m_hwnd, TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&insertMark));
}

bool ToolbarView::IsButtonVisible(size_t index) const
{
	RECT toolbarRect;
	GetClientRect(m_hwnd, &toolbarRect);

	RECT buttonRect = GetButtonRect(index);

	return buttonRect.right <= toolbarRect.right;
}

RECT ToolbarView::GetButtonRect(size_t index) const
{
	RECT buttonRect;
	auto res = SendMessage(m_hwnd, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&buttonRect));
	CHECK(res) << "Button not found";
	return buttonRect;
}

// Returns the index of the button that comes after the specified point. If the point
// is past the last button on the toolbar, this index will be one past the last button (or 0 if
// there are no buttons).
size_t ToolbarView::FindNextButtonIndex(const POINT &ptClient) const
{
	size_t nextIndex = 0;

	for (size_t i = 0; i < m_buttons.size(); i++)
	{
		RECT rc = GetButtonRect(i);

		if (ptClient.x < rc.right)
		{
			break;
		}

		nextIndex = i + 1;
	}

	return nextIndex;
}

void ToolbarView::SetHotItem(size_t index)
{
	SendMessage(m_hwnd, TB_SETHOTITEM, index, 0);
}

void ToolbarView::OnFontOrDpiUpdated()
{
	RefreshToolbarAfterFontOrDpiChange(m_hwnd);

	m_toolbarSizeUpdatedSignal();
}

boost::signals2::connection ToolbarView::AddToolbarSizeUpdatedObserver(
	const ToolbarSizeChangedSignal::slot_type &observer)
{
	return m_toolbarSizeUpdatedSignal.connect(observer);
}

boost::signals2::connection ToolbarView::AddWindowDestroyedObserver(
	const WindowDestroyedSignal::slot_type &observer)
{
	return m_windowDestroyedSignal.connect(observer);
}

void ToolbarView::OnNcDestroy()
{
	m_windowDestroyedSignal();

	delete this;
}
