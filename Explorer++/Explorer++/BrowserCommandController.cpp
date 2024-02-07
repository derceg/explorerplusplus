// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserCommandController.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabContainer.h"

BrowserCommandController::BrowserCommandController(BrowserWindow *browserWindow) :
	m_browserWindow(browserWindow)
{
}

BrowserCommandController::BrowserCommandController(ShellBrowserInterface *shellBrowser) :
	m_testShellBrowser(shellBrowser)
{
}

void BrowserCommandController::ExecuteCommand(int command, OpenFolderDisposition disposition)
{
	switch (command)
	{
	case IDM_GO_BACK:
		GoBack(disposition);
		break;

	case IDM_GO_FORWARD:
		GoForward(disposition);
		break;

	case IDM_GO_UP:
		GoUp(disposition);
		break;

	default:
		DCHECK(false);
		break;
	}
}

void BrowserCommandController::GoBack(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetSelectedShellBrowser();

	if (disposition == OpenFolderDisposition::CurrentTab)
	{
		shellBrowser->GetNavigationController()->GoBack();
	}
	else
	{
		auto *entry = shellBrowser->GetNavigationController()->GetEntry(-1);

		if (!entry)
		{
			return;
		}

		m_browserWindow->OpenItem(entry->GetPidl().get(), disposition);
	}
}

void BrowserCommandController::GoForward(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetSelectedShellBrowser();

	if (disposition == OpenFolderDisposition::CurrentTab)
	{
		shellBrowser->GetNavigationController()->GoForward();
	}
	else
	{
		auto *entry = shellBrowser->GetNavigationController()->GetEntry(1);

		if (!entry)
		{
			return;
		}

		m_browserWindow->OpenItem(entry->GetPidl().get(), disposition);
	}
}

void BrowserCommandController::GoUp(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetSelectedShellBrowser();

	if (disposition == OpenFolderDisposition::CurrentTab)
	{
		shellBrowser->GetNavigationController()->GoUp();
	}
	else
	{
		auto *currentEntry = shellBrowser->GetNavigationController()->GetCurrentEntry();

		unique_pidl_absolute pidlParent;
		HRESULT hr =
			GetVirtualParentPath(currentEntry->GetPidl().get(), wil::out_param(pidlParent));

		if (FAILED(hr))
		{
			return;
		}

		m_browserWindow->OpenItem(pidlParent.get(), disposition);
	}
}

ShellBrowserInterface *BrowserCommandController::GetSelectedShellBrowser() const
{
	if (m_testShellBrowser)
	{
		return m_testShellBrowser;
	}

	return m_browserWindow->GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}
