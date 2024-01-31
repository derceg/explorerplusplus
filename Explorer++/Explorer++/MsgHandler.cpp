// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "ColorRule.h"
#include "Config.h"
#include "DarkModeHelper.h"
#include "Explorer++_internal.h"
#include "HolderWindow.h"
#include "LoadSaveRegistry.h"
#include "LoadSaveXml.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "Plugins/PluginManager.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellTreeView/ShellTreeView.h"
#include "SystemFontHelper.h"
#include "TabContainer.h"
#include "ToolbarHelper.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Macros.h"
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

void Explorerplusplus::TestConfigFile()
{
	m_bLoadSettingsFromXML = TestConfigFileInternal();
}

BOOL TestConfigFileInternal()
{
	HANDLE hConfigFile;
	TCHAR szConfigFile[MAX_PATH];
	BOOL bLoadSettingsFromXML = FALSE;

	/* To ensure the configuration file is loaded from the same directory
	as the executable, determine the fully qualified path of the executable,
	then save the configuration file in that directory. */
	GetProcessImageName(GetCurrentProcessId(), szConfigFile, SIZEOF_ARRAY(szConfigFile));

	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, NExplorerplusplus::XML_FILENAME);

	hConfigFile =
		CreateFile(szConfigFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hConfigFile != INVALID_HANDLE_VALUE)
	{
		bLoadSettingsFromXML = TRUE;

		CloseHandle(hConfigFile);
	}

	return bLoadSettingsFromXML;
}

void Explorerplusplus::LoadAllSettings(ILoadSave **pLoadSave)
{
	/* Tests for the existence of the configuration
	file. If the file is present, a flag is set
	indicating that the config file should be used
	to load settings. */
	TestConfigFile();

	/* Initialize the LoadSave interface. Note
	that this interface must be regenerated when
	saving, as it's possible for the save/load
	methods to be different. */
	if (m_bLoadSettingsFromXML)
	{
		*pLoadSave = new LoadSaveXML(this, TRUE);

		/* When loading from the config file, also
		set the option to save back to it on exit. */
		m_bSavePreferencesToXMLFile = TRUE;
	}
	else
	{
		*pLoadSave = new LoadSaveRegistry(this);
	}

	(*pLoadSave)->LoadBookmarks();
	(*pLoadSave)->LoadGenericSettings();
	(*pLoadSave)->LoadDefaultColumns();
	(*pLoadSave)->LoadApplicationToolbar();
	(*pLoadSave)->LoadToolbarInformation();
	(*pLoadSave)->LoadColorRules();
	(*pLoadSave)->LoadDialogStates();

	ValidateLoadedSettings();
}

void Explorerplusplus::OpenItem(const std::wstring &itemPath,
	OpenFolderDisposition openFolderDisposition)
{
	unique_pidl_absolute pidlItem;
	HRESULT hr =
		SHParseDisplayName(itemPath.c_str(), nullptr, wil::out_param(pidlItem), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		OpenItem(pidlItem.get(), openFolderDisposition);
	}
}

void Explorerplusplus::OpenItem(PCIDLIST_ABSOLUTE pidlItem,
	OpenFolderDisposition openFolderDisposition)
{
	BOOL bControlPanelParent = FALSE;

	unique_pidl_absolute pidlControlPanel;
	HRESULT hr =
		SHGetFolderLocation(nullptr, CSIDL_CONTROLS, nullptr, 0, wil::out_param(pidlControlPanel));

	if (SUCCEEDED(hr))
	{
		/* Check if the parent of the item is the control panel.
		If it is, pass it to the shell to open, rather than
		opening it in-place. */
		if (ILIsParent(pidlControlPanel.get(), pidlItem, FALSE)
			&& !ArePidlsEquivalent(pidlControlPanel.get(), pidlItem))
		{
			bControlPanelParent = TRUE;
		}
	}

	/* On Vista and later, the Control Panel was split into
	two completely separate views:
	 - Icon View
	 - Category View
	Icon view is essentially the same view provided in
	Windows XP and earlier (i.e. a simple, flat listing of
	all the items in the control panel).
	Category view, on the other hand, groups similar
	Control Panel items under several broad categories.
	It is important to note that both these 'views' are
	represented by different GUID's, and are NOT the
	same folder.
	 - Icon View:
	   ::{21EC2020-3AEA-1069-A2DD-08002B30309D} (Vista and Win 7)
	   ::{26EE0668-A00A-44D7-9371-BEB064C98683}\0 (Win 7)
	 - Category View:
	   ::{26EE0668-A00A-44D7-9371-BEB064C98683} (Vista and Win 7)
	*/
	if (!bControlPanelParent)
	{
		unique_pidl_absolute pidlControlPanelCategoryView;
		hr = SHParseDisplayName(CONTROL_PANEL_CATEGORY_VIEW, nullptr,
			wil::out_param(pidlControlPanelCategoryView), 0, nullptr);

		if (SUCCEEDED(hr))
		{
			/* Check if the parent of the item is the control panel.
			If it is, pass it to the shell to open, rather than
			opening it in-place. */
			if (ILIsParent(pidlControlPanelCategoryView.get(), pidlItem, FALSE)
				&& !ArePidlsEquivalent(pidlControlPanelCategoryView.get(), pidlItem))
			{
				bControlPanelParent = TRUE;
			}
		}
	}

	SFGAOF uAttributes = SFGAO_FOLDER | SFGAO_STREAM | SFGAO_LINK;
	hr = GetItemAttributes(pidlItem, &uAttributes);

	if (SUCCEEDED(hr))
	{
		if ((uAttributes & SFGAO_FOLDER) && (uAttributes & SFGAO_STREAM))
		{
			/* Zip file. */
			if (m_config->handleZipFiles)
			{
				OpenFolderItem(pidlItem, openFolderDisposition);
			}
			else
			{
				OpenFileItem(pidlItem, EMPTY_STRING);
			}
		}
		else if (((uAttributes & SFGAO_FOLDER) && !bControlPanelParent))
		{
			OpenFolderItem(pidlItem, openFolderDisposition);
		}
		else if (uAttributes & SFGAO_LINK && !bControlPanelParent)
		{
			/* This item is a shortcut. */
			TCHAR szTargetPath[MAX_PATH];

			std::wstring itemPath;
			GetDisplayName(pidlItem, SHGDN_FORPARSING, itemPath);

			hr = NFileOperations::ResolveLink(m_hContainer, 0, itemPath.c_str(), szTargetPath,
				SIZEOF_ARRAY(szTargetPath));

			if (hr == S_OK)
			{
				/* The target of the shortcut was found
				successfully. Query it to determine whether
				it is a folder or not. */
				uAttributes = SFGAO_FOLDER | SFGAO_STREAM;
				hr = GetItemAttributes(szTargetPath, &uAttributes);

				/* Note this is functionally equivalent to
				recursively calling this function again.
				However, the link may be arbitrarily deep
				(or point to itself). Therefore, DO NOT
				call this function recursively with itself
				without some way of stopping. */
				if (SUCCEEDED(hr))
				{
					/* Is this a link to a folder or zip file? */
					if (((uAttributes & SFGAO_FOLDER) && !(uAttributes & SFGAO_STREAM))
						|| ((uAttributes & SFGAO_FOLDER) && (uAttributes & SFGAO_STREAM)
							&& m_config->handleZipFiles))
					{
						unique_pidl_absolute pidlTarget;
						hr = SHParseDisplayName(szTargetPath, nullptr, wil::out_param(pidlTarget),
							0, nullptr);

						if (SUCCEEDED(hr))
						{
							OpenFolderItem(pidlTarget.get(), openFolderDisposition);
						}
					}
					else
					{
						hr = E_FAIL;
					}
				}
			}

			if (FAILED(hr))
			{
				/* It is possible the target may not resolve,
				yet the shortcut is still valid. This is the
				case with shortcut URL's for example.
				Also, even if the shortcut points to a dead
				folder, it should still attempted to be
				opened. */
				OpenFileItem(pidlItem, EMPTY_STRING);
			}
		}
		else if (bControlPanelParent && (uAttributes & SFGAO_FOLDER))
		{
			std::wstring parsingPath;
			GetDisplayName(pidlItem, SHGDN_FORPARSING, parsingPath);

			auto explorerPath = ExpandEnvironmentStringsWrapper(_T("%windir%\\explorer.exe"));

			if (explorerPath)
			{
				/* Invoke Windows Explorer directly. Note that only folder
				items need to be passed directly to Explorer. Two central
				reasons:
				1. Explorer can only open folder items.
				2. Non-folder items can be opened directly (regardless of
				whether or not they're children of the control panel). */
				ShellExecute(m_hContainer, _T("open"), explorerPath->c_str(), parsingPath.c_str(),
					nullptr, SW_SHOWNORMAL);
			}
		}
		else
		{
			/* File item. */
			OpenFileItem(pidlItem, EMPTY_STRING);
		}
	}
}

void Explorerplusplus::OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem,
	OpenFolderDisposition openFolderDisposition)
{
	if (m_config->alwaysOpenNewTab && openFolderDisposition == OpenFolderDisposition::CurrentTab)
	{
		openFolderDisposition = OpenFolderDisposition::ForegroundTab;
	}

	switch (openFolderDisposition)
	{
	case OpenFolderDisposition::CurrentTab:
	{
		Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
		auto navigateParams = NavigateParams::Normal(pidlItem);
		tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
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
	}
}

void Explorerplusplus::OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory)
{
	/* Create a new instance of this program, with the
	specified path as an argument. */
	std::wstring path;
	GetDisplayName(pidlDirectory, SHGDN_FORPARSING, path);

	TCHAR szParameters[512];
	StringCchPrintf(szParameters, SIZEOF_ARRAY(szParameters), _T("\"%s\""), path.c_str());

	ExecuteAndShowCurrentProcess(m_hContainer, szParameters);
}

void Explorerplusplus::OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const TCHAR *szParameters)
{
	unique_pidl_absolute pidlParent(ILCloneFull(pidlItem));
	ILRemoveLastID(pidlParent.get());

	std::wstring itemDirectory;
	GetDisplayName(pidlParent.get(), SHGDN_FORPARSING, itemDirectory);

	ExecuteFileAction(m_hContainer, EMPTY_STRING, szParameters, itemDirectory.c_str(), pidlItem);
}

OpenFolderDisposition Explorerplusplus::DetermineOpenDisposition(bool isMiddleButtonDown,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (isMiddleButtonDown || isCtrlKeyDown)
	{
		if (!isShiftKeyDown)
		{
			if (m_config->openTabsInForeground)
			{
				return OpenFolderDisposition::ForegroundTab;
			}
			else
			{
				return OpenFolderDisposition::BackgroundTab;
			}
		}
		else
		{
			// Shift inverts the usual behavior.
			if (m_config->openTabsInForeground)
			{
				return OpenFolderDisposition::BackgroundTab;
			}
			else
			{
				return OpenFolderDisposition::ForegroundTab;
			}
		}
	}

	if (isShiftKeyDown)
	{
		return OpenFolderDisposition::NewWindow;
	}

	return OpenFolderDisposition::CurrentTab;
}

void Explorerplusplus::OnSize(HWND hwnd, UINT state, int mainWindowWidth, int mainWindowHeight)
{
	UNREFERENCED_PARAMETER(hwnd);

	RECT rc;
	UINT uFlags;
	int indentBottom = 0;
	int indentTop = 0;
	int indentRight = 0;
	int indentLeft = 0;
	int iIndentRebar = 0;
	int iHolderWidth;
	int iHolderHeight;
	int iHolderTop;
	int iTabBackingWidth;
	int iTabBackingLeft;

	if (!m_applicationInitialized)
	{
		return;
	}

	if (state == SIZE_MINIMIZED)
	{
		// There's no need to update the layout when the window is being minimized.
		return;
	}

	auto &dpiCompatibility = DpiCompatibility::GetInstance();

	m_config->treeViewWidth = std::clamp(m_config->treeViewWidth,
		dpiCompatibility.ScaleValue(m_treeViewHolder->GetHWND(), TREEVIEW_MINIMUM_WIDTH),
		static_cast<int>(TREEVIEW_MAXIMUM_WIDTH_PERCENTAGE * mainWindowWidth));

	RECT rebarRect;
	GetClientRect(m_hMainRebar, &rebarRect);
	SetWindowPos(m_hMainRebar, nullptr, 0, 0, mainWindowWidth, GetRectHeight(&rebarRect),
		SWP_NOZORDER | SWP_NOMOVE);

	iIndentRebar += GetRectHeight(&rebarRect);

	if (m_config->showStatusBar)
	{
		GetWindowRect(m_hStatusBar, &rc);
		indentBottom += GetRectHeight(&rc);
	}

	if (m_config->showDisplayWindow)
	{
		if (m_config->displayWindowVertical)
		{
			indentRight += m_config->displayWindowWidth;
		}
		else
		{
			indentBottom += m_config->displayWindowHeight;
		}
	}

	if (m_config->showFolders.get())
	{
		indentLeft = m_config->treeViewWidth;
	}

	// Since the display area is indicated to start at (0, 0), displayRect.top will contain the
	// height of the tab control above the display area.
	RECT displayRect = { 0, 0, 0, 0 };
	TabCtrl_AdjustRect(GetActivePane()->GetTabContainer()->GetHWND(), true, &displayRect);
	int tabWindowHeight = std::abs(displayRect.top);

	indentTop = iIndentRebar;

	if (m_bShowTabBar)
	{
		if (!m_config->showTabBarAtBottom.get())
		{
			indentTop += tabWindowHeight;
		}
	}

	/* <---- Tab control + backing ----> */

	if (m_config->extendTabControl.get())
	{
		iTabBackingLeft = 0;
		iTabBackingWidth = mainWindowWidth;
	}
	else
	{
		iTabBackingLeft = indentLeft;
		iTabBackingWidth = mainWindowWidth - indentLeft - indentRight;
	}

	uFlags = m_bShowTabBar ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;

	int iTabTop;

	if (!m_config->showTabBarAtBottom.get())
	{
		iTabTop = iIndentRebar;
	}
	else
	{
		iTabTop = mainWindowHeight - indentBottom - tabWindowHeight;
	}

	/* If we're showing the tab bar at the bottom of the listview,
	the only thing that will change is the top coordinate. */
	SetWindowPos(m_hTabBacking, m_hDisplayWindow, iTabBackingLeft, iTabTop, iTabBackingWidth,
		tabWindowHeight, uFlags);

	SetWindowPos(GetActivePane()->GetTabContainer()->GetHWND(), nullptr, 0, 0,
		iTabBackingWidth - 25, tabWindowHeight, SWP_SHOWWINDOW | SWP_NOZORDER);

	/* Tab close button. */
	int scaledCloseToolbarXOffset =
		dpiCompatibility.ScaleValue(m_hTabWindowToolbar, ToolbarHelper::CLOSE_TOOLBAR_X_OFFSET);

	RECT tabToolbarRect;
	GetClientRect(m_hTabWindowToolbar, &tabToolbarRect);
	SetWindowPos(m_hTabWindowToolbar, nullptr,
		iTabBackingWidth - GetRectWidth(&tabToolbarRect) - scaledCloseToolbarXOffset,
		(tabWindowHeight - GetRectHeight(&tabToolbarRect)) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	if (m_config->extendTabControl.get() && !m_config->showTabBarAtBottom.get())
	{
		iHolderTop = indentTop;
	}
	else
	{
		iHolderTop = iIndentRebar;
	}

	/* <---- Holder window + child windows ----> */

	if (m_config->extendTabControl.get() && m_config->showTabBarAtBottom.get() && m_bShowTabBar)
	{
		iHolderHeight = mainWindowHeight - indentBottom - iHolderTop - tabWindowHeight;
	}
	else
	{
		iHolderHeight = mainWindowHeight - indentBottom - iHolderTop;
	}

	iHolderWidth = m_config->treeViewWidth;

	SetWindowPos(m_treeViewHolder->GetHWND(), nullptr, 0, iHolderTop, iHolderWidth, iHolderHeight,
		SWP_NOZORDER);

	/* <---- Display window ----> */

	if (m_config->displayWindowVertical)
	{
		SetWindowPos(m_hDisplayWindow, NULL, mainWindowWidth - indentRight, iIndentRebar,
			m_config->displayWindowWidth, mainWindowHeight - iIndentRebar - indentBottom,
			SWP_SHOWWINDOW | SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(m_hDisplayWindow, nullptr, 0, mainWindowHeight - indentBottom, mainWindowWidth,
			m_config->displayWindowHeight, SWP_SHOWWINDOW | SWP_NOZORDER);
	}

	/* <---- ALL listview windows ----> */

	for (auto &tab : GetActivePane()->GetTabContainer()->GetAllTabs() | boost::adaptors::map_values)
	{
		uFlags = SWP_NOZORDER;

		if (GetActivePane()->GetTabContainer()->IsTabSelected(*tab))
		{
			uFlags |= SWP_SHOWWINDOW;
		}

		int width = mainWindowWidth - indentLeft - indentRight;
		int height = mainWindowHeight - indentBottom - indentTop;

		if (m_config->showTabBarAtBottom.get() && m_bShowTabBar)
		{
			height -= tabWindowHeight;
		}

		SetWindowPos(tab->GetShellBrowser()->GetListView(), NULL, indentLeft, indentTop, width,
			height, uFlags);
	}

	/* <---- Status bar ----> */

	PinStatusBar(m_hStatusBar, mainWindowWidth, mainWindowHeight);
	SetStatusBarParts(mainWindowWidth);
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
		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (!darkModeHelper.IsDarkModeEnabled())
		{
			return std::nullopt;
		}

		return reinterpret_cast<INT_PTR>(m_tabBarBackgroundBrush.get());
	}

	return std::nullopt;
}

void Explorerplusplus::OnSettingChange(const WCHAR *systemParameter)
{
	// The "ImmersiveColorSet" change notification will be sent when the user changes the dark mode
	// setting in Windows (or one of the individual Windows mode/app mode settings). Changes to the
	// Windows mode settings will be ignored, as the app mode setting is what's used to determine
	// whether a light or dark theme is used.
	if (lstrcmp(systemParameter, L"ImmersiveColorSet") == 0
		&& m_config->theme.get() == +Theme::System)
	{
		DarkModeHelper::GetInstance().EnableForApp(ShouldEnableDarkMode(m_config->theme.get()));
	}
}

void Explorerplusplus::OnThemeUpdated(Theme theme)
{
	DarkModeHelper::GetInstance().EnableForApp(ShouldEnableDarkMode(theme));
}

boost::signals2::connection Explorerplusplus::AddApplicationShuttingDownObserver(
	const ApplicationShuttingDownSignal::slot_type &observer)
{
	return m_applicationShuttingDownSignal.connect(observer);
}

int Explorerplusplus::OnDestroy()
{
	m_applicationShuttingDownSignal();
	m_applicationShuttingDown = true;

	// Broadcasting focus changed events during shutdown is both unnecessary and unsafe. It's
	// unsafe, because guarantees that are normally upheld while the application is running won't
	// necessarily be upheld while the application is shutting down. For example, normally there
	// should always be at least one tab. During shutdown, the tab container will be destroyed, so
	// the assumption that there is at least a single tab won't necessarily hold.
	// Therefore, all slots are disconnected here, as focus changes during shutdown aren't
	// meaningful anyway.
	m_focusChangedSignal.disconnect_all_slots();

	if (m_SHChangeNotifyID != 0)
	{
		SHChangeNotifyDeregister(m_SHChangeNotifyID);
	}

	delete m_pStatusBar;

	return 0;
}

void Explorerplusplus::RequestCloseApplication()
{
	if (m_config->confirmCloseTabs && (GetActivePane()->GetTabContainer()->GetNumTabs() > 1))
	{
		std::wstring message =
			ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_CLOSE_ALL_TABS);
		int response = MessageBox(m_hContainer, message.c_str(), NExplorerplusplus::APP_NAME,
			MB_ICONINFORMATION | MB_YESNO);

		if (response == IDNO)
		{
			return;
		}
	}

	// It's important that the plugins are destroyed before the main
	// window is destroyed and before this class is destroyed.
	// The first because the API binding classes may interact with the
	// UI on destruction (e.g. to remove menu entries they've added).
	// The second because the API bindings assume they can use the
	// objects passed to them until their destruction. Those objects are
	// destroyed automatically when this class is destroyed, so letting
	// the plugins be destroyed automatically could result in objects
	// being destroyed in the wrong order.
	m_pluginManager.reset();

	KillTimer(m_hContainer, AUTOSAVE_TIMER_ID);

	SaveAllSettings();

	DestroyWindow(m_hContainer);
}

void Explorerplusplus::StartDirectoryMonitoringForTab(const Tab &tab)
{
	if (tab.GetShellBrowser()->InVirtualFolder())
	{
		return;
	}

	DirectoryAltered *directoryAltered = (DirectoryAltered *) malloc(sizeof(DirectoryAltered));

	directoryAltered->iIndex = tab.GetId();
	directoryAltered->iFolderIndex = tab.GetShellBrowser()->GetUniqueFolderId();
	directoryAltered->pData = this;

	std::wstring directoryToWatch = tab.GetShellBrowser()->GetDirectory();

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

	tab.GetShellBrowser()->SetDirMonitorId(*dirMonitorId);
}

void Explorerplusplus::StopDirectoryMonitoringForTab(const Tab &tab)
{
	auto dirMonitorId = tab.GetShellBrowser()->GetDirMonitorId();

	if (!dirMonitorId)
	{
		return;
	}

	m_pDirMon->StopDirectoryMonitor(*dirMonitorId);
	tab.GetShellBrowser()->ClearDirMonitorId();
}

void Explorerplusplus::OnDisplayWindowResized(WPARAM wParam)
{
	if (m_config->displayWindowVertical)
	{
		m_config->displayWindowWidth = max(LOWORD(wParam), DISPLAY_WINDOW_MINIMUM_WIDTH);
	}
	else
	{
		m_config->displayWindowHeight = max(HIWORD(wParam), DISPLAY_WINDOW_MINIMUM_HEIGHT);
	}

	RECT rc;
	GetClientRect(m_hContainer, &rc);
	SendMessage(m_hContainer, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
}

/* Cycle through the current views. */
void Explorerplusplus::OnToolbarViews()
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->CycleViewMode(true);
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

void Explorerplusplus::OnLockToolbars()
{
	REBARBANDINFO rbbi;
	UINT nBands;
	UINT i = 0;

	m_config->lockToolbars = !m_config->lockToolbars;

	nBands = (UINT) SendMessage(m_hMainRebar, RB_GETBANDCOUNT, 0, 0);

	for (i = 0; i < nBands; i++)
	{
		/* First, retrieve the current style for this band. */
		rbbi.cbSize = sizeof(REBARBANDINFO);
		rbbi.fMask = RBBIM_STYLE;
		SendMessage(m_hMainRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);

		/* Add the gripper style. */
		AddGripperStyle(&rbbi.fStyle, !m_config->lockToolbars);

		/* Now, set the new style. */
		SendMessage(m_hMainRebar, RB_SETBANDINFO, i, (LPARAM) &rbbi);
	}

	/* If the rebar is locked, prevent items from
	been rearranged. */
	AddWindowStyle(m_hMainRebar, RBS_FIXEDORDER, m_config->lockToolbars);
}

void Explorerplusplus::OnAppCommand(UINT cmd)
{
	switch (cmd)
	{
	case APPCOMMAND_BROWSER_BACKWARD:
		/* This will cancel any menu that may be shown
		at the moment. */
		SendMessage(m_hContainer, WM_CANCELMODE, 0, 0);
		OnGoBack();
		break;

	case APPCOMMAND_BROWSER_FORWARD:
		SendMessage(m_hContainer, WM_CANCELMODE, 0, 0);
		OnGoForward();
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
	tab.GetShellBrowser()->GetNavigationController()->Refresh();
}

void Explorerplusplus::CopyColumnInfoToClipboard()
{
	auto currentColumns = m_pActiveShellBrowser->GetCurrentColumns();

	std::wstring strColumnInfo;
	int nActiveColumns = 0;

	for (const auto &column : currentColumns)
	{
		if (column.bChecked)
		{
			TCHAR szText[64];
			LoadString(m_resourceInstance, ShellBrowser::LookupColumnNameStringIndex(column.type),
				szText, SIZEOF_ARRAY(szText));

			strColumnInfo += std::wstring(szText) + _T("\t");

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
			ListView_GetItemText(m_hActiveListView, iItem, i, szText, SIZEOF_ARRAY(szText));

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
		StringCchPrintf(szTemp, SIZEOF_ARRAY(szTemp), _T("%d"), dwShellIconSize + 1);
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
		tab->GetShellBrowser()->GetNavigationController()->Refresh();
	}

	/* Now, refresh the treeview. */
	m_shellTreeView->RefreshAllIcons();

	/* TODO: Update the address bar. */
}

void Explorerplusplus::OnCloneWindow()
{
	std::wstring currentDirectory = m_pActiveShellBrowser->GetDirectory();

	TCHAR szQuotedCurrentDirectory[MAX_PATH];
	StringCchPrintf(szQuotedCurrentDirectory, SIZEOF_ARRAY(szQuotedCurrentDirectory), _T("\"%s\""),
		currentDirectory.c_str());

	ExecuteAndShowCurrentProcess(m_hContainer, szQuotedCurrentDirectory);
}

void Explorerplusplus::ShowMainRebarBand(HWND hwnd, BOOL bShow)
{
	REBARBANDINFO rbi;
	LRESULT lResult;
	UINT nBands;
	UINT i = 0;

	nBands = (UINT) SendMessage(m_hMainRebar, RB_GETBANDCOUNT, 0, 0);

	for (i = 0; i < nBands; i++)
	{
		rbi.cbSize = sizeof(rbi);
		rbi.fMask = RBBIM_CHILD;
		lResult = SendMessage(m_hMainRebar, RB_GETBANDINFO, i, (LPARAM) &rbi);

		if (lResult)
		{
			if (hwnd == rbi.hwndChild)
			{
				SendMessage(m_hMainRebar, RB_SHOWBAND, i, bShow);
				break;
			}
		}
	}
}

void Explorerplusplus::OnDisplayWindowRClick(POINT *ptClient)
{
	wil::unique_hmenu parentMenu(
		LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_DISPLAYWINDOW_RCLICK)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	MenuHelper::CheckItem(menu, IDM_DISPLAYWINDOW_VERTICAL, m_config->displayWindowVertical);

	POINT ptScreen = *ptClient;
	BOOL res = ClientToScreen(m_hDisplayWindow, &ptScreen);

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
	SortMode currentSortMode = selectedTab.GetShellBrowser()->GetSortMode();

	if (sortMode == currentSortMode)
	{
		selectedTab.GetShellBrowser()->SetSortDirection(
			InvertSortDirection(selectedTab.GetShellBrowser()->GetSortDirection()));
	}
	else
	{
		selectedTab.GetShellBrowser()->SetSortMode(sortMode);
	}
}

void Explorerplusplus::OnGroupBy(SortMode groupMode)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	SortMode currentGroupMode = selectedTab.GetShellBrowser()->GetGroupMode();

	if (selectedTab.GetShellBrowser()->GetShowInGroups() && groupMode == currentGroupMode)
	{
		selectedTab.GetShellBrowser()->SetGroupSortDirection(
			InvertSortDirection(selectedTab.GetShellBrowser()->GetGroupSortDirection()));
	}
	else
	{
		selectedTab.GetShellBrowser()->SetGroupMode(groupMode);
		selectedTab.GetShellBrowser()->SetShowInGroups(true);
	}
}

void Explorerplusplus::OnGroupByNone()
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->SetShowInGroups(false);
}

void Explorerplusplus::OnSortDirectionSelected(SortDirection direction)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->SetSortDirection(direction);
}

void Explorerplusplus::OnGroupSortDirectionSelected(SortDirection direction)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->SetGroupSortDirection(direction);
}

void Explorerplusplus::SaveAllSettings()
{
	m_iLastSelectedTab = GetActivePane()->GetTabContainer()->GetSelectedTabIndex();

	ILoadSave *pLoadSave = nullptr;

	if (m_bSavePreferencesToXMLFile)
		pLoadSave = new LoadSaveXML(this, FALSE);
	else
		pLoadSave = new LoadSaveRegistry(this);

	pLoadSave->SaveGenericSettings();
	pLoadSave->SaveTabs();
	pLoadSave->SaveDefaultColumns();
	pLoadSave->SaveBookmarks();
	pLoadSave->SaveApplicationToolbar();
	pLoadSave->SaveToolbarInformation();
	pLoadSave->SaveColorRules();
	pLoadSave->SaveDialogStates();

	delete pLoadSave;
}

const Config *Explorerplusplus::GetConfig() const
{
	return m_config.get();
}

HINSTANCE Explorerplusplus::GetResourceInstance() const
{
	return m_resourceInstance;
}

HACCEL *Explorerplusplus::GetAcceleratorTable() const
{
	return &g_hAccl;
}

HWND Explorerplusplus::GetMainWindow() const
{
	return m_hContainer;
}

ShellBrowser *Explorerplusplus::GetActiveShellBrowser() const
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

TabRestorer *Explorerplusplus::GetTabRestorer() const
{
	return m_tabRestorer.get();
}

HWND Explorerplusplus::GetTreeView() const
{
	return m_shellTreeView->GetHWND();
}

IDirectoryMonitor *Explorerplusplus::GetDirectoryMonitor() const
{
	return m_pDirMon;
}

IconResourceLoader *Explorerplusplus::GetIconResourceLoader() const
{
	return m_iconResourceLoader.get();
}

CachedIcons *Explorerplusplus::GetCachedIcons()
{
	return &m_cachedIcons;
}

BOOL Explorerplusplus::GetSavePreferencesToXmlFile() const
{
	return m_bSavePreferencesToXMLFile;
}

void Explorerplusplus::SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile)
{
	m_bSavePreferencesToXMLFile = savePreferencesToXmlFile;
}

void Explorerplusplus::OnShowHiddenFiles()
{
	Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	tab.GetShellBrowser()->SetShowHidden(!tab.GetShellBrowser()->GetShowHidden());
	tab.GetShellBrowser()->GetNavigationController()->Refresh();
}

void Explorerplusplus::FocusChanged()
{
	m_focusChangedSignal();
}

boost::signals2::connection Explorerplusplus::AddFocusChangeObserver(
	const FocusChangedSignal::slot_type &observer)
{
	if (m_applicationShuttingDown)
	{
		throw std::runtime_error("Adding a focus changed observer during shutdown is unsafe");
	}

	return m_focusChangedSignal.connect(observer);
}

void Explorerplusplus::FocusActiveTab()
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	SetFocus(selectedTab.GetShellBrowser()->GetListView());
}

bool Explorerplusplus::OnActivate(int activationState, bool minimized)
{
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
