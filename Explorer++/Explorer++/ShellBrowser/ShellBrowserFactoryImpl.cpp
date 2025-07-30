// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserFactoryImpl.h"
#include "ShellBrowserImpl.h"
#include "BrowserWindow.h"

ShellBrowserFactoryImpl::ShellBrowserFactoryImpl(App *app, BrowserWindow *browser,
	FileActionHandler *fileActionHandler) :
	m_app(app),
	m_browser(browser),
	m_fileActionHandler(fileActionHandler)
{
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryImpl::Create(const PidlAbsolute &initialPidl,
	const FolderSettings &folderSettings, const FolderColumns *initialColumns)
{
	return std::make_unique<ShellBrowserImpl>(m_browser->GetHWND(), m_app, m_browser,
		m_fileActionHandler, initialPidl, folderSettings, initialColumns);
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryImpl::CreateFromPreserved(
	const PreservedShellBrowser &preservedShellBrowser)
{
	return std::make_unique<ShellBrowserImpl>(m_browser->GetHWND(), m_app, m_browser,
		m_fileActionHandler, preservedShellBrowser);
}
