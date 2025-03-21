// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddressBarView.h"
#include "AddressBarDelegate.h"
#include "CoreInterface.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <Shlwapi.h>

AddressBarView *AddressBarView::Create(HWND parent, const Config *config,
	CoreInterface *coreInterface)
{
	return new AddressBarView(parent, config, coreInterface);
}

AddressBarView::AddressBarView(HWND parent, const Config *config, CoreInterface *coreInterface) :
	m_hwnd(CreateAddressBar(parent)),
	m_fontSetter(m_hwnd, config),
	m_coreInterface(coreInterface)
{
	HIMAGELIST smallIcons;
	BOOL res = Shell_GetImageLists(nullptr, &smallIcons);
	CHECK(res);
	SendMessage(m_hwnd, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(smallIcons));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&AddressBarView::ComboBoxExSubclass, this)));

	auto edit = GetEditControl();
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(edit,
		std::bind_front(&AddressBarView::EditSubclass, this)));

	HRESULT hr = SHAutoComplete(edit, SHACF_FILESYSTEM | SHACF_AUTOSUGGEST_FORCE_ON);
	DCHECK(SUCCEEDED(hr));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(parent,
		std::bind_front(&AddressBarView::ParentSubclass, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(
		std::bind(&AddressBarView::OnFontOrDpiUpdated, this));
}

HWND AddressBarView::CreateAddressBar(HWND parent)
{
	return CreateWindowEx(WS_EX_TOOLWINDOW, WC_COMBOBOXEX, L"",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CLIPSIBLINGS
			| WS_CLIPCHILDREN,
		0, 0, 0, 0, parent, nullptr, GetModuleHandle(nullptr), nullptr);
}

LRESULT AddressBarView::ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DPICHANGED_AFTERPARENT:
		OnFontOrDpiUpdated();
		break;

	case WM_NCDESTROY:
		OnNcDestroy();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT AddressBarView::EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (m_delegate && m_delegate->OnKeyPressed(static_cast<UINT>(wParam)))
		{
			return 0;
		}
		break;

	case WM_SETFOCUS:
		m_coreInterface->FocusChanged();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT AddressBarView::ParentSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case CBEN_DRAGBEGIN:
				if (m_delegate)
				{
					m_delegate->OnBeginDrag();
				}
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void AddressBarView::SetDelegate(AddressBarDelegate *delegate)
{
	m_delegate = delegate;
}

HWND AddressBarView::GetHWND() const
{
	return m_hwnd;
}

std::wstring AddressBarView::GetText() const
{
	return GetWindowString(m_hwnd);
}

bool AddressBarView::IsTextModified() const
{
	return SendMessage(GetEditControl(), EM_GETMODIFY, 0, 0);
}

void AddressBarView::SelectAllText()
{
	SendMessage(GetEditControl(), EM_SETSEL, 0, -1);
}

void AddressBarView::UpdateTextAndIcon(const std::optional<std::wstring> &optionalText,
	int iconIndex)
{
	COMBOBOXEXITEM cbItem = {};
	cbItem.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_INDENT;
	cbItem.iItem = -1;
	cbItem.iImage = iconIndex;
	cbItem.iSelectedImage = iconIndex;
	cbItem.iIndent = 1;

	if (optionalText)
	{
		WI_SetFlag(cbItem.mask, CBEIF_TEXT);
		cbItem.pszText = const_cast<LPWSTR>(optionalText->c_str());

		m_currentText = *optionalText;
	}

	auto res = SendMessage(m_hwnd, CBEM_SETITEM, 0, reinterpret_cast<LPARAM>(&cbItem));
	DCHECK(res);
}

void AddressBarView::RevertText()
{
	SendMessage(m_hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(m_currentText.c_str()));
}

HWND AddressBarView::GetEditControl() const
{
	return reinterpret_cast<HWND>(SendMessage(m_hwnd, CBEM_GETEDITCONTROL, 0, 0));
}

void AddressBarView::OnFontOrDpiUpdated()
{
	sizeUpdatedSignal.m_signal();
}

void AddressBarView::OnNcDestroy()
{
	windowDestroyedSignal.m_signal();

	delete this;
}
