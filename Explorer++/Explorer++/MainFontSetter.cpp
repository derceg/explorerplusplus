// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainFontSetter.h"
#include "Config.h"
#include "FontHelper.h"
#include "../Helper/WindowSubclassWrapper.h"

MainFontSetter::MainFontSetter(HWND hwnd, const Config *config,
	std::optional<LOGFONT> defaultFont) :
	m_hwnd(hwnd),
	m_config(config),
	m_defaultFont(defaultFont)
{
	MaybeSubclassWindow();
	UpdateFont();

	m_connections.push_back(
		m_config->mainFont.addObserver(std::bind(&MainFontSetter::UpdateFont, this)));
}

void MainFontSetter::MaybeSubclassWindow()
{
	WCHAR className[256];
	auto res = GetClassName(m_hwnd, className, static_cast<int>(std::size(className)));

	if (res == 0)
	{
		assert(false);
		return;
	}

	if (lstrcmp(className, TOOLTIPS_CLASS) == 0)
	{
		m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
			std::bind_front(&MainFontSetter::TooltipWndProc, this)));
	}
}

LRESULT MainFontSetter::TooltipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_THEMECHANGED:
	{
		auto res = DefSubclassProc(hwnd, msg, wParam, lParam);

		// The tooltip control will internally reset its font when it receives a WM_THEMECHANGED
		// message. Therefore, if a custom font is currently active within the application, that
		// font will need to be restored. If a custom font isn't active, the control can use the
		// default font it sets.
		if (m_font)
		{
			SendMessage(m_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(m_font.get()), true);
		}

		return res;
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void MainFontSetter::UpdateFont()
{
	// If a custom font isn't active, the default value (nullptr) will be passed into
	// WM_SETFONT, which will reset the font back to the default system font.
	wil::unique_hfont updatedFont;

	auto &mainFont = m_config->mainFont.get();

	if (mainFont)
	{
		updatedFont = CreateFontFromNameAndSize(mainFont->name, mainFont->size, m_hwnd);

		if (!updatedFont)
		{
			assert(false);
			return;
		}
	}
	else if (m_defaultFont)
	{
		updatedFont.reset(CreateFontIndirect(&*m_defaultFont));

		if (!updatedFont)
		{
			assert(false);
			return;
		}
	}

	SendMessage(m_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(updatedFont.get()), true);

	m_font = std::move(updatedFont);

	fontUpdatedSignal.m_signal();
}
