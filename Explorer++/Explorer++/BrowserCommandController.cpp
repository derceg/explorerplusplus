// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserCommandController.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabContainerImpl.h"
#include "../Helper/ShellHelper.h"

BrowserCommandController::BrowserCommandController(BrowserWindow *browserWindow) :
	m_browserWindow(browserWindow)
{
}

BrowserCommandController::BrowserCommandController(ShellBrowser *shellBrowser) :
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

	case IDM_GO_QUICK_ACCESS:
		GoToPath(QUICK_ACCESS_PATH, disposition);
		break;

	case IDM_GO_COMPUTER:
		GoToKnownFolder(FOLDERID_ComputerFolder, disposition);
		break;

	case IDM_GO_DOCUMENTS:
		GoToKnownFolder(FOLDERID_Documents, disposition);
		break;

	case IDM_GO_DOWNLOADS:
		GoToKnownFolder(FOLDERID_Downloads, disposition);
		break;

	case IDM_GO_MUSIC:
		GoToKnownFolder(FOLDERID_Music, disposition);
		break;

	case IDM_GO_PICTURES:
		GoToKnownFolder(FOLDERID_Pictures, disposition);
		break;

	case IDM_GO_VIDEOS:
		GoToKnownFolder(FOLDERID_Videos, disposition);
		break;

	case IDM_GO_DESKTOP:
		GoToKnownFolder(FOLDERID_Desktop, disposition);
		break;

	case IDM_GO_RECYCLE_BIN:
		GoToKnownFolder(FOLDERID_RecycleBinFolder, disposition);
		break;

	case IDM_GO_CONTROL_PANEL:
		GoToKnownFolder(FOLDERID_ControlPanelFolder, disposition);
		break;

	case IDM_GO_PRINTERS:
		GoToKnownFolder(FOLDERID_PrintersFolder, disposition);
		break;

	case IDM_GO_NETWORK:
		GoToKnownFolder(FOLDERID_NetworkFolder, disposition);
		break;

	case IDM_GO_WSL_DISTRIBUTIONS:
		GoToPath(WSL_DISTRIBUTIONS_PATH, disposition);
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

		m_browserWindow->OpenItem(entry->GetPidl().Raw(), disposition);
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

		m_browserWindow->OpenItem(entry->GetPidl().Raw(), disposition);
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
			GetVirtualParentPath(currentEntry->GetPidl().Raw(), wil::out_param(pidlParent));

		if (FAILED(hr))
		{
			return;
		}

		m_browserWindow->OpenItem(pidlParent.get(), disposition);
	}
}

void BrowserCommandController::GoToPath(const std::wstring &path, OpenFolderDisposition disposition)
{
	m_browserWindow->OpenItem(path, disposition);
}

void BrowserCommandController::GoToKnownFolder(REFKNOWNFOLDERID knownFolderId,
	OpenFolderDisposition disposition)
{
	unique_pidl_absolute pidl;
	HRESULT hr =
		SHGetKnownFolderIDList(knownFolderId, KF_FLAG_DEFAULT, nullptr, wil::out_param(pidl));

	if (FAILED(hr))
	{
		return;
	}

	m_browserWindow->OpenItem(pidl.get(), disposition);
}

ShellBrowser *BrowserCommandController::GetSelectedShellBrowser() const
{
	if (m_testShellBrowser)
	{
		return m_testShellBrowser;
	}

	return m_browserWindow->GetActivePane()
		->GetTabContainerImpl()
		->GetSelectedTab()
		.GetShellBrowserImpl();
}
