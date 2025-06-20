// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainWindow.h"
#include "App.h"
#include "Config.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/WindowSubclass.h"
#include <wil/resource.h>

MainWindow *MainWindow::Create(HWND hwnd, App *app, BrowserWindow *browser,
	CoreInterface *coreInterface)
{
	return new MainWindow(hwnd, app, browser, coreInterface);
}

MainWindow::MainWindow(HWND hwnd, App *app, BrowserWindow *browser, CoreInterface *coreInterface) :
	m_hwnd(hwnd),
	m_app(app),
	m_coreInterface(coreInterface)
{
	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&MainWindow::WndProc, this)));

	m_connections.push_back(m_app->GetTabEvents()->AddSelectedObserver(
		std::bind_front(&MainWindow::OnTabSelected, this), TabEventScope::ForBrowser(*browser)));

	m_connections.push_back(m_app->GetShellBrowserEvents()->AddDirectoryPropertiesChangedObserver(
		std::bind_front(&MainWindow::OnDirectoryPropertiesChanged, this),
		NavigationEventScope::ForActiveShellBrowser(*browser)));

	m_connections.push_back(m_app->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&MainWindow::OnNavigationCommitted, this),
		NavigationEventScope::ForActiveShellBrowser(*browser)));

	m_connections.push_back(m_app->GetConfig()->showFullTitlePath.addObserver(
		std::bind_front(&MainWindow::OnShowFullTitlePathUpdated, this)));
	m_connections.push_back(m_app->GetConfig()->showUserNameInTitleBar.addObserver(
		std::bind_front(&MainWindow::OnShowUserNameInTitleBarUpdated, this)));
	m_connections.push_back(m_app->GetConfig()->showPrivilegeLevelInTitleBar.addObserver(
		std::bind_front(&MainWindow::OnShowPrivilegeLevelInTitleBarUpdated, this)));

	// The main window is registered as a drop target only so that the drag image will be
	// consistently shown when an item is being dragged. For the drag image to be shown, the
	// relevant IDropTargetHelper methods need to be called during the drag. To do that, the window
	// under the mouse needs to be registered as a drop target.
	// Rather than having to register every window, the top-level window can simply be registered
	// instead. That way, it will act as a fallback if there isn't a more specific child window
	// registered.
	m_dropTargetWindow =
		winrt::make_self<DropTargetWindow>(m_hwnd, static_cast<DropTargetInternal *>(this));
}

LRESULT MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCDESTROY:
		OnNcDestroy();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void MainWindow::OnNavigationCommitted(const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(request);

	UpdateWindowText();
}

void MainWindow::OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	UNREFERENCED_PARAMETER(shellBrowser);

	UpdateWindowText();
}

void MainWindow::OnTabSelected(const Tab &tab)
{
	UNREFERENCED_PARAMETER(tab);

	UpdateWindowText();
}

void MainWindow::OnShowFullTitlePathUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void MainWindow::OnShowUserNameInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void MainWindow::OnShowPrivilegeLevelInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void MainWindow::UpdateWindowText()
{
	const Tab &tab = m_coreInterface->GetTabContainer()->GetSelectedTab();
	auto pidlDirectory = tab.GetShellBrowserImpl()->GetDirectoryIdl();

	std::wstring folderDisplayName;

	/* Don't show full paths for virtual folders (as only the folders
	GUID will be shown). */
	if (m_app->GetConfig()->showFullTitlePath.get()
		&& !tab.GetShellBrowserImpl()->InVirtualFolder())
	{
		GetDisplayName(pidlDirectory.get(), SHGDN_FORPARSING, folderDisplayName);
	}
	else
	{
		GetDisplayName(pidlDirectory.get(), SHGDN_NORMAL, folderDisplayName);
	}

	std::wstring title = std::format(L"{} - {}", folderDisplayName, App::APP_NAME);

	if (m_app->GetConfig()->showUserNameInTitleBar.get()
		|| m_app->GetConfig()->showPrivilegeLevelInTitleBar.get())
	{
		title += L" [";
	}

	if (m_app->GetConfig()->showUserNameInTitleBar.get())
	{
		TCHAR owner[512];
		GetProcessOwner(GetCurrentProcessId(), owner, std::size(owner));

		title += owner;
	}

	if (m_app->GetConfig()->showPrivilegeLevelInTitleBar.get())
	{
		std::wstring privilegeLevel;

		if (CheckGroupMembership(GroupType::Administrators))
		{
			privilegeLevel =
				m_app->GetResourceLoader()->LoadString(IDS_PRIVILEGE_LEVEL_ADMINISTRATORS);
		}
		else if (CheckGroupMembership(GroupType::PowerUsers))
		{
			privilegeLevel =
				m_app->GetResourceLoader()->LoadString(IDS_PRIVILEGE_LEVEL_POWER_USERS);
		}
		else if (CheckGroupMembership(GroupType::Users))
		{
			privilegeLevel = m_app->GetResourceLoader()->LoadString(IDS_PRIVILEGE_LEVEL_USERS);
		}
		else if (CheckGroupMembership(GroupType::UsersRestricted))
		{
			privilegeLevel =
				m_app->GetResourceLoader()->LoadString(IDS_PRIVILEGE_LEVEL_USERS_RESTRICTED);
		}

		if (m_app->GetConfig()->showUserNameInTitleBar.get())
		{
			title += L" - ";
		}

		title += privilegeLevel;
	}

	if (m_app->GetConfig()->showUserNameInTitleBar.get()
		|| m_app->GetConfig()->showPrivilegeLevelInTitleBar.get())
	{
		title += L"]";
	}

	SetWindowText(m_hwnd, title.c_str());
}

// DropTargetInternal
// Note that, as described above, this window is registered as a drop target only so that drag
// images are shown consistently. Dropping items isn't supported at all.
DWORD MainWindow::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

DWORD MainWindow::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

void MainWindow::DragLeave()
{
}

DWORD MainWindow::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

void MainWindow::OnNcDestroy()
{
	delete this;
}
