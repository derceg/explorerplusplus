// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserWindow.h"
#include "ShellBrowser/ShellBrowser.h"

BrowserWindow::BrowserWindow() : m_id(idCounter++)
{
}

int BrowserWindow::GetId() const
{
	return m_id;
}

bool BrowserWindow::IsShellBrowserActive(const ShellBrowser *shellBrowser) const
{
	return shellBrowser == GetActiveShellBrowser();
}
