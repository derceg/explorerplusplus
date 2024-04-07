// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>

MainWindow *MainWindow::Create(HWND hwnd, std::shared_ptr<Config> config,
	HINSTANCE resourceInstance, CoreInterface *coreInterface)
{
	return new MainWindow(hwnd, config, resourceInstance, coreInterface);
}

MainWindow::MainWindow(HWND hwnd, std::shared_ptr<Config> config, HINSTANCE resourceInstance,
	CoreInterface *coreInterface) :
	m_hwnd(hwnd),
	m_config(config),
	m_resourceInstance(resourceInstance),
	m_coreInterface(coreInterface)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&MainWindow::WndProc, this)));

	m_coreInterface->AddTabsInitializedObserver(
		[this]
		{
			m_connections.push_back(
				m_coreInterface->GetTabContainer()->tabSelectedSignal.AddObserver(
					std::bind_front(&MainWindow::OnTabSelected, this)));
			m_connections.push_back(
				m_coreInterface->GetTabContainer()->tabNavigationCommittedSignal.AddObserver(
					std::bind_front(&MainWindow::OnNavigationCommitted, this)));
		});

	m_connections.push_back(m_config->showFullTitlePath.addObserver(
		std::bind_front(&MainWindow::OnShowFullTitlePathUpdated, this)));
	m_connections.push_back(m_config->showUserNameInTitleBar.addObserver(
		std::bind_front(&MainWindow::OnShowUserNameInTitleBarUpdated, this)));
	m_connections.push_back(m_config->showPrivilegeLevelInTitleBar.addObserver(
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

void MainWindow::OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

	if (m_coreInterface->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateWindowText();
	}
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
	auto pidlDirectory = tab.GetShellBrowser()->GetDirectoryIdl();

	std::wstring folderDisplayName;

	/* Don't show full paths for virtual folders (as only the folders
	GUID will be shown). */
	if (m_config->showFullTitlePath.get() && !tab.GetShellBrowser()->InVirtualFolder())
	{
		GetDisplayName(pidlDirectory.get(), SHGDN_FORPARSING, folderDisplayName);
	}
	else
	{
		GetDisplayName(pidlDirectory.get(), SHGDN_NORMAL, folderDisplayName);
	}

	TCHAR szTitle[512];

	TCHAR szTemp[64];
	LoadString(m_resourceInstance, IDS_MAIN_WINDOW_TITLE, szTemp, SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szTitle, SIZEOF_ARRAY(szTitle), szTemp, folderDisplayName.c_str(),
		NExplorerplusplus::APP_NAME);

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), _T(" ["));
	}

	if (m_config->showUserNameInTitleBar.get())
	{
		TCHAR szOwner[512];
		GetProcessOwner(GetCurrentProcessId(), szOwner, SIZEOF_ARRAY(szOwner));

		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), szOwner);
	}

	if (m_config->showPrivilegeLevelInTitleBar.get())
	{
		TCHAR szPrivilegeAddition[64];
		TCHAR szPrivilege[64];

		if (CheckGroupMembership(GroupType::Administrators))
		{
			LoadString(m_resourceInstance, IDS_PRIVILEGE_LEVEL_ADMINISTRATORS, szPrivilege,
				SIZEOF_ARRAY(szPrivilege));
		}
		else if (CheckGroupMembership(GroupType::PowerUsers))
		{
			LoadString(m_resourceInstance, IDS_PRIVILEGE_LEVEL_POWER_USERS, szPrivilege,
				SIZEOF_ARRAY(szPrivilege));
		}
		else if (CheckGroupMembership(GroupType::Users))
		{
			LoadString(m_resourceInstance, IDS_PRIVILEGE_LEVEL_USERS, szPrivilege,
				SIZEOF_ARRAY(szPrivilege));
		}
		else if (CheckGroupMembership(GroupType::UsersRestricted))
		{
			LoadString(m_resourceInstance, IDS_PRIVILEGE_LEVEL_USERS_RESTRICTED, szPrivilege,
				SIZEOF_ARRAY(szPrivilege));
		}

		if (m_config->showUserNameInTitleBar.get())
		{
			StringCchPrintf(szPrivilegeAddition, SIZEOF_ARRAY(szPrivilegeAddition), _T(" - %s"),
				szPrivilege);
		}
		else
		{
			StringCchPrintf(szPrivilegeAddition, SIZEOF_ARRAY(szPrivilegeAddition), _T("%s"),
				szPrivilege);
		}

		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), szPrivilegeAddition);
	}

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), _T("]"));
	}

	SetWindowText(m_hwnd, szTitle);
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
