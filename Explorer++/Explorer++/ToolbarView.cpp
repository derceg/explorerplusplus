// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarView.h"
#include "DarkModeHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"

ToolbarButton::ToolbarButton(ClickedCallback clickedCallback) : m_clickedCallback(clickedCallback)
{
}

void ToolbarButton::SetRightClickedCallback(RightClickedCallback rightClickedCallback)
{
	m_rightClickedCallback = rightClickedCallback;
}

void ToolbarButton::SetMiddleClickedCallback(MiddleClickedCallback middleClickedCallback)
{
	m_middleClickedCallback = middleClickedCallback;
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

ToolbarView::ToolbarView(HWND parent, DWORD style, DWORD extendedStyle) :
	m_hwnd(CreateToolbar(parent, style, extendedStyle))
{
	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&ToolbarView::WndProc, this), SUBCLASS_ID));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(GetParent(m_hwnd),
		std::bind_front(&ToolbarView::ParentWndProc, this), PARENT_SUBCLASS_ID));

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		darkModeHelper.SetDarkModeForToolbarTooltips(m_hwnd);
	}
}

HWND ToolbarView::GetHWND() const
{
	return m_hwnd;
}

void ToolbarView::AddButton(std::unique_ptr<ToolbarButton> button, size_t index)
{
	assert(index <= m_buttons.size());

	std::wstring text = button->GetText();

	TBBUTTON tbButton = {};
	tbButton.idCommand = m_idCounter;
	tbButton.iBitmap = button->GetImageIndex() ? *button->GetImageIndex() : I_IMAGENONE;
	tbButton.fsState = TBSTATE_ENABLED;
	tbButton.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	tbButton.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hwnd, TB_INSERTBUTTON, index, reinterpret_cast<LPARAM>(&tbButton));

	++m_idCounter;

	m_buttons.insert(m_buttons.begin() + index, std::move(button));

	m_toolbarUpdatedSignal();
}

void ToolbarView::UpdateButton(size_t index)
{
	auto button = MaybeGetButtonFromIndex(index);

	if (!button)
	{
		return;
	}

	std::wstring text = button->GetText();

	TBBUTTONINFO tbButtonInfo = {};
	tbButtonInfo.cbSize = sizeof(tbButtonInfo);
	tbButtonInfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE | TBIF_TEXT;
	tbButtonInfo.iImage = button->GetImageIndex() ? *button->GetImageIndex() : I_IMAGENONE;
	tbButtonInfo.pszText = text.data();
	SendMessage(m_hwnd, TB_SETBUTTONINFO, index, reinterpret_cast<LPARAM>(&tbButtonInfo));
}

void ToolbarView::RemoveButton(size_t index)
{
	assert(index < m_buttons.size());
	auto itr = m_buttons.begin() + index;
	m_buttons.erase(itr);

	SendMessage(m_hwnd, TB_DELETEBUTTON, index, 0);

	m_toolbarUpdatedSignal();
}

LRESULT ToolbarView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
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

bool ToolbarView::OnClick(const NMMOUSE *nmm)
{
	if (nmm->dwItemSpec == -1)
	{
		return false;
	}

	auto index = SendMessage(m_hwnd, TB_COMMANDTOINDEX, nmm->dwItemSpec, 0);

	if (index == -1)
	{
		return false;
	}

	auto button = MaybeGetButtonFromIndex(index);

	if (!button)
	{
		return false;
	}

	button->OnClicked({ nmm->pt, IsKeyDown(VK_SHIFT), IsKeyDown(VK_CONTROL) });

	return true;
}

bool ToolbarView::OnMiddleButtonUp(const POINT *pt, UINT keysDown)
{
	auto index = SendMessage(m_hwnd, TB_HITTEST, 0, reinterpret_cast<LPARAM>(pt));

	if (index < 0)
	{
		return false;
	}

	auto button = MaybeGetButtonFromIndex(index);

	if (!button)
	{
		return false;
	}

	button->OnMiddleClicked(
		{ *pt, WI_IsFlagSet(keysDown, MK_SHIFT), WI_IsFlagSet(keysDown, MK_CONTROL) });

	return true;
}

bool ToolbarView::OnRightClick(const NMMOUSE *nmm)
{
	if (nmm->dwItemSpec == -1)
	{
		return false;
	}

	auto index = SendMessage(m_hwnd, TB_COMMANDTOINDEX, nmm->dwItemSpec, 0);

	if (index == -1)
	{
		return false;
	}

	auto button = MaybeGetButtonFromIndex(index);

	if (!button)
	{
		return false;
	}

	button->OnRightClicked({ nmm->pt, IsKeyDown(VK_SHIFT), IsKeyDown(VK_CONTROL) });

	return true;
}

bool ToolbarView::OnGetInfoTip(NMTBGETINFOTIP *infoTip)
{
	auto index = SendMessage(m_hwnd, TB_COMMANDTOINDEX, infoTip->iItem, 0);

	if (index == -1)
	{
		return false;
	}

	auto button = MaybeGetButtonFromIndex(index);

	if (!button)
	{
		return false;
	}

	StringCchCopy(infoTip->pszText, infoTip->cchTextMax, button->GetTooltipText().c_str());

	return true;
}

ToolbarButton *ToolbarView::MaybeGetButtonFromIndex(size_t index)
{
	if (index >= m_buttons.size())
	{
		return nullptr;
	}

	auto itr = m_buttons.begin() + index;
	return itr->get();
}

boost::signals2::connection ToolbarView::AddToolbarUpdatedObserver(
	const ToolbarUpdatedSignal::slot_type &observer)
{
	return m_toolbarUpdatedSignal.connect(observer);
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
