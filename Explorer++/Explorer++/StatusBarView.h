// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "../Helper/SignalWrapper.h"
#include <memory>
#include <vector>

struct Config;
class WindowSubclass;

class StatusBarView
{
public:
	// Signals
	SignalWrapper<StatusBarView, void()> windowDestroyedSignal;

	static StatusBarView *Create(HWND parent, const Config *config);

	HWND GetHWND() const;

	// The parts are specified here as a list of integer values between 1 and 100, with each value
	// indicating how far along the status bar the associated part ends. For example, if the values
	// specified were:
	//
	// 25, 40, 100
	//
	// The parts would span:
	//
	// 0% - 25%
	// 25% - 40%
	// 40% - 100%
	void SetParts(const std::vector<int> &parts);

	void SetPartText(int partIndex, const std::wstring &text);

private:
	StatusBarView(HWND parent, const Config *config);

	LRESULT StatusBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// This function should be called whenever the font is updated for the status bar control.
	void UpdateMinHeight();

	static bool VerifyParts(const std::vector<int> &parts);
	void UpdateControlParts();

	void OnNcDestroy();

	const HWND m_hwnd;
	MainFontSetter m_fontSetter;
	std::vector<int> m_parts;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
