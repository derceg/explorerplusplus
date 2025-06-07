// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "../Helper/SignalWrapper.h"
#include <boost/core/noncopyable.hpp>
#include <memory>
#include <vector>

struct Config;
class WindowSubclass;

class TabView : private boost::noncopyable
{
public:
	enum class ScrollDirection
	{
		Left,
		Right
	};

	HWND GetHWND() const;

	void Scroll(ScrollDirection direction);

	// Signals
	SignalWrapper<TabView, void()> sizeUpdatedSignal;
	SignalWrapper<TabView, void()> windowDestroyedSignal;

protected:
	TabView(HWND parent, DWORD style, const Config *config);
	virtual ~TabView();

private:
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMouseWheel(HWND hwnd, int xPos, int yPos, int delta, UINT keys);
	void OnFontUpdated();
	void OnNcDestroy();

	const HWND m_hwnd;
	MainFontSetter m_fontSetter;
	std::unique_ptr<MainFontSetter> m_tooltipFontSetter;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
