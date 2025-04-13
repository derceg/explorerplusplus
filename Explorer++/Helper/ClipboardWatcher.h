// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include <wil/resource.h>
#include <memory>

class WindowSubclass;

// Allows clients to observe clipboard updates, without having to manually listen for
// WM_CLIPBOARDUPDATE.
class ClipboardWatcher
{
public:
	ClipboardWatcher();
	~ClipboardWatcher();

	// Signals
	SignalWrapper<ClipboardWatcher, void()> updateSignal;

private:
	LRESULT Subclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	const wil::unique_hwnd m_hwnd;
	const std::unique_ptr<WindowSubclass> m_windowSubclass;
};
