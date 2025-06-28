// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserWindow.h"
#include "ShellBrowser/ShellBrowser.h"

BrowserWindow::BrowserWindow() : m_id(idCounter++), m_commandTargetManager(this)
{
}

int BrowserWindow::GetId() const
{
	return m_id;
}

BrowserWindow::LifecycleState BrowserWindow::GetLifecycleState() const
{
	return m_lifecycleState;
}

void BrowserWindow::SetLifecycleState(LifecycleState state)
{
	if (state == LifecycleState::Main)
	{
		CHECK(m_lifecycleState == LifecycleState::Starting);
	}
	else if (state == LifecycleState::Closing)
	{
		CHECK(m_lifecycleState == LifecycleState::Main);
	}
	else
	{
		CHECK(false);
	}

	m_lifecycleState = state;

	m_lifecycleStateChangedSignal(state);
}

boost::signals2::connection BrowserWindow::AddLifecycleStateChangedObserver(
	const LifecycleStateChangedSignal::slot_type &observer)
{
	return m_lifecycleStateChangedSignal.connect(observer);
}

BrowserCommandTargetManager *BrowserWindow::GetCommandTargetManager()
{
	return &m_commandTargetManager;
}

bool BrowserWindow::IsShellBrowserActive(const ShellBrowser *shellBrowser) const
{
	return shellBrowser == GetActiveShellBrowser();
}

void BrowserWindow::OpenDefaultItem()
{
	OpenDefaultItem(OpenFolderDisposition::CurrentTab);
}

void BrowserWindow::OpenItem(const std::wstring &itemPath)
{
	OpenItem(itemPath, OpenFolderDisposition::CurrentTab);
}

void BrowserWindow::OpenItem(PCIDLIST_ABSOLUTE pidlItem)
{
	OpenItem(pidlItem, OpenFolderDisposition::CurrentTab);
}
