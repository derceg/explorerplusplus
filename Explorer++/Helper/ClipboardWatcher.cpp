// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardWatcher.h"
#include "MessageWindowHelper.h"
#include "WindowSubclass.h"

ClipboardWatcher::ClipboardWatcher() :
	m_hwnd(MessageWindowHelper::CreateMessageOnlyWindow()),
	m_windowSubclass(std::make_unique<WindowSubclass>(m_hwnd.get(),
		std::bind_front(&ClipboardWatcher::Subclass, this)))
{
	auto res = AddClipboardFormatListener(m_hwnd.get());
	DCHECK(res);
}

ClipboardWatcher::~ClipboardWatcher()
{
	auto res = RemoveClipboardFormatListener(m_hwnd.get());
	DCHECK(res);
}

LRESULT ClipboardWatcher::Subclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLIPBOARDUPDATE:
		updateSignal.m_signal();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
