// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserFactoryImpl.h"
#include "ShellBrowserImpl.h"
#include "BrowserWindow.h"

ShellBrowserFactoryImpl::ShellBrowserFactoryImpl(BrowserWindow *browser, AppServices *appServices,
	HINSTANCE resourceInstance) :
	m_browser(browser),
	m_appServices(appServices),
	m_resourceInstance(resourceInstance)
{
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryImpl::Create(const PidlAbsolute &initialPidl,
	const FolderSettings &folderSettings, const FolderColumns *initialColumns)
{
	return std::make_unique<ShellBrowserImpl>(initialPidl, folderSettings, initialColumns,
		m_browser, m_appServices, m_resourceInstance);
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryImpl::CreateFromPreserved(
	const PreservedShellBrowser &preservedShellBrowser)
{
	return std::make_unique<ShellBrowserImpl>(preservedShellBrowser, m_browser, m_appServices,
		m_resourceInstance);
}
