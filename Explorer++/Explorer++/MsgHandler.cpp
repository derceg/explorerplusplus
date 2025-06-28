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
#include "ShellBrowser/ColumnHelper.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellTreeView/ShellTreeView.h"
#include "StatusBar.h"
#include "StatusBarView.h"
#include "Storage.h"
#include "SystemFontHelper.h"
#include "TabBacking.h"
#include "TabContainer.h"
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

		if (m_config->openContainerFiles)
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

		openAsFolder = isFolder || (isContainerFile && m_config->openContainerFiles);
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
		GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams, { .selected = true });
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
	if (m_app->GetFeatureList()->IsEnabled(Feature::MultipleWindowsPerSession))
	{
		CreateNewWindow({ { .pidl = pidlDirectory } });
	}
	else
	{
		// Create a new instance of this program, with the specified path as an argument.
		std::wstring path;
		GetDisplayName(pidlDirectory, SHGDN_FORPARSING, path);

		TCHAR szParameters[512];
		StringCchPrintf(szParameters, std::size(szParameters), _T("\"%s\""), path.c_str());

		LaunchCurrentProcess(m_hContainer, szParameters);
	}
}

void Explorerplusplus::OpenFileItem(const std::wstring &itemPath, const std::wstring &parameters)
{
	auto shellBrowser = GetActiveShellBrowserImpl();
	ExecuteFileAction(m_hContainer, itemPath, L"", parameters,
		shellBrowser->InVirtualFolder() ? L"" : shellBrowser->GetDirectoryPath().c_str());
}

void Explorerplusplus::OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const std::wstring &parameters)
{
	auto shellBrowser = GetActiveShellBrowserImpl();
	ExecuteFileAction(m_hContainer, pidlItem, L"", parameters,
		shellBrowser->InVirtualFolder() ? L"" : shellBrowser->GetDirectoryPath().c_str());
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
	if (GetLifecycleState() != LifecycleState::Main)
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

	if (m_config->showStatusBar.get())
	{
		RECT statusBarRect;
		GetWindowRect(m_statusBar->GetView()->GetHWND(), &statusBarRect);
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
	SetWindowPos(m_tabBacking->GetHWND(), nullptr, tabBackingLeft, tabTop, tabBackingWidth,
		tabWindowHeight, showFlags);

	SetWindowPos(GetActivePane()->GetTabContainer()->GetHWND(), nullptr, 0, 0, tabBackingWidth - 25,
		tabWindowHeight, SWP_SHOWWINDOW | SWP_NOZORDER);

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
		(m_config->showFolders.get() ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOZORDER);

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

	RECT statusBarRect;
	GetWindowRect(m_statusBar->GetView()->GetHWND(), &statusBarRect);

	UINT statusBarShowFlags =
		(m_config->showStatusBar.get() ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOZORDER;
	SetWindowPos(m_statusBar->GetView()->GetHWND(), nullptr, 0,
		mainWindowHeight - GetRectHeight(&statusBarRect), mainWindowWidth,
		GetRectHeight(&statusBarRect), statusBarShowFlags);
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

	if (hwnd == m_tabBacking->GetHWND())
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
	SetLifecycleState(LifecycleState::Closing);

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

	return 0;
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
		m_commandController.ExecuteCommand(IDA_HOME);
		break;

	case APPCOMMAND_BROWSER_FAVORITES:
		break;

	case APPCOMMAND_BROWSER_REFRESH:
		SendMessage(m_hContainer, WM_CANCELMODE, 0, 0);
		m_commandController.ExecuteCommand(IDM_VIEW_REFRESH);
		break;

	case APPCOMMAND_BROWSER_SEARCH:
		OnSearch();
		break;

	case APPCOMMAND_CLOSE:
		m_commandController.ExecuteCommand(IDM_FILE_CLOSETAB);
		break;

	case APPCOMMAND_CUT:
		m_commandController.ExecuteCommand(IDM_EDIT_CUT);
		break;

	case APPCOMMAND_COPY:
		m_commandController.ExecuteCommand(IDM_EDIT_COPY);
		break;

	case APPCOMMAND_HELP:
		m_commandController.ExecuteCommand(IDM_HELP_ONLINE_DOCUMENTATION);
		break;

	case APPCOMMAND_NEW:
		break;

	case APPCOMMAND_PASTE:
		OnPaste();
		break;

	case APPCOMMAND_UNDO:
		m_fileActionHandler.Undo();
		break;

	case APPCOMMAND_REDO:
		break;
	}
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
			strColumnInfo += GetColumnName(m_app->GetResourceLoader(), column.type) + L"\t";

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

	BulkClipboardWriter clipboardWriter(m_app->GetClipboardStore());
	clipboardWriter.WriteText(strColumnInfo);
}

void Explorerplusplus::OnDirectoryContentsChanged(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();
	UpdateDisplayWindow(*tab);
}

void Explorerplusplus::CreateNewWindow(const std::vector<TabStorageData> &tabs)
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
	initialData.tabs = tabs;

	Explorerplusplus::Create(m_app, &initialData);
}

void Explorerplusplus::OnCloneWindow()
{
	std::wstring currentDirectory = m_pActiveShellBrowser->GetDirectoryPath();

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

void Explorerplusplus::OnGroupSortDirectionSelected(SortDirection direction)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->SetGroupSortDirection(direction);
}

HWND Explorerplusplus::GetMainWindow() const
{
	return m_hContainer;
}

ShellBrowserImpl *Explorerplusplus::GetActiveShellBrowserImpl() const
{
	return m_pActiveShellBrowser;
}

TabEvents *Explorerplusplus::GetTabEvents()
{
	return m_app->GetTabEvents();
}

TabContainer *Explorerplusplus::GetTabContainer() const
{
	return GetActivePane()->GetTabContainer();
}

void Explorerplusplus::OnShowHiddenFiles()
{
	Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	tab.GetShellBrowserImpl()->SetShowHidden(!tab.GetShellBrowserImpl()->GetShowHidden());
	tab.GetShellBrowserImpl()->GetNavigationController()->Refresh();
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
