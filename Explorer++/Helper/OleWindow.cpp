// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OleWindow.h"

OleWindow::OleWindow(GetWindowCallback getWindowCallback) :
	m_getWindowCallback(std::move(getWindowCallback))
{
}

IFACEMETHODIMP OleWindow::GetWindow(HWND *outputHwnd)
{
	HWND hwnd = m_getWindowCallback();

	if (!hwnd)
	{
		return E_FAIL;
	}

	*outputHwnd = hwnd;

	return S_OK;
}

IFACEMETHODIMP OleWindow::ContextSensitiveHelp(BOOL enterMode)
{
	UNREFERENCED_PARAMETER(enterMode);

	return E_NOTIMPL;
}
