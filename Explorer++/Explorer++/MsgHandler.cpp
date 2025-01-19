// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "App.h"
#include "BrowserTracker.h"
#include "ColorRule.h"
#include "Config.h"
#include "DarkModeManager.h"
#include "DisplayWindow/DisplayWindow.h"
#include "HolderWindow.h"
#include "MainRebarStorage.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "Plugins/PluginManager.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellTreeView/ShellTreeView.h"
#include "Storage.h"
#include "SystemFontHelper.h"
#include "TabContainer.h"
#include "TabStorage.h"
#include "TaskbarThumbnails.h"
#include "ToolbarHelper.h"
#include "WindowStorage.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/iDirectoryMonitor.h"
#include <boost/range/adaptor/map.hpp>
#include <glog/logging.h>
#include <wil/resource.h>
#include <algorithm>

void Explorerplusplus::OpenDefaultItem(OpenFolderDisposition openFolderDisposition)
{
	OpenItem(m_config->defaultTabDirectory, openFolderDisposition);
}

void Explorerplusplus::OpenItem(const std::wstring &itemPath,
	OpenFolderDisposition openFolderDisposition)
{
	unique_pidl_absolute pidlItem;
	HRESULT hr = ParseDisplayNameForNavigation(itemPath, pidlItem);

	if (SUCCEEDED(hr))
	{
		OpenItem(pidlItem.get(), openFolderDisposition);
	}
}

void Explorerplusplus::OpenItem(PCIDLIST_ABSOLUTE pidlItem,
	OpenFolderDisposition openFolderDisposition)
{
	SFGAOF attributes = SFGAO_FOLDER | SFGAO_STREAM | SFGAO_LINK;
	HRESULT hr = GetItemAttributes(pidlItem, &attributes);

	if (FAILED(hr))
	{
		return;
	}

	if (WI_AreAllFlagsSet(attributes, SFGAO_FOLDER | SFGAO_STREAM))
	{
		// This is container file. Examples of these files include:
		//
		// - .7z
		// - .cab
		// - .search-ms
		// - .zip

		if (ShouldOpenContainerFile(pidlItem))
		{
			OpenFolderItem(pidlItem, openFolderDisposition);
		}
		else
		{
			OpenFileItem(pidlItem, L"");
		}
	}
	else if (WI_IsFlagSet(attributes, SFGAO_FOLDER))
	{
		OpenFolderItem(pidlItem, openFolderDisposition);
	}
	else if (WI_IsFlagSet(attributes, SFGAO_LINK))
	{
		OpenShortcutItem(pidlItem, openFolderDisposition);
	}
	else
	{
		OpenFileItem(pidlItem, L"");
	}
}

void Explorerplusplus::OpenShortcutItem(PCIDLIST_ABSOLUTE pidlItem,
	OpenFolderDisposition openFolderDisposition)
{
	unique_pidl_absolute target;
	HRESULT hr = MaybeResolveLinkTarget(m_hContainer, pidlItem, target);

	if (FAILED(hr))
	{
		// If the target doesn't exist, MaybeResolveLinkTarget() will show an error message to the
		// user. So, that case doesn't need to be handled at all here.
		return;
	}

	bool openAsFolder = false;

	SFGAOF targetAttributes = SFGAO_FOLDER | SFGAO_STREAM;
	hr = GetItemAttributes(target.get(), &targetAttributes);

	if (SUCCEEDED(hr))
	{
		bool isFolder = WI_IsFlagSet(targetAttributes, SFGAO_FOLDER)
			&& WI_IsFlagClear(targetAttributes, SFGAO_STREAM);
		bool isContainerFile = WI_IsFlagSet(targetAttributes, SFGAO_FOLDER)
			&& WI_IsFlagSet(targetAttributes, SFGAO_STREAM);

		openAsFolder = isFolder || (isContainerFile && ShouldOpenContainerFile(target.get()));
	}

	if (openAsFolder)
	{
		// This is a shortcut to a folder item or container file. In either case, it should be
		// opened here, rather than being opened via the shell (since opening the shortcut via the
		// shell will result in the item being opened in the default file manager).
		OpenFolderItem(target.get(), openFolderDisposition);
	}
	else
	{
		// If the shortcut file points to something other than a folder/container file, the shortcut
		// should be opened via the shell. It's important to do that, rather than executing the
		// target directly, since the shortcut can have various start options defined (e.g.
		// parameters, initial directory, window state). Those options won't be applied if the
		// target is simply executed.
		// This branch wil also be taken if the shortcut points to a .zip file and .zip file
		// handling is turned off. In that situation, the shortcut should still be opened via the
		// shell. That's because at least one of the shortcut options (window state) will be applied
		// when opening the shortcut. That won't be the case if the target is executed directly.
		OpenFileItem(pidlItem, L"");
	}
}

// Returns true if the specified container file should be opened as a folder. If false, the file
// should be opened via the shell.
bool Explorerplusplus::ShouldOpenContainerFile(PCIDLIST_ABSOLUTE pidlItem)
{
	std::wstring parsingPath;
	HRESULT hr = GetDisplayName(pidlItem, SHGDN_FORPARSING, parsingPath);

	if (FAILED(hr))
	{
		return false;
	}

	bool isZipFile = (SUCCEEDED(hr) && parsingPath.ends_with(L".zip"));
	return (isZipFile && m_config->handleZipFiles) || !isZipFile;
}

void Explorerplusplus::OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem,
	OpenFolderDisposition openFolderDisposition)
{
	if (openFolderDisposition == OpenFolderDisposition::CurrentTab)
	{
		if (m_config->alwaysOpenNewTab)
		{
			openFolderDisposition = OpenFolderDisposition::ForegroundTab;
		}
	}
	else if (openFolderDisposition == OpenFolderDisposition::NewTabDefault)
	{
		openFolderDisposition = m_config->openTabsInForeground
			? OpenFolderDisposition::ForegroundTab
			: OpenFolderDisposition::BackgroundTab;
	}
	else if (openFolderDisposition == OpenFolderDisposition::NewTabAlternate)
	{
		openFolderDisposition = m_config->openTabsInForeground
			? OpenFolderDisposition::BackgroundTab
			: OpenFolderDisposition::ForegroundTab;
	}

	switch (openFolderDisposition)
	{
	case OpenFolderDisposition::CurrentTab:
	{
		Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
		auto navigateParams = NavigateParams::Normal(pidlItem);
		tab.GetShellBrowserImpl()->GetNavigationController()->Navigate(navigateParams);
	}
	break;

	case OpenFolderDisposition::BackgroundTab:
	{
		auto navigateParams = NavigateParams::Normal(pidlItem);
		GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams);
	}
	break;

	case OpenFolderDisposition::ForegroundTab:
	{
		auto navigateParams = NavigateParams::Normal(pidlItem);
		GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams,
			TabSettings(_selected = true));
	}
	break;

	case OpenFolderDisposition::NewWindow:
		OpenDirectoryInNewWindow(pidlItem);
		break;

	default:
		DCHECK(false) << "Unhandled disposition";
		break;
	}
}

void Explorerplusplus::OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory)
{
	/* Create a new instance of this program, with the
	specified path as an argument. */
	std::wstring path;
	GetDisplayName(pidlDirectory, SHGDN_FORPARSING, path);

	TCHAR szParameters[512];
	StringCchPrintf(szParameters, std::size(szParameters), _T("\"%s\""), path.c_str());

	LaunchCurrentProcess(m_hContainer, szParameters);
}

void Explorerplusplus::OpenFileItem(const std::wstring &itemPath, const std::wstring &parameters)
{
	auto shellBrowser = GetActiveShellBrowserImpl();
	ExecuteFileAction(m_hContainer, itemPath, L"", parameters,
		shellBrowser->InVirtualFolder() ? L"" : shellBrowser->GetDirectory().c_str());
}

void Explorerplusplus::OpenFileItem(PCIDLIST_ABSOLUTE pidl, const std::wstring &parameters)
{
	auto shellBrowser = GetActiveShellBrowserImpl();
	ExecuteFileAction(m_hContainer, pidl, L"", parameters,
		shellBrowser->InVirtualFolder() ? L"" : shellBrowser->GetDirectory().c_str());
}

void Explorerplusplus::OnSize(UINT state)
{
	if (state == SIZE_MINIMIZED)
	{
		// There's no need to update the layout when the window is being minimized.
		return;
	}

	UpdateLayout();
}

concurrencpp::null_result Explorerplusplus::ScheduleUpdateLayout(WeakPtr<Explorerplusplus> self,
	Runtime *runtime)
{
	// This function is designed to be called from the UI thread and the call here will also resume
	// on the UI thread. Rather than immediately resuming, however, this call will result in a
	// message being posted. Therefore, this function will only resume once the message has been
	// processed.
	co_await concurrencpp::resume_on(runtime->GetUiThreadExecutor());

	if (!self)
	{
		co_return;
	}

	self->UpdateLayout();
}

void Explorerplusplus::UpdateLayout()
{
	if (!m_browserInitialized || m_browserClosing)
	{
		return;
	}

#if DCHECK_IS_ON()
	// When updating the size of a control below (e.g. the main rebar control), it's possible that
	// another layout may be requested. That layout, however, shouldn't occur in the middle of an
	// existing layout operation, but should instead be scheduled to run at a future point.
	DCHECK(!m_performingLayout);
	m_performingLayout = true;
	auto resetPerformingLayout = wil::scope_exit([this]() { m_performingLayout = false; });
#endif

	RECT mainWindowRect;
	GetClientRect(m_hContainer, &mainWindowRect);

	int mainWindowWidth = GetRectWidth(&mainWindowRect);
	int mainWindowHeight = GetRectHeight(&mainWindowRect);

	int indentBottom = 0;
	int indentTop = 0;
	int indentRight = 0;
	int indentLeft = 0;

	auto &dpiCompatibility = DpiCompatibility::GetInstance();

	m_treeViewWidth = std::clamp(m_treeViewWidth,
		dpiCompatibility.ScaleValue(m_treeViewHolder->GetHWND(), TREEVIEW_MINIMUM_WIDTH),
		static_cast<int>(TREEVIEW_MAXIMUM_WIDTH_PERCENTAGE * mainWindowWidth));
	m_displayWindowWidth = std::max(m_displayWindowWidth,
		dpiCompatibility.ScaleValue(m_displayWindow->GetHWND(), DISPLAY_WINDOW_MINIMUM_WIDTH));
	m_displayWindowHeight = std::max(m_displayWindowHeight,
		dpiCompatibility.ScaleValue(m_displayWindow->GetHWND(), DISPLAY_WINDOW_MINIMUM_HEIGHT));

	auto rebarHeight = m_mainRebarView->GetHeight();
	SetWindowPos(m_mainRebarView->GetHWND(), nullptr, 0, 0, mainWindowWidth, rebarHeight,
		SWP_NOZORDER | SWP_NOMOVE);

	int indentRebar = rebarHeight;

	if (m_config->showStatusBar)
	{
		RECT statusBarRect;
		GetWindowRect(m_hStatusBar, &statusBarRect);
		indentBottom += GetRectHeight(&statusBarRect);
	}

	if (m_config->showDisplayWindow.get())
	{
		if (m_config->displayWindowVertical)
		{
			indentRight += m_displayWindowWidth;
		}
		else
		{
			indentBottom += m_displayWindowHeight;
		}
	}

	if (m_config->showFolders.get())
	{
		indentLeft = m_treeViewWidth;
	}

	// Since the display area is indicated to start at (0, 0), displayRect.top will contain the
	// height of the tab control above the display area.
	RECT displayRect = { 0, 0, 0, 0 };
	TabCtrl_AdjustRect(GetActivePane()->GetTabContainer()->GetHWND(), true, &displayRect);
	int tabWindowHeight = std::abs(displayRect.top);

	indentTop = indentRebar;

	if (m_bShowTabBar)
	{
		if (!m_config->showTabBarAtBottom.get())
		{
			indentTop += tabWindowHeight;
		}
	}

	/* <---- Tab control + backing ----> */

	int tabBackingLeft;
	int tabBackingWidth;

	if (m_config->extendTabControl.get())
	{
		tabBackingLeft = 0;
		tabBackingWidth = mainWindowWidth;
	}
	else
	{
		tabBackingLeft = indentLeft;
		tabBackingWidth = mainWindowWidth - indentLeft - indentRight;
	}

	UINT showFlags = (m_bShowTabBar ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOZORDER;

	int tabTop;

	if (!m_config->showTabBarAtBottom.get())
	{
		tabTop = indentRebar;
	}
	else
	{
		tabTop = mainWindowHeight - indentBottom - tabWindowHeight;
	}

	/* If we're showing the tab bar at the bottom of the listview,
	the only thing that will change is the top coordinate. */
	SetWindowPos(m_hTabBacking, nullptr, tabBackingLeft, tabTop, tabBackingWidth, tabWindowHeight,
		showFlags);

	SetWindowPos(GetActivePane()->GetTabContainer()->GetHWND(), nullptr, 0, 0, tabBackingWidth - 25,
		tabWindowHeight, SWP_SHOWWINDOW | SWP_NOZORDER);

	/* Tab close button. */
	int scaledCloseToolbarXOffset =
		dpiCompatibility.ScaleValue(m_hTabWindowToolbar, ToolbarHelper::CLOSE_TOOLBAR_X_OFFSET);

	RECT tabToolbarRect;
	GetClientRect(m_hTabWindowToolbar, &tabToolbarRect);
	SetWindowPos(m_hTabWindowToolbar, nullptr,
		tabBackingWidth - GetRectWidth(&tabToolbarRect) - scaledCloseToolbarXOffset,
		(tabWindowHeight - GetRectHeight(&tabToolbarRect)) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	int holderTop;

	if (m_config->extendTabControl.get() && !m_config->showTabBarAtBottom.get())
	{
		holderTop = indentTop;
	}
	else
	{
		holderTop = indentRebar;
	}

	/* <---- Holder window + child windows ----> */

	int holderHeight;

	if (m_config->extendTabControl.get() && m_config->showTabBarAtBottom.get() && m_bShowTabBar)
	{
		holderHeight = mainWindowHeight - indentBottom - holderTop - tabWindowHeight;
	}
	else
	{
		holderHeight = mainWindowHeight - indentBottom - holderTop;
	}

	SetWindowPos(m_treeViewHolder->GetHWND(), nullptr, 0, holderTop, m_treeViewWidth, holderHeight,
		SWP_NOZORDER);

	/* <---- Display window ----> */

	UINT displayWindowShowFlags =
		(m_config->showDisplayWindow.get() ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOZORDER;

	if (m_config->displayWindowVertical)
	{
		SetWindowPos(m_displayWindow->GetHWND(), nullptr, mainWindowWidth - indentRight,
			indentRebar, m_displayWindowWidth, mainWindowHeight - indentRebar - indentBottom,
			displayWindowShowFlags);
	}
	else
	{
		SetWindowPos(m_displayWindow->GetHWND(), nullptr, 0, mainWindowHeight - indentBottom,
			mainWindowWidth, m_displayWindowHeight, displayWindowShowFlags);
	}

	/* <---- ALL listview windows ----> */

	for (auto &tab : GetActivePane()->GetTabContainer()->GetAllTabs() | boost::adaptors::map_values)
	{
		showFlags = SWP_NOZORDER;

		if (GetActivePane()->GetTabContainer()->IsTabSelected(*tab))
		{
			showFlags |= SWP_SHOWWINDOW;
		}

		int width = mainWindowWidth - indentLeft - indentRight;
		int height = mainWindowHeight - indentBottom - indentTop;

		if (m_config->showTabBarAtBottom.get() && m_bShowTabBar)
		{
			height -= tabWindowHeight;
		}

		SetWindowPos(tab->GetShellBrowserImpl()->GetListView(), NULL, indentLeft, indentTop, width,
			height, showFlags);
	}

	/* <---- Status bar ----> */

	PinStatusBar(m_hStatusBar, mainWindowWidth, mainWindowHeight);
}

void Explorerplusplus::OnDpiChanged(const RECT *updatedWindowRect)
{
	SetWindowPos(m_hContainer, nullptr, updatedWindowRect->left, updatedWindowRect->top,
		GetRectWidth(updatedWindowRect), GetRectHeight(updatedWindowRect),
		SWP_NOZORDER | SWP_NOACTIVATE);
}

std::optional<LRESULT> Explorerplusplus::OnCtlColorStatic(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hdc);

	if (hwnd == m_hTabBacking)
	{
		if (!m_app->GetDarkModeManager()->IsDarkModeEnabled())
		{
			return std::nullopt;
		}

		return reinterpret_cast<INT_PTR>(m_tabBarBackgroundBrush.get());
	}

	return std::nullopt;
}

int Explorerplusplus::OnDestroy()
{
	DCHECK(!m_browserClosing);
	m_browserClosing = true;

	// Broadcasting focus changed events when the browser is being closed is both unnecessary and
	// unsafe. It's unsafe, because guarantees that are normally upheld during the lifetime of the
	// browser window won't necessarily be upheld while the browser window is closing. For example,
	// normally there should always be at least one tab. When the browser window is closing, the tab
	// container will be destroyed, so the assumption that there is at least a single tab won't
	// necessarily hold.
	// Therefore, all slots are disconnected here.
	m_focusChangedSignal.disconnect_all_slots();

	if (m_SHChangeNotifyID != 0)
	{
		SHChangeNotifyDeregister(m_SHChangeNotifyID);
	}

	// It's important that the plugins are destroyed before the main window is destroyed and before
	// this class is destroyed.
	// The first reason is that the API binding classes may interact with the UI on destruction
	// (e.g. to remove menu entries they've added).
	// The second reason is that the API bindings assume they can use the objects passed to them
	// until their destruction. Those objects are destroyed automatically when this class is
	// destroyed, so letting the plugins be destroyed automatically could result in objects being
	// destroyed in the wrong order.
	m_pluginManager.reset();

	// This class depends on the TabContainer instance and needs to be destroyed before the
	// TabContainer instance is destroyed.
	m_taskbarThumbnails.reset();

	delete m_pStatusBar;

	return 0;
}

void Explorerplusplus::StartDirectoryMonitoringForTab(const Tab &tab)
{
	if (tab.GetShellBrowserImpl()->InVirtualFolder())
	{
		return;
	}

	DirectoryAltered *directoryAltered = (DirectoryAltered *) malloc(sizeof(DirectoryAltered));

	directoryAltered->iIndex = tab.GetId();
	directoryAltered->iFolderIndex = tab.GetShellBrowserImpl()->GetUniqueFolderId();
	directoryAltered->pData = this;

	std::wstring directoryToWatch = tab.GetShellBrowserImpl()->GetDirectory();

	/* Start monitoring the directory that was opened. */
	LOG(INFO) << "Starting directory monitoring for \"" << wstrToUtf8Str(directoryToWatch) << "\"";
	auto dirMonitorId = m_pDirMon->WatchDirectory(directoryToWatch.c_str(),
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_DIR_NAME
			| FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_LAST_WRITE
			| FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION
			| FILE_NOTIFY_CHANGE_SECURITY,
		DirectoryAlteredCallback, FALSE, (void *) directoryAltered);

	if (!dirMonitorId)
	{
		return;
	}

	tab.GetShellBrowserImpl()->SetDirMonitorId(*dirMonitorId);
}

void Explorerplusplus::StopDirectoryMonitoringForTab(const Tab &tab)
{
	auto dirMonitorId = tab.GetShellBrowserImpl()->GetDirMonitorId();

	if (!dirMonitorId)
	{
		return;
	}

	m_pDirMon->StopDirectoryMonitor(*dirMonitorId);
	tab.GetShellBrowserImpl()->ClearDirMonitorId();
}

void Explorerplusplus::OnDisplayWindowResized(WPARAM wParam)
{
	if (m_config->displayWindowVertical)
	{
		m_displayWindowWidth = LOWORD(wParam);
	}
	else
	{
		m_displayWindowHeight = HIWORD(wParam);
	}

	UpdateLayout();
}

/* Cycle through the current views. */
void Explorerplusplus::OnToolbarViews()
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->CycleViewMode(true);
}

// This is used for both Tab/Shift+Tab and F6/Shift+F6. While IsDialogMessage() could be used to
// handle Tab/Shift+Tab, the key combinations in this case are synonyms for each other. Since
// F6/Shift+F6 would need to be handled manually anyway, handling both with the same function
// ensures that they have identical behavior.
void Explorerplusplus::OnFocusNextWindow(FocusChangeDirection direction)
{
	HWND focus = GetFocus();
	HWND initialControl;

	// The focus should always be on one of the child windows, but this function should still set
	// the focus even if there is no current focus or the focus is on the parent.
	// GetNextDlgTabItem() may fail if the initial control is NULL or the parent window, so that
	// situation is prevented here.
	if (focus && IsChild(m_hContainer, focus))
	{
		initialControl = focus;
	}
	else
	{
		initialControl = GetWindow(m_hContainer, GW_CHILD);
	}

	HWND nextWindow = GetNextDlgTabItem(m_hContainer, initialControl,
		direction == FocusChangeDirection::Previous);
	assert(nextWindow);

	if (nextWindow)
	{
		SetFocus(nextWindow);
	}
}

void Explorerplusplus::OnAppCommand(UINT cmd)
{
	switch (cmd)
	{
	case APPCOMMAND_BROWSER_BACKWARD:
		/* This will cancel any menu that may be shown
		at the moment. */
		SendMessage(m_hContainer, WM_CANCELMODE, 0, 0);
		m_commandController.ExecuteCommand(IDM_GO_BACK);
		break;

	case APPCOMMAND_BROWSER_FORWARD:
		SendMessage(m_hContainer, WM_CANCELMODE, 0, 0);
		m_commandController.ExecuteCommand(IDM_GO_FORWARD);
		break;

	case APPCOMMAND_BROWSER_HOME:
		OnGoHome();
		break;

	case APPCOMMAND_BROWSER_FAVORITES:
		break;

	case APPCOMMAND_BROWSER_REFRESH:
		SendMessage(m_hContainer, WM_CANCELMODE, 0, 0);
		OnRefresh();
		break;

	case APPCOMMAND_BROWSER_SEARCH:
		OnSearch();
		break;

	case APPCOMMAND_CLOSE:
		SendMessage(m_hContainer, WM_CANCELMODE, 0, 0);
		OnCloseTab();
		break;

	case APPCOMMAND_COPY:
		OnCopy(TRUE);
		break;

	case APPCOMMAND_CUT:
		OnCopy(FALSE);
		break;

	case APPCOMMAND_HELP:
		OnOpenOnlineDocumentation();
		break;

	case APPCOMMAND_NEW:
		break;

	case APPCOMMAND_PASTE:
		OnPaste();
		break;

	case APPCOMMAND_UNDO:
		m_FileActionHandler.Undo();
		break;

	case APPCOMMAND_REDO:
		break;
	}
}

void Explorerplusplus::OnRefresh()
{
	Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	tab.GetShellBrowserImpl()->GetNavigationController()->Refresh();
}

void Explorerplusplus::CopyColumnInfoToClipboard()
{
	auto currentColumns = m_pActiveShellBrowser->GetCurrentColumns();

	std::wstring strColumnInfo;
	int nActiveColumns = 0;

	for (const auto &column : currentColumns)
	{
		if (column.checked)
		{
			auto columnName = ResourceHelper::LoadString(m_app->GetResourceInstance(),
				ShellBrowserImpl::LookupColumnNameStringIndex(column.type));
			strColumnInfo += columnName + L"\t";

			nActiveColumns++;
		}
	}

	/* Remove the trailing tab. */
	strColumnInfo = strColumnInfo.substr(0, strColumnInfo.size() - 1);

	strColumnInfo += _T("\r\n");

	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		for (int i = 0; i < nActiveColumns; i++)
		{
			TCHAR szText[64];
			ListView_GetItemText(m_hActiveListView, iItem, i, szText, std::size(szText));

			strColumnInfo += std::wstring(szText) + _T("\t");
		}

		strColumnInfo = strColumnInfo.substr(0, strColumnInfo.size() - 1);

		strColumnInfo += _T("\r\n");
	}

	/* Remove the trailing newline. */
	strColumnInfo = strColumnInfo.substr(0, strColumnInfo.size() - 2);

	BulkClipboardWriter clipboardWriter;
	clipboardWriter.WriteText(strColumnInfo);
}

void Explorerplusplus::OnDirectoryModified(const Tab &tab)
{
	if (GetActivePane()->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateStatusBarText(tab);
		UpdateDisplayWindow(tab);
	}
}

/* A file association has changed. Rather
than refreshing all tabs, just find all
icons again.

To refresh system image list:
1. Call FileIconInit(TRUE)
2. Change "Shell Icon Size" in "Control Panel\\Desktop\\WindowMetrics"
3. Call FileIconInit(FALSE)

Note that refreshing the system image list affects
the WHOLE PROGRAM. This means that the treeview
needs to have its icons refreshed as well.

References:
http://tech.groups.yahoo.com/group/wtl/message/13911
http://www.eggheadcafe.com/forumarchives/platformsdkshell/Nov2005/post24294253.asp
*/
void Explorerplusplus::OnAssocChanged()
{
	typedef BOOL(WINAPI * FII_PROC)(BOOL);
	FII_PROC fileIconInit;
	HKEY hKey;
	HMODULE hShell32;
	TCHAR szTemp[32];
	DWORD dwShellIconSize;
	LONG res;

	hShell32 = LoadLibrary(_T("shell32.dll"));

	fileIconInit = (FII_PROC) GetProcAddress(hShell32, (LPCSTR) 660);

	res = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop\\WindowMetrics"), 0,
		KEY_READ | KEY_WRITE, &hKey);

	if (res == ERROR_SUCCESS)
	{
		std::wstring shellIconSize;
		RegistrySettings::ReadString(hKey, _T("Shell Icon Size"), shellIconSize);

		dwShellIconSize = _wtoi(shellIconSize.c_str());

		/* Increment the value by one, and save it back to the registry. */
		StringCchPrintf(szTemp, std::size(szTemp), _T("%d"), dwShellIconSize + 1);
		RegistrySettings::SaveString(hKey, _T("Shell Icon Size"), szTemp);

		if (fileIconInit != nullptr)
			fileIconInit(TRUE);

		/* Now, set it back to the original value. */
		RegistrySettings::SaveString(hKey, _T("Shell Icon Size"), shellIconSize);

		if (fileIconInit != nullptr)
			fileIconInit(FALSE);

		RegCloseKey(hKey);
	}

	/* DO NOT free shell32.dll. Doing so will release
	the image lists (among other things). */

	/* When the system image list is refresh, ALL previous
	icons will be discarded. This means that SHGetFileInfo()
	needs to be called to get each files icon again. */

	/* Now, go through each tab, and refresh each icon. */
	for (auto &tab : GetActivePane()->GetTabContainer()->GetAllTabs() | boost::adaptors::map_values)
	{
		tab->GetShellBrowserImpl()->GetNavigationController()->Refresh();
	}

	/* Now, refresh the treeview. */
	m_shellTreeView->RefreshAllIcons();

	/* TODO: Update the address bar. */
}

void Explorerplusplus::OnNewWindow()
{
	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(m_hContainer, &placement);
	CHECK(res);

	constexpr int windowOffsetInPixels = 10;

	RECT bounds = placement.rcNormalPosition;
	OffsetRect(&bounds, windowOffsetInPixels, windowOffsetInPixels);

	WindowStorageData initialData;
	initialData.bounds = bounds;
	initialData.showState = NativeShowStateToShowState(placement.showCmd);
	initialData.treeViewWidth = m_treeViewWidth;
	initialData.displayWindowWidth = m_displayWindowWidth;
	initialData.displayWindowHeight = m_displayWindowHeight;

	Explorerplusplus::Create(m_app, &initialData);
}

void Explorerplusplus::OnCloneWindow()
{
	std::wstring currentDirectory = m_pActiveShellBrowser->GetDirectory();

	TCHAR szQuotedCurrentDirectory[MAX_PATH];
	StringCchPrintf(szQuotedCurrentDirectory, std::size(szQuotedCurrentDirectory), _T("\"%s\""),
		currentDirectory.c_str());

	LaunchCurrentProcess(m_hContainer, szQuotedCurrentDirectory);
}

void Explorerplusplus::OnDisplayWindowRClick(POINT *ptClient)
{
	wil::unique_hmenu parentMenu(
		LoadMenu(m_app->GetResourceInstance(), MAKEINTRESOURCE(IDR_DISPLAYWINDOW_RCLICK)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	MenuHelper::CheckItem(menu, IDM_DISPLAYWINDOW_VERTICAL, m_config->displayWindowVertical);

	POINT ptScreen = *ptClient;
	BOOL res = ClientToScreen(m_displayWindow->GetHWND(), &ptScreen);

	if (!res)
	{
		return;
	}

	TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL, ptScreen.x, ptScreen.y, 0,
		m_hContainer, nullptr);
}

void Explorerplusplus::OnSortBy(SortMode sortMode)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	SortMode currentSortMode = selectedTab.GetShellBrowserImpl()->GetSortMode();

	if (sortMode == currentSortMode)
	{
		selectedTab.GetShellBrowserImpl()->SetSortDirection(
			InvertSortDirection(selectedTab.GetShellBrowserImpl()->GetSortDirection()));
	}
	else
	{
		selectedTab.GetShellBrowserImpl()->SetSortMode(sortMode);
	}
}

void Explorerplusplus::OnGroupBy(SortMode groupMode)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	SortMode currentGroupMode = selectedTab.GetShellBrowserImpl()->GetGroupMode();

	if (selectedTab.GetShellBrowserImpl()->GetShowInGroups() && groupMode == currentGroupMode)
	{
		selectedTab.GetShellBrowserImpl()->SetGroupSortDirection(
			InvertSortDirection(selectedTab.GetShellBrowserImpl()->GetGroupSortDirection()));
	}
	else
	{
		selectedTab.GetShellBrowserImpl()->SetGroupMode(groupMode);
		selectedTab.GetShellBrowserImpl()->SetShowInGroups(true);
	}
}

void Explorerplusplus::OnGroupByNone()
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->SetShowInGroups(false);
}

void Explorerplusplus::OnSortDirectionSelected(SortDirection direction)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->SetSortDirection(direction);
}

void Explorerplusplus::OnGroupSortDirectionSelected(SortDirection direction)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->SetGroupSortDirection(direction);
}

const Config *Explorerplusplus::GetConfig() const
{
	return m_config;
}

HINSTANCE Explorerplusplus::GetResourceInstance() const
{
	return m_app->GetResourceInstance();
}

AcceleratorManager *Explorerplusplus::GetAcceleratorManager() const
{
	return m_app->GetAcceleratorManager();
}

HWND Explorerplusplus::GetMainWindow() const
{
	return m_hContainer;
}

ShellBrowserImpl *Explorerplusplus::GetActiveShellBrowserImpl() const
{
	return m_pActiveShellBrowser;
}

CoreInterface *Explorerplusplus::GetCoreInterface()
{
	return this;
}

TabContainer *Explorerplusplus::GetTabContainer() const
{
	return GetActivePane()->GetTabContainer();
}

HWND Explorerplusplus::GetTreeView() const
{
	return m_shellTreeView->GetHWND();
}

IDirectoryMonitor *Explorerplusplus::GetDirectoryMonitor() const
{
	return m_pDirMon;
}

CachedIcons *Explorerplusplus::GetCachedIcons()
{
	return m_app->GetCachedIcons();
}

void Explorerplusplus::OnShowHiddenFiles()
{
	Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	tab.GetShellBrowserImpl()->SetShowHidden(!tab.GetShellBrowserImpl()->GetShowHidden());
	tab.GetShellBrowserImpl()->GetNavigationController()->Refresh();
}

void Explorerplusplus::FocusChanged()
{
	m_focusChangedSignal();
}

boost::signals2::connection Explorerplusplus::AddFocusChangeObserver(
	const FocusChangedSignal::slot_type &observer)
{
	CHECK(!m_browserClosing)
		<< "Adding a focus changed observer to a browser window while it's being closed is unsafe";

	return m_focusChangedSignal.connect(observer);
}

void Explorerplusplus::FocusActiveTab()
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	SetFocus(selectedTab.GetShellBrowserImpl()->GetListView());
}

bool Explorerplusplus::OnActivate(int activationState, bool minimized)
{
	// This may be called while the window is being constructed, before it has been added to the
	// browser list. In that case, the window won't be visible and there's no need to try and set it
	// as the active browser.
	if (IsWindowVisible(m_hContainer) && activationState != WA_INACTIVE && !minimized)
	{
		m_app->GetBrowserList()->SetLastActive(this);
	}

	if (activationState == WA_INACTIVE)
	{
		m_lastActiveWindow = GetFocus();
	}
	else
	{
		if (!minimized && m_lastActiveWindow)
		{
			SetFocus(m_lastActiveWindow);
			return true;
		}
	}

	return false;
}

void Explorerplusplus::OnChangeMainFontSize(FontSizeType sizeType)
{
	auto &mainFont = m_config->mainFont.get();
	std::wstring updatedFontName;
	int updatedFontSize;

	if (mainFont)
	{
		updatedFontName = mainFont->GetName();
		updatedFontSize = mainFont->GetSize();
	}
	else
	{
		auto systemLogFont = GetDefaultSystemFontScaledToWindow(m_hContainer);
		int systemFontSize = std::abs(
			DpiCompatibility::GetInstance().PixelsToPoints(m_hContainer, systemLogFont.lfHeight));

		updatedFontName = systemLogFont.lfFaceName;
		updatedFontSize = systemFontSize;
	}

	if (sizeType == FontSizeType::Decrease)
	{
		updatedFontSize -= FONT_SIZE_CHANGE_DELTA;
	}
	else
	{
		updatedFontSize += FONT_SIZE_CHANGE_DELTA;
	}

	m_config->mainFont = CustomFont(updatedFontName, updatedFontSize);
}

void Explorerplusplus::OnResetMainFontSize()
{
	auto &mainFont = m_config->mainFont.get();

	if (!mainFont)
	{
		// The default font is being used, so the font size is currently the default size and
		// nothing needs to change.
		return;
	}

	auto systemLogFont = GetDefaultSystemFontScaledToWindow(m_hContainer);
	int systemFontSize = std::abs(
		DpiCompatibility::GetInstance().PixelsToPoints(m_hContainer, systemLogFont.lfHeight));

	// Different fonts can have different metrics, so there isn't really a concept of a "default"
	// font size. The size of the default system font is taken as a reasonable proxy. This also
	// means that if the user only changes the font size (while still using the system font),
	// resetting the font size will work as expected.
	m_config->mainFont = CustomFont(mainFont->GetName(), systemFontSize);
}
