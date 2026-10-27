// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuBase.h"
#include "PopupMenuView.h"
#include "ToolbarView.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/WindowSubclass.h"
#include <glog/logging.h>
#include <wil/common.h>
#include <windowsx.h>

ToolbarButton::ToolbarButton(MouseEventCallback clickedCallback) :
	m_clickedCallback(std::move(clickedCallback))
{
}

void ToolbarButton::SetParent(ToolbarViewBase *parent)
{
	m_parent = parent;
}

bool ToolbarButton::ShouldShowText() const
{
	return true;
}

bool ToolbarButton::IsDropDownButton() const
{
	return false;
}

ToolbarButton::MenuFactory ToolbarButton::MaybeGetMenuFactory() const
{
	return {};
}

bool ToolbarButton::ShouldSuppressClick() const
{
	return false;
}

void ToolbarButton::SetRightClickedCallback(MouseEventCallback rightClickedCallback)
{
	m_rightClickedCallback = std::move(rightClickedCallback);
}

void ToolbarButton::SetMiddleClickedCallback(MouseEventCallback middleClickedCallback)
{
	m_middleClickedCallback = std::move(middleClickedCallback);
}

void ToolbarButton::SetDragStartedCallback(DragStartedCallback dragStartedCallback)
{
	m_dragStartedCallback = std::move(dragStartedCallback);
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

bool ToolbarButton::IsEnabled() const
{
	return true;
}

bool ToolbarButton::IsChecked() const
{
	return false;
}

void ToolbarButton::NotifyParentOfUpdate() const
{
	if (m_parent)
	{
		m_parent->UpdateButton(this);
	}
}

WeakPtr<ToolbarButton> ToolbarButton::GetWeakPtr()
{
	return m_weakPtrFactory.GetWeakPtr();
}

ToolbarMenuButton::ToolbarMenuButton(MouseEventCallback clickedCallback) :
	ToolbarButton(std::bind_front(&ToolbarMenuButton::OnMenuButtonClicked, this)),
	m_clickedCallback(std::move(clickedCallback))
{
}

void ToolbarMenuButton::OnMenuButtonClicked(const MouseEvent &event)
{
	// While the menu is shown, the button will be checked. This simply results in the button being
	// displayed in the pressed state. The reason the button isn't simply put into the pressed state
	// directly is that the pressed state changes with user interaction, whereas the checked state
	// doesn't. That is, a button marked as checked will stay checked until it's manually marked as
	// not checked (which fits better with the way toolbar buttons are represented here).
	SetMenuOpen(true);
	m_clickedCallback(event);
	SetMenuOpen(false);

	m_menuLastClosedTime = std::chrono::steady_clock::now();
}

void ToolbarMenuButton::SetMenuOpen(bool open)
{
	m_menuOpen = open;

	NotifyParentOfUpdate();
}

bool ToolbarMenuButton::ShouldSuppressClick() const
{
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	return (now - m_menuLastClosedTime) <= MINIMUM_TIME_BETWEEN_CLICKS;
}

bool ToolbarMenuButton::IsChecked() const
{
	return m_menuOpen;
}

ToolbarItem::ToolbarItem(ToolbarButton *button) : m_button(button)
{
}

ToolbarItem ToolbarItem::Button(ToolbarButton *button)
{
	CHECK(button);
	return ToolbarItem(button);
}

ToolbarItem ToolbarItem::Separator()
{
	return ToolbarItem(nullptr);
}

bool ToolbarItem::IsButton() const
{
	return m_button != nullptr;
}

bool ToolbarItem::IsSeparator() const
{
	return m_button == nullptr;
}

ToolbarButton *ToolbarItem::GetButton() const
{
	return m_button;
}

ToolbarViewBase::ToolbarViewBase(HWND parent, DWORD style, DWORD extendedStyle,
	MenuHelpTextHost *menuHelpTextHost, const Config *config) :
	ViewBase(CreateToolbar(parent, style, extendedStyle)),
	m_menuHelpTextHost(menuHelpTextHost),
	m_fontSetter(GetHWND(), config)
{
	auto tooltipControl = reinterpret_cast<HWND>(SendMessage(GetHWND(), TB_GETTOOLTIPS, 0, 0));

	if (tooltipControl)
	{
		m_tooltipFontSetter = std::make_unique<MainFontSetter>(tooltipControl, config);
	}

	SendMessage(GetHWND(), TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	AddSubclass(std::make_unique<WindowSubclass>(GetHWND(),
		std::bind_front(&ToolbarViewBase::WndProc, this)));
	AddSubclass(std::make_unique<WindowSubclass>(parent,
		std::bind_front(&ToolbarViewBase::ParentWndProc, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(
		std::bind(&ToolbarViewBase::OnFontOrDpiUpdated, this));
}

HWND ToolbarViewBase::GetHWND() const
{
	return ViewBase::GetHWND();
}

UINT ToolbarViewBase::GetHeight() const
{
	SIZE size;
	auto res = SendMessage(GetHWND(), TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&size));
	CHECK(res);

	return size.cy;
}

std::optional<UINT> ToolbarViewBase::MaybeGetIdealWidth() const
{
	SIZE size;
	auto res = SendMessage(GetHWND(), TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&size));
	CHECK(res);

	return size.cx;
}

void ToolbarViewBase::SetupSmallShellImageList()
{
	HIMAGELIST smallShellImageList;
	auto res = Shell_GetImageLists(nullptr, &smallShellImageList);
	CHECK(res);

	SetImageList(smallShellImageList);
}

// Note that if an image list is replaced, the image indexes for each of the buttons should remain
// the same.
void ToolbarViewBase::SetImageList(HIMAGELIST imageList)
{
	int iconWidth;
	int iconHeight;
	auto iconSizeRes = ImageList_GetIconSize(imageList, &iconWidth, &iconHeight);
	CHECK(iconSizeRes);

	auto res = SendMessage(GetHWND(), TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));
	CHECK(res);

	SendMessage(GetHWND(), TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList));

	SendMessage(GetHWND(), TB_AUTOSIZE, 0, 0);

	RefreshAfterSizeChange();
	NotifySizeRequirementsChanged();
}

HIMAGELIST ToolbarViewBase::MaybeGetImageList() const
{
	return reinterpret_cast<HIMAGELIST>(SendMessage(GetHWND(), TB_GETIMAGELIST, 0, 0));
}

void ToolbarViewBase::AddButton(int id, const ToolbarButton *button, size_t index)
{
	CHECK(SendMessage(GetHWND(), TB_COMMANDTOINDEX, id, 0) == -1);
	CHECK(index <= GetNumButtons());

	std::wstring text = button->GetText();

	auto tbButton = BuildNativeButtonStruct(id, button);
	tbButton.iString = reinterpret_cast<INT_PTR>(text.c_str());
	auto res = SendMessage(GetHWND(), TB_INSERTBUTTON, index, reinterpret_cast<LPARAM>(&tbButton));
	CHECK(res);

	NotifySizeRequirementsChanged();
}

TBBUTTON ToolbarViewBase::BuildNativeButtonStruct(int id, const ToolbarButton *button) const
{
	BYTE style = BTNS_AUTOSIZE | BTNS_NOPREFIX;

	if (button->ShouldShowText())
	{
		WI_SetFlag(style, BTNS_SHOWTEXT);
	}

	if (button->IsDropDownButton())
	{
		WI_SetFlag(style, BTNS_DROPDOWN);
	}

	TBBUTTON tbButton = {};
	tbButton.idCommand = id;
	tbButton.iBitmap = button->GetImageIndex() ? *button->GetImageIndex() : I_IMAGENONE;
	tbButton.fsState = GetToolbarButtonState(button);
	tbButton.fsStyle = style;
	return tbButton;
}

void ToolbarViewBase::AddSeparator(size_t index)
{
	CHECK(index <= GetNumButtons());

	TBBUTTON tbButton = {};
	tbButton.fsStyle = BTNS_SEP;
	auto res = SendMessage(GetHWND(), TB_INSERTBUTTON, index, reinterpret_cast<LPARAM>(&tbButton));
	CHECK(res);

	NotifySizeRequirementsChanged();
}

void ToolbarViewBase::UpdateButton(const ToolbarButton *button)
{
	auto index = MaybeGetButtonIndex(button);

	if (!index)
	{
		// If the toolbar is customizable, the button won't necessarily exist in the control.
		return;
	}

	std::wstring text = button->GetText();

	TBBUTTONINFO tbButtonInfo = {};
	tbButtonInfo.cbSize = sizeof(tbButtonInfo);
	tbButtonInfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE | TBIF_TEXT | TBIF_STATE;
	tbButtonInfo.iImage = button->GetImageIndex() ? *button->GetImageIndex() : I_IMAGENONE;
	tbButtonInfo.pszText = text.data();
	tbButtonInfo.fsState = GetToolbarButtonState(button);
	auto res =
		SendMessage(GetHWND(), TB_SETBUTTONINFO, *index, reinterpret_cast<LPARAM>(&tbButtonInfo));
	CHECK(res);

	NotifySizeRequirementsChanged();
}

std::optional<size_t> ToolbarViewBase::MaybeGetButtonIndex(const ToolbarButton *button) const
{
	size_t numButtons = GetNumButtons();

	for (size_t i = 0; i < numButtons; i++)
	{
		auto item = GetItemAtIndex(i);

		if (item.IsButton() && item.GetButton() == button)
		{
			return i;
		}
	}

	return std::nullopt;
}

BYTE ToolbarViewBase::GetToolbarButtonState(const ToolbarButton *button)
{
	BYTE state = 0;

	if (button->IsEnabled())
	{
		WI_SetFlag(state, TBSTATE_ENABLED);
	}

	if (button->IsChecked())
	{
		WI_SetFlag(state, TBSTATE_CHECKED);
	}

	return state;
}

LRESULT ToolbarViewBase::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
		if (OnMiddleButtonUp(static_cast<int>(wParam),
				{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }))
		{
			return 0;
		}
		break;

	case WM_DPICHANGED_AFTERPARENT:
		OnFontOrDpiUpdated();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ToolbarViewBase::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == GetHWND())
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

			case TBN_DROPDOWN:
				return OnDropDown(reinterpret_cast<NMTOOLBAR *>(lParam));
			}

			if (auto result = MaybeHandleNotification(reinterpret_cast<LPNMHDR>(lParam)))
			{
				return *result;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

std::optional<LRESULT> ToolbarViewBase::MaybeHandleNotification(NMHDR *notification)
{
	UNREFERENCED_PARAMETER(notification);

	return std::nullopt;
}

bool ToolbarViewBase::OnLeftButtonDown(const POINT &ptClient)
{
	auto button = MaybeGetButtonAtPoint(ptClient);

	if (!button)
	{
		return false;
	}

	// The cursor point saved below will be used to determine whether or not to enter a drag loop
	// when the mouse moves. Note that although there's a TBN_BEGINDRAG message, it appears that
	// it's sent as soon as a mouse button goes down (even if the mouse hasn't moved while the
	// button is down). Therefore, there's not much point using it.
	//
	// Additionally, that event is also sent when the right button goes down, though a drag should
	// only begin when the left button goes down.
	//
	// There is also a TBN_DRAGOUT message, but that's sent only when the mouse goes down over a
	// button and is then moved outside the button. That is, that message isn't based on how far the
	// mouse moves while it's down, but rather, whether the mouse has moved from over the button to
	// outside it. Which isn't really the desired behavior here.
	m_leftButtonDownPoint = ptClient;

	// Returning true here will prevent the button press animation. The WM_LBUTTONUP and NM_CLICK
	// messages will still be sent, though, which is why ShouldSuppressClick() is also called in the
	// NM_CLICK handler.
	return button->ShouldSuppressClick();
}

void ToolbarViewBase::OnMouseMove(int keys, const POINT &ptClient)
{
	if (!m_leftButtonDownPoint || !WI_IsFlagSet(keys, MK_LBUTTON))
	{
		m_leftButtonDownPoint.reset();
		return;
	}

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(GetHWND());

	RECT rect = { m_leftButtonDownPoint->x, m_leftButtonDownPoint->y, m_leftButtonDownPoint->x,
		m_leftButtonDownPoint->y };
	InflateRect(&rect, dpiCompat.GetSystemMetricsForDpi(SM_CXDRAG, dpi),
		dpiCompat.GetSystemMetricsForDpi(SM_CYDRAG, dpi));

	if (!PtInRect(&rect, ptClient))
	{
		// m_leftButtonDownPoint will only be set if the mouse was on a button, so retrieving the
		// button here should succeed.
		auto button = MaybeGetButtonAtPoint(*m_leftButtonDownPoint);
		CHECK(button);

		m_leftButtonDownPoint.reset();
		button->OnDragStarted();
	}
}

void ToolbarViewBase::OnLeftButtonUp()
{
	m_leftButtonDownPoint.reset();
}

bool ToolbarViewBase::OnClick(const NMMOUSE *mouseInfo)
{
	if (mouseInfo->dwItemSpec == static_cast<DWORD_PTR>(-1))
	{
		return false;
	}

	auto *button = GetButtonById(static_cast<int>(mouseInfo->dwItemSpec));

	if (button->ShouldSuppressClick())
	{
		return false;
	}

	POINT ptScreen = mouseInfo->pt;
	auto res = ClientToScreen(GetHWND(), &ptScreen);
	CHECK(res);

	button->OnClicked({ ptScreen, IsKeyDown(VK_SHIFT), IsKeyDown(VK_CONTROL) });

	return true;
}

bool ToolbarViewBase::OnMiddleButtonUp(int keys, const POINT &ptClient)
{
	auto button = MaybeGetButtonAtPoint(ptClient);

	if (!button || !button->IsEnabled())
	{
		return false;
	}

	POINT ptScreen = ptClient;
	auto res = ClientToScreen(GetHWND(), &ptScreen);
	CHECK(res);

	button->OnMiddleClicked(
		{ ptScreen, WI_IsFlagSet(keys, MK_SHIFT), WI_IsFlagSet(keys, MK_CONTROL) });

	return true;
}

bool ToolbarViewBase::OnRightClick(const NMMOUSE *mouseInfo)
{
	if (mouseInfo->dwItemSpec == static_cast<DWORD_PTR>(-1))
	{
		return false;
	}

	auto *button = GetButtonById(static_cast<int>(mouseInfo->dwItemSpec));

	if (!button->IsEnabled())
	{
		return true;
	}

	POINT ptScreen = mouseInfo->pt;
	auto res = ClientToScreen(GetHWND(), &ptScreen);
	CHECK(res);

	button->OnRightClicked({ ptScreen, IsKeyDown(VK_SHIFT), IsKeyDown(VK_CONTROL) });

	return true;
}

bool ToolbarViewBase::OnGetInfoTip(NMTBGETINFOTIP *infoTip)
{
	auto *button = GetButtonById(infoTip->iItem);
	StringCchCopy(infoTip->pszText, infoTip->cchTextMax, button->GetTooltipText().c_str());
	return true;
}

LRESULT ToolbarViewBase::OnDropDown(const NMTOOLBAR *data)
{
	auto *button = GetButtonById(data->iItem);
	auto menuFactory = button->MaybeGetMenuFactory();

	if (!menuFactory)
	{
		return TBDDRET_NODEFAULT;
	}

	auto buttonRect = GetButtonRect(button);

	POINT pt = { buttonRect.left, buttonRect.bottom };
	auto res = ClientToScreen(GetHWND(), &pt);
	CHECK(res);

	PopupMenuView popupMenu(GetHWND(), m_menuHelpTextHost);
	auto menu = menuFactory(&popupMenu);
	popupMenu.Show(pt);

	return TBDDRET_DEFAULT;
}

ToolbarButton *ToolbarViewBase::MaybeGetButtonAtPoint(const POINT &ptClient) const
{
	auto index = MaybeGetIndexOfButtonAtPoint(ptClient);

	if (!index)
	{
		return nullptr;
	}

	auto item = GetItemAtIndex(*index);

	if (item.IsSeparator())
	{
		return nullptr;
	}

	return item.GetButton();
}

std::optional<size_t> ToolbarViewBase::MaybeGetIndexOfButtonAtPoint(const POINT &ptClient) const
{
	auto index = SendMessage(GetHWND(), TB_HITTEST, 0, reinterpret_cast<LPARAM>(&ptClient));

	if (index < 0)
	{
		return std::nullopt;
	}

	return index;
}

ToolbarViewBase::DropLocation ToolbarViewBase::GetDropLocation(const POINT &ptScreen) const
{
	POINT ptClient = ptScreen;
	auto res = ScreenToClient(GetHWND(), &ptClient);
	CHECK(res);

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

void ToolbarViewBase::ShowInsertMark(size_t index)
{
	size_t numButtons = GetNumButtons();
	size_t finalIndex = index;
	DWORD flags = 0;

	if (numButtons > 0 && index == numButtons)
	{
		finalIndex--;
		flags = TBIMHT_AFTER;
	}

	TBINSERTMARK insertMark;
	insertMark.iButton = static_cast<int>(finalIndex);
	insertMark.dwFlags = flags;
	SendMessage(GetHWND(), TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&insertMark));
}

void ToolbarViewBase::RemoveInsertMark()
{
	TBINSERTMARK insertMark = {};
	insertMark.iButton = -1;
	SendMessage(GetHWND(), TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&insertMark));
}

std::vector<ToolbarItem> ToolbarViewBase::GetItems() const
{
	std::vector<ToolbarItem> items;
	size_t numButtons = GetNumButtons();

	for (size_t i = 0; i < numButtons; i++)
	{
		items.push_back(GetItemAtIndex(i));
	}

	return items;
}

std::vector<ToolbarItem> ToolbarViewBase::GetOverflowItems() const
{
	std::vector<ToolbarItem> items;
	size_t numButtons = GetNumButtons();

	for (size_t i = 0; i < numButtons; i++)
	{
		if (IsItemVisible(i))
		{
			continue;
		}

		items.push_back(GetItemAtIndex(i));
	}

	return items;
}

ToolbarItem ToolbarViewBase::GetItemAtIndex(size_t index) const
{
	TBBUTTON tbButton = {};
	auto res = SendMessage(GetHWND(), TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton));
	CHECK(res);

	if (WI_IsFlagSet(tbButton.fsStyle, BTNS_SEP))
	{
		return ToolbarItem::Separator();
	}

	return ToolbarItem::Button(GetButtonById(tbButton.idCommand));
}

bool ToolbarViewBase::IsButtonVisible(const ToolbarButton *button) const
{
	auto index = MaybeGetButtonIndex(button);
	CHECK(index);
	return IsItemVisible(*index);
}

RECT ToolbarViewBase::GetButtonRect(const ToolbarButton *button) const
{
	auto index = MaybeGetButtonIndex(button);
	CHECK(index);
	return GetItemRect(*index);
}

bool ToolbarViewBase::IsItemVisible(size_t index) const
{
	RECT toolbarRect;
	GetClientRect(GetHWND(), &toolbarRect);

	RECT buttonRect = GetItemRect(index);

	return buttonRect.right <= toolbarRect.right;
}

RECT ToolbarViewBase::GetItemRect(size_t index) const
{
	RECT buttonRect;
	auto res = SendMessage(GetHWND(), TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&buttonRect));
	CHECK(res) << "Button not found";
	return buttonRect;
}

// Returns the index of the button that comes after the specified point. If the point
// is past the last button on the toolbar, this index will be one past the last button (or 0 if
// there are no buttons).
size_t ToolbarViewBase::FindNextButtonIndex(const POINT &ptClient) const
{
	size_t numButtons = GetNumButtons();
	size_t nextIndex = 0;

	for (size_t i = 0; i < numButtons; i++)
	{
		RECT rc = GetItemRect(i);

		if (ptClient.x < rc.right)
		{
			break;
		}

		nextIndex = i + 1;
	}

	return nextIndex;
}

void ToolbarViewBase::SetHotItem(const ToolbarButton *button)
{
	auto index = MaybeGetButtonIndex(button);
	CHECK(index);
	SendMessage(GetHWND(), TB_SETHOTITEM, *index, 0);
}

size_t ToolbarViewBase::GetNumButtons() const
{
	return SendMessage(GetHWND(), TB_BUTTONCOUNT, 0, 0);
}

void ToolbarViewBase::OnFontOrDpiUpdated()
{
	RefreshAfterSizeChange();
	NotifySizeRequirementsChanged();
}

// It appears that changing the font size in a toolbar doesn't result in the layout being correctly
// updated. For example, the width of a button won't change, which will cause the text for the
// button to be cut off if the font size is increased. Also, the toolbar height doesn't always seen
// to get calculated correctly. That means that changing from one font size to another can result in
// the toolbar reporting the original height, even if that's not enough to accommodate the new font.
// Similar problems also exist when the DPI changes or when the image list is updated. Button text
// may end up being cut off and the height of the toolbar may not be correct.
//
// These issues can be worked around by deleting all buttons in the toolbar and reinserting them.
void ToolbarViewBase::RefreshAfterSizeChange()
{
	ScopedRedrawDisabler redrawDisabler(GetHWND());

	struct SavedItem
	{
		ToolbarItem item;
		int id;
	};

	std::vector<SavedItem> savedItems;
	size_t numButtons = GetNumButtons();

	for (size_t i = 0; i < numButtons; i++)
	{
		TBBUTTON tbButton = {};
		auto res = SendMessage(GetHWND(), TB_GETBUTTON, 0, reinterpret_cast<LPARAM>(&tbButton));
		CHECK(res);

		if (WI_IsFlagSet(tbButton.fsStyle, BTNS_SEP))
		{
			savedItems.push_back({ ToolbarItem::Separator(), 0 });
		}
		else
		{
			savedItems.push_back(
				{ ToolbarItem::Button(GetButtonById(tbButton.idCommand)), tbButton.idCommand });
		}

		res = SendMessage(GetHWND(), TB_DELETEBUTTON, 0, 0);
		CHECK(res);
	}

	size_t index = 0;

	for (const auto &savedItem : savedItems)
	{
		if (savedItem.item.IsButton())
		{
			AddButton(savedItem.id, savedItem.item.GetButton(), index);
		}
		else
		{
			AddSeparator(index);
		}

		index++;
	}
}
