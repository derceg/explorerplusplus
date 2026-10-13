// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserView.h"
#include "AppInfo.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "TabEvents.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/WindowSubclass.h"

BrowserView *BrowserView::Create(HWND hwnd, BrowserWindow *browser, const Config *config,
	TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
	NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader)
{
	return new BrowserView(hwnd, browser, config, tabEvents, shellBrowserEvents, navigationEvents,
		resourceLoader);
}

BrowserView::BrowserView(HWND hwnd, BrowserWindow *browser, const Config *config,
	TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
	NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader) :
	m_hwnd(hwnd),
	m_browser(browser),
	m_config(config),
	m_resourceLoader(resourceLoader)
{
	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&BrowserView::WndProc, this)));

	m_connections.push_back(m_browser->AddLifecycleStateChangedObserver(
		std::bind_front(&BrowserView::OnBrowserLifecycleStateChanged, this)));

	m_connections.push_back(tabEvents->AddSelectedObserver(
		std::bind_front(&BrowserView::OnTabSelected, this), TabEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(shellBrowserEvents->AddDirectoryPropertiesChangedObserver(
		std::bind_front(&BrowserView::OnDirectoryPropertiesChanged, this),
		NavigationEventScope::ForActiveShellBrowser(*m_browser)));

	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&BrowserView::OnNavigationCommitted, this),
		NavigationEventScope::ForActiveShellBrowser(*m_browser)));

	m_connections.push_back(m_config->showFullTitlePath.addObserver(
		std::bind_front(&BrowserView::OnShowFullTitlePathUpdated, this)));
	m_connections.push_back(m_config->showUserNameInTitleBar.addObserver(
		std::bind_front(&BrowserView::OnShowUserNameInTitleBarUpdated, this)));
	m_connections.push_back(m_config->showPrivilegeLevelInTitleBar.addObserver(
		std::bind_front(&BrowserView::OnShowPrivilegeLevelInTitleBarUpdated, this)));

	// The window is registered as a drop target only so that the drag image will be consistently
	// shown when an item is being dragged. For the drag image to be shown, the relevant
	// IDropTargetHelper methods need to be called during the drag. To do that, the window under the
	// mouse needs to be registered as a drop target.
	//
	// Rather than having to register every window, the top-level window can simply be registered
	// instead. That way, it will act as a fallback if there isn't a more specific child window
	// registered.
	m_dropTargetWindow =
		winrt::make_self<DropTargetWindow>(m_hwnd, static_cast<DropTargetInternal *>(this));
}

LRESULT BrowserView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCDESTROY:
		OnNcDestroy();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void BrowserView::OnBrowserLifecycleStateChanged(BrowserWindow::LifecycleState updatedState)
{
	if (updatedState == BrowserWindow::LifecycleState::Main)
	{
		UpdateWindowText();
	}
}

void BrowserView::OnNavigationCommitted(const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(request);

	UpdateWindowText();
}

void BrowserView::OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	UNREFERENCED_PARAMETER(shellBrowser);

	UpdateWindowText();
}

void BrowserView::OnTabSelected(const Tab &tab)
{
	UNREFERENCED_PARAMETER(tab);

	UpdateWindowText();
}

void BrowserView::OnShowFullTitlePathUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void BrowserView::OnShowUserNameInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void BrowserView::OnShowPrivilegeLevelInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void BrowserView::UpdateWindowText()
{
	if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
	{
		return;
	}

	const Tab &tab = m_browser->GetActiveTabContainer()->GetSelectedTab();
	const auto &pidl = tab.GetShellBrowser()->GetDirectory();

	std::wstring folderDisplayName;

	/* Don't show full paths for virtual folders (as only the folders
	GUID will be shown). */
	if (m_config->showFullTitlePath.get() && !tab.GetShellBrowserImpl()->InVirtualFolder())
	{
		GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, folderDisplayName);
	}
	else
	{
		GetDisplayName(pidl.Raw(), SHGDN_NORMAL, folderDisplayName);
	}

	std::wstring title = std::format(L"{} - {}", folderDisplayName, AppInfo::NAME);

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		title += L" [";
	}

	if (m_config->showUserNameInTitleBar.get())
	{
		TCHAR owner[512];
		GetProcessOwner(GetCurrentProcessId(), owner, std::size(owner));

		title += owner;
	}

	if (m_config->showPrivilegeLevelInTitleBar.get())
	{
		std::wstring privilegeLevel;

		if (CheckGroupMembership(GroupType::Administrators))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_ADMINISTRATORS);
		}
		else if (CheckGroupMembership(GroupType::PowerUsers))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_POWER_USERS);
		}
		else if (CheckGroupMembership(GroupType::Users))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_USERS);
		}
		else if (CheckGroupMembership(GroupType::UsersRestricted))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_USERS_RESTRICTED);
		}

		if (m_config->showUserNameInTitleBar.get())
		{
			title += L" - ";
		}

		title += privilegeLevel;
	}

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		title += L"]";
	}

	SetWindowText(m_hwnd, title.c_str());
}

void BrowserView::RedrawMenuBar()
{
	auto res = DrawMenuBar(m_hwnd);
	CHECK(res);
}

// This is used for both Tab/Shift+Tab and F6/Shift+F6. While IsDialogMessage() could be used to
// handle Tab/Shift+Tab, the key combinations in this case are synonyms for each other. Since
// F6/Shift+F6 would need to be handled manually anyway, handling both with the same function
// ensures that they have identical behavior.
void BrowserView::FocusNextWindow(FocusChangeDirection direction)
{
	HWND focus = GetFocus();
	HWND initialControl;

	// The focus should always be on one of the child windows, but this function should still set
	// the focus even if there is no current focus or the focus is on the parent.
	//
	// GetNextDlgTabItem() may fail if the initial control is NULL or the parent window, so that
	// situation is prevented here.
	if (focus && IsChild(m_hwnd, focus))
	{
		initialControl = focus;
	}
	else
	{
		initialControl = GetWindow(m_hwnd, GW_CHILD);
	}

	HWND nextWindow =
		GetNextDlgTabItem(m_hwnd, initialControl, direction == FocusChangeDirection::Previous);
	DCHECK(nextWindow);

	if (nextWindow)
	{
		SetFocus(nextWindow);
	}
}

// DropTargetInternal
// Note that, as described above, this window is registered as a drop target only so that drag
// images are shown consistently. Dropping items isn't supported at all.
DWORD BrowserView::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

DWORD BrowserView::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

void BrowserView::DragLeave()
{
}

DWORD BrowserView::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

void BrowserView::OnNcDestroy()
{
	delete this;
}
