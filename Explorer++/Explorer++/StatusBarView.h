// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "ViewBase.h"
#include <vector>

struct Config;

class StatusBarView : public ViewBase
{
public:
	StatusBarView(HWND parent, const Config *config);

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
	LRESULT StatusBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// This function should be called whenever the font is updated for the status bar control.
	void UpdateMinHeight();

	static bool VerifyParts(const std::vector<int> &parts);
	void UpdateControlParts();

	MainFontSetter m_fontSetter;
	std::vector<int> m_parts;
};
