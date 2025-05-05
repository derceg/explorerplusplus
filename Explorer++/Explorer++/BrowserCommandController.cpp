// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserCommandController.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "../Helper/ShellHelper.h"

BrowserCommandController::BrowserCommandController(BrowserWindow *browser, Config *config) :
	m_browser(browser),
	m_config(config)
{
}

bool BrowserCommandController::IsCommandEnabled(int command) const
{
	switch (command)
	{
	// These commands are context-sensitive (i.e. they depend on the active target).
	case IDM_FILE_DELETE:
	case IDM_FILE_DELETEPERMANENTLY:
		return m_browser->GetCommandTargetManager()->GetCurrentTarget()->IsCommandEnabled(command);

	default:
		DCHECK(false);
		return false;
	}
}

void BrowserCommandController::ExecuteCommand(int command, OpenFolderDisposition disposition)
{
	switch (command)
	{
	case IDM_FILE_DELETE:
	case IDM_FILE_DELETEPERMANENTLY:
		m_browser->GetCommandTargetManager()->GetCurrentTarget()->ExecuteCommand(command);
		break;

	case IDM_VIEW_TOOLBARS_ADDRESS_BAR:
		m_config->showAddressBar = !m_config->showAddressBar.get();
		break;

	case IDM_VIEW_TOOLBARS_MAIN_TOOLBAR:
		m_config->showMainToolbar = !m_config->showMainToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR:
		m_config->showBookmarksToolbar = !m_config->showBookmarksToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR:
		m_config->showDrivesToolbar = !m_config->showDrivesToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR:
		m_config->showApplicationToolbar = !m_config->showApplicationToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_LOCK_TOOLBARS:
		m_config->lockToolbars = !m_config->lockToolbars.get();
		break;

	case IDM_VIEW_TOOLBARS_CUSTOMIZE:
		m_browser->StartMainToolbarCustomization();
		break;

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
	auto *shellBrowser = GetActiveShellBrowser();

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

		m_browser->OpenItem(entry->GetPidl().Raw(), disposition);
	}
}

void BrowserCommandController::GoForward(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetActiveShellBrowser();

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

		m_browser->OpenItem(entry->GetPidl().Raw(), disposition);
	}
}

void BrowserCommandController::GoUp(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetActiveShellBrowser();

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

		m_browser->OpenItem(pidlParent.get(), disposition);
	}
}

void BrowserCommandController::GoToPath(const std::wstring &path, OpenFolderDisposition disposition)
{
	m_browser->OpenItem(path, disposition);
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

	m_browser->OpenItem(pidl.get(), disposition);
}

ShellBrowser *BrowserCommandController::GetActiveShellBrowser() const
{
	return m_browser->GetActiveShellBrowser();
}
