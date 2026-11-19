// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "AddressBarView.h"
#include "AppServices.h"
#include "ApplicationToolbar.h"
#include "ApplicationToolbarView.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "Config.h"
#include "DrivesToolbar.h"
#include "DrivesToolbarView.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MenuRanges.h"
#include "Plugins/PluginCommandManager.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabBar.h"
#include "../Helper/Controls.h"
#include "../Helper/EnumHelper.h"
#include "../Helper/FileActionHandler.h"
#include <windowsx.h>

LRESULT Explorerplusplus::WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (OnActivate(LOWORD(wParam), HIWORD(wParam)))
		{
			return 0;
		}
		break;

	case WM_TIMER:
		if (wParam == LISTVIEW_ITEM_CHANGED_TIMER_ID)
		{
			Tab &selectedTab = GetActivePane()->GetTabBar()->GetSelectedTab();

			UpdateDisplayWindow(selectedTab);
			m_mainToolbar->UpdateToolbarButtonStates();

			KillTimer(m_hwnd, LISTVIEW_ITEM_CHANGED_TIMER_ID);
		}
		break;

	case WM_COMMAND:
		return CommandHandler(hwnd, reinterpret_cast<HWND>(lParam), LOWORD(wParam), HIWORD(wParam));

	case WM_NOTIFY:
		return NotifyHandler(hwnd, msg, wParam, lParam);

	case WM_SIZE:
		OnSize(static_cast<UINT>(wParam));
		return 0;

	case WM_NCDESTROY:
		delete this;
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT Explorerplusplus::CommandHandler(HWND hwnd, HWND control, UINT id, UINT notificationCode)
{
	// Several toolbars will handle their own items.
	if (control
		&& ((m_drivesToolbar && control == m_drivesToolbar->GetView()->GetHWND())
			|| (m_applicationToolbar && control == m_applicationToolbar->GetView()->GetHWND())
			|| (m_bookmarksToolbar && control == m_bookmarksToolbar->GetView()->GetHWND())
			|| (m_mainToolbar && control == m_mainToolbar->GetView()->GetHWND())))
	{
		return 1;
	}

	if (control && notificationCode != 0)
	{
		return HandleControlNotification(hwnd, notificationCode);
	}
	else
	{
		return HandleMenuOrToolbarButtonOrAccelerator(id, notificationCode);
	}
}

// It makes sense to handle menu items/toolbar buttons/accelerators together, since an individual
// command might be represented by all three of those.
LRESULT Explorerplusplus::HandleMenuOrToolbarButtonOrAccelerator(UINT id, UINT notificationCode)
{
	if (notificationCode == 1 && id >= ACCELERATOR_PLUGIN_START_ID
		&& id < ACCELERATOR_PLUGIN_END_ID)
	{
		m_appServices->GetPluginCommandManager()->onAcceleratorPressed(id);
		return 0;
	}

	switch (id)
	{
	case IDM_FILE_NEWTAB:
		OnNewTab();
		break;

	case IDM_FILE_CLOSETAB:
	case IDM_FILE_NEW_WINDOW:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_FILE_CLONEWINDOW:
		OnCloneWindow();
		break;

	case IDM_FILE_SAVEDIRECTORYLISTING:
	case IDM_FILE_OPENCOMMANDPROMPT:
	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
	case IDM_FILE_COPYFOLDERPATH:
	case IDM_FILE_COPYITEMPATH:
	case IDM_FILE_COPYUNIVERSALFILEPATHS:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_FILE_COPYCOLUMNTEXT:
		CopyColumnInfoToClipboard();
		break;

	case IDM_FILE_SETFILEATTRIBUTES:
	case IDM_FILE_DELETE:
	case IDM_FILE_DELETEPERMANENTLY:
	case IDM_FILE_RENAME:
	case IDM_FILE_PROPERTIES:
	case IDM_FILE_EXIT:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_EDIT_UNDO:
		m_appServices->GetFileActionHandler()->Undo();
		break;

	case IDM_EDIT_CUT:
	case IDM_EDIT_COPY:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_EDIT_PASTE:
		OnPaste();
		break;

	case IDM_EDIT_PASTESHORTCUT:
		OnPasteShortcut();
		break;

	case IDM_EDIT_PASTEHARDLINK:
		GetActiveShellBrowserImpl()->PasteHardLinks();
		break;

	case IDM_EDIT_PASTE_SYMBOLIC_LINK:
		GetActiveShellBrowserImpl()->PasteSymLinks();
		break;

	case IDM_EDIT_MOVETOFOLDER:
	case IDM_EDIT_COPYTOFOLDER:
	case IDM_EDIT_SELECTALL:
	case IDM_EDIT_INVERTSELECTION:
	case IDM_EDIT_SELECTNONE:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_EDIT_SELECTALLOFSAMETYPE:
		HighlightSimilarFiles(m_hActiveListView);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_WILDCARDSELECTION:
	case IDM_EDIT_WILDCARDDESELECT:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_EDIT_RESOLVELINK:
		OnResolveLink();
		break;

	case IDM_VIEW_DUAL_PANE:
		m_config->dualPane = !m_config->dualPane;
		break;

	case IDM_VIEW_STATUSBAR:
	case IDM_VIEW_FOLDERS:
	case IDM_VIEW_DISPLAYWINDOW:
	case IDM_VIEW_DECREASE_TEXT_SIZE:
	case IDM_VIEW_INCREASE_TEXT_SIZE:
	case IDA_RESET_TEXT_SIZE:
	case IDM_VIEW_CHANGEDISPLAYCOLOURS:
	case IDM_FILTER_FILTERRESULTS:
	case IDM_FILTER_ENABLE_FILTER:
	case IDM_VIEW_AUTOARRANGE:
	case IDM_VIEW_SHOWHIDDENFILES:
	case IDM_VIEW_REFRESH:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_SORTBY_MORE:
	case IDM_VIEW_SELECTCOLUMNS:
		OnSelectColumns();
		break;

	case IDM_VIEW_AUTOSIZECOLUMNS:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT:
	{
		/* Dump the columns from the current tab, and save
		them as the default columns for the appropriate folder
		type.. */
		const auto &currentColumns = m_pActiveShellBrowser->GetCurrentColumnSet();
		const auto &pidl = m_pActiveShellBrowser->GetDirectory();

		unique_pidl_absolute pidlDrives;
		SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, wil::out_param(pidlDrives));

		unique_pidl_absolute pidlControls;
		SHGetFolderLocation(nullptr, CSIDL_CONTROLS, nullptr, 0, wil::out_param(pidlControls));

		unique_pidl_absolute pidlBitBucket;
		SHGetFolderLocation(nullptr, CSIDL_BITBUCKET, nullptr, 0, wil::out_param(pidlBitBucket));

		unique_pidl_absolute pidlPrinters;
		SHGetFolderLocation(nullptr, CSIDL_PRINTERS, nullptr, 0, wil::out_param(pidlPrinters));

		unique_pidl_absolute pidlConnections;
		SHGetFolderLocation(nullptr, CSIDL_CONNECTIONS, nullptr, 0,
			wil::out_param(pidlConnections));

		unique_pidl_absolute pidlNetwork;
		SHGetFolderLocation(nullptr, CSIDL_NETWORK, nullptr, 0, wil::out_param(pidlNetwork));

		IShellFolder *pShellFolder;
		SHGetDesktopFolder(&pShellFolder);

		if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.Raw(), pidlDrives.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.myComputerColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.Raw(), pidlControls.get())
			== 0)
		{
			m_config->globalFolderSettings.folderColumns.controlPanelColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.Raw(), pidlBitBucket.get())
			== 0)
		{
			m_config->globalFolderSettings.folderColumns.recycleBinColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.Raw(), pidlPrinters.get())
			== 0)
		{
			m_config->globalFolderSettings.folderColumns.printersColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.Raw(), pidlConnections.get())
			== 0)
		{
			m_config->globalFolderSettings.folderColumns.networkConnectionsColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.Raw(), pidlNetwork.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.myNetworkPlacesColumns = currentColumns;
		}
		else
		{
			m_config->globalFolderSettings.folderColumns.realFolderColumns = currentColumns;
		}

		pShellFolder->Release();
	}
	break;

	case IDM_ACTIONS_NEWFOLDER:
	case IDM_ACTIONS_SPLITFILE:
	case IDM_ACTIONS_MERGEFILES:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_ACTIONS_DESTROYFILES:
		OnDestroyFiles();
		break;

	case IDM_GO_BACK:
	case IDM_GO_FORWARD:
	case IDM_GO_UP:
	case IDM_GO_QUICK_ACCESS:
	case IDM_GO_COMPUTER:
	case IDM_GO_DOCUMENTS:
	case IDM_GO_DOWNLOADS:
	case IDM_GO_MUSIC:
	case IDM_GO_PICTURES:
	case IDM_GO_VIDEOS:
	case IDM_GO_DESKTOP:
	case IDM_GO_RECYCLE_BIN:
	case IDM_GO_CONTROL_PANEL:
	case IDM_GO_PRINTERS:
	case IDM_GO_NETWORK:
	case IDM_GO_WSL_DISTRIBUTIONS:
		m_commandController.ExecuteCommand(id, DetermineOpenDisposition(false));
		break;

	case IDM_BOOKMARKS_BOOKMARK_THIS_TAB:
	case IDM_BOOKMARKS_BOOKMARK_ALL_TABS:
	case IDM_BOOKMARKS_MANAGE_BOOKMARKS:
	case IDM_TOOLS_SEARCH:
	case IDM_TOOLS_CUSTOMIZECOLORS:
	case IDM_TOOLS_RUNSCRIPT:
	case IDM_TOOLS_OPTIONS:
	case IDM_WINDOW_SEARCH_TABS:
	case IDM_HELP_ONLINE_DOCUMENTATION:
	case IDM_HELP_CHECKFORUPDATES:
	case IDM_HELP_ABOUT:
	case IDA_SELECT_PREVIOUS_TAB:
	case IDA_SELECT_NEXT_TAB:
	case IDA_ADDRESSBAR:
	case IDA_COMBODROPDOWN:
	case IDA_PREVIOUSWINDOW:
	case IDA_NEXTWINDOW:
	case IDA_DUPLICATE_TAB:
	case IDA_HOME:
	case IDA_SELECT_TAB_1:
	case IDA_SELECT_TAB_2:
	case IDA_SELECT_TAB_3:
	case IDA_SELECT_TAB_4:
	case IDA_SELECT_TAB_5:
	case IDA_SELECT_TAB_6:
	case IDA_SELECT_TAB_7:
	case IDA_SELECT_TAB_8:
	case IDA_SELECT_LAST_TAB:
	case IDA_RESTORE_LAST_TAB:
	case IDM_CYCLE_VIEW:
		m_commandController.ExecuteCommand(id);
		break;
	}

	return 1;
}

LRESULT Explorerplusplus::HandleControlNotification(HWND hwnd, UINT notificationCode)
{
	UNREFERENCED_PARAMETER(hwnd);

	switch (notificationCode)
	{
	case CBN_DROPDOWN:
		AddPathsToComboBoxEx(m_addressBar->GetView()->GetHWND(),
			m_pActiveShellBrowser->GetDirectoryPath().c_str());
		break;
	}

	return 1;
}

LRESULT CALLBACK Explorerplusplus::NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

	switch (nmhdr->code)
	{
	case LVN_KEYDOWN:
		return OnListViewKeyDown(lParam);
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
