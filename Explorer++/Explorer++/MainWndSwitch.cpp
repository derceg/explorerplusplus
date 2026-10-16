// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "AddressBarView.h"
#include "AppController.h"
#include "AppServices.h"
#include "ApplicationToolbar.h"
#include "ApplicationToolbarView.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/ManageBookmarksDialog.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "BrowserView.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "DrivesToolbar.h"
#include "DrivesToolbarView.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MainToolbarButtons.h"
#include "MenuRanges.h"
#include "ModelessDialogHelper.h"
#include "Plugins/PluginCommandManager.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/EnumHelper.h"
#include <windowsx.h>

static const int FOLDER_SIZE_LINE_INDEX = 1;

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

	case WM_MENUSELECT:
		MenuItemSelected(reinterpret_cast<HMENU>(lParam), LOWORD(wParam), HIWORD(wParam));
		break;

	case WM_TIMER:
		if (wParam == LISTVIEW_ITEM_CHANGED_TIMER_ID)
		{
			Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();

			UpdateDisplayWindow(selectedTab);
			m_mainToolbar->UpdateToolbarButtonStates();

			KillTimer(m_hwnd, LISTVIEW_ITEM_CHANGED_TIMER_ID);
		}
		break;

	case WM_USER_DISPLAYWINDOWRESIZED:
		OnDisplayWindowResized(wParam);
		break;

	case WM_APP_FOLDERSIZECOMPLETED:
	{
		DWFolderSizeCompletion *pDWFolderSizeCompletion = nullptr;
		TCHAR szSizeString[64];
		BOOL bValid = FALSE;

		pDWFolderSizeCompletion = (DWFolderSizeCompletion *) wParam;

		std::list<DWFolderSize>::iterator itr;

		/* First, make sure we should still display the
		results (we won't if the listview selection has
		changed, or this folder size was calculated for
		a tab other than the current one). */
		for (itr = m_DWFolderSizes.begin(); itr != m_DWFolderSizes.end(); itr++)
		{
			if (itr->uId == pDWFolderSizeCompletion->uId)
			{
				if (itr->iTabId == GetActivePane()->GetTabContainer()->GetSelectedTab().GetId())
				{
					bValid = itr->bValid;
				}

				m_DWFolderSizes.erase(itr);

				break;
			}
		}

		if (bValid)
		{
			auto displayFormat = m_config->globalFolderSettings.forceSize
				? m_config->globalFolderSettings.sizeDisplayFormat
				: SizeDisplayFormat::None;
			auto folderSizeText =
				FormatSizeString(pDWFolderSizeCompletion->liFolderSize.QuadPart, displayFormat);

			auto totalSizeText = m_resourceLoader->LoadString(IDS_GENERAL_TOTALSIZE);
			StringCchPrintf(szSizeString, std::size(szSizeString), _T("%s: %s"),
				totalSizeText.c_str(), folderSizeText.c_str());

			/* TODO: The line index should be stored in some other (variable) way. */
			DisplayWindow_SetLine(m_displayWindow->GetHWND(), FOLDER_SIZE_LINE_INDEX, szSizeString);
		}

		free(pDWFolderSizeCompletion);
	}
	break;

	case WM_APPCOMMAND:
		OnAppCommand(GET_APPCOMMAND_LPARAM(lParam));
		break;

	case WM_COMMAND:
		return CommandHandler(hwnd, reinterpret_cast<HWND>(lParam), LOWORD(wParam), HIWORD(wParam));

	case WM_NOTIFY:
		return NotifyHandler(hwnd, msg, wParam, lParam);

	case WM_SIZE:
		OnSize(static_cast<UINT>(wParam));
		return 0;

	case WM_DPICHANGED:
		OnDpiChanged(reinterpret_cast<RECT *>(lParam));
		return 0;

	case WM_CTLCOLORSTATIC:
		if (auto res =
				OnCtlColorStatic(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam)))
		{
			return *res;
		}
		break;

	// COM calls (such as IDropTarget::DragEnter) can result in a call being made to PeekMessage().
	// That method will then dispatch non-queued messages, with WM_CLOSE being one such message.
	// That's an issue, as it means if a WM_CLOSE message is in the message queue when a COM method
	// is called, the WM_CLOSE message could be processed, the main window destroyed and
	// Explorerplusplus instance deleted, all within the call to the COM method. Once the COM method
	// returns, the application isn't going to be in a valid state and will crash.
	// PeekMessage() won't, however, dispatch posted (i.e. queued) messages. So the message that's
	// posted here will only be processed in the normal message loop. If a COM modal loop is
	// running, the message won't be processed until that modal loop ends and the normal message
	// loop resumes.
	case WM_CLOSE:
		PostMessage(hwnd, WM_APP_CLOSE, 0, 0);
		return 0;

	case WM_APP_CLOSE:
		TryClose();
		break;

	case WM_ENDSESSION:
		if (wParam)
		{
			m_appServices->GetAppController()->NotifySessionEnding();
		}
		return 0;

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT Explorerplusplus::CommandHandler(HWND hwnd, HWND control, UINT id, UINT notificationCode)
{
	// Several toolbars will handle their own items.
	if (control
		&& ((m_drivesToolbar && control == m_drivesToolbar->GetView()->GetHWND())
			|| (m_applicationToolbar && control == m_applicationToolbar->GetView()->GetHWND())
			|| (m_bookmarksToolbar && control == m_bookmarksToolbar->GetView()->GetHWND())))
	{
		return 1;
	}

	if (control && notificationCode != 0)
	{
		return HandleControlNotification(hwnd, notificationCode);
	}
	else
	{
		return HandleMenuOrToolbarButtonOrAccelerator(hwnd, id, notificationCode);
	}
}

// It makes sense to handle menu items/toolbar buttons/accelerators together, since an individual
// command might be represented by all three of those.
LRESULT Explorerplusplus::HandleMenuOrToolbarButtonOrAccelerator(HWND hwnd, UINT id,
	UINT notificationCode)
{
	if (notificationCode == 1 && id >= ACCELERATOR_PLUGIN_START_ID
		&& id < ACCELERATOR_PLUGIN_END_ID)
	{
		m_appServices->GetPluginCommandManager()->onAcceleratorPressed(id);
		return 0;
	}

	switch (id)
	{
	case ToUnderlying(MainToolbarButton::NewTab):
	case IDM_FILE_NEWTAB:
		OnNewTab();
		break;

	case ToUnderlying(MainToolbarButton::CloseTab):
	case IDM_FILE_CLOSETAB:
		m_commandController.ExecuteCommand(IDM_FILE_CLOSETAB);
		break;

	case IDM_FILE_NEW_WINDOW:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_FILE_CLONEWINDOW:
		OnCloneWindow();
		break;

	case IDM_FILE_SAVEDIRECTORYLISTING:
		m_commandController.ExecuteCommand(id);
		break;

	case ToUnderlying(MainToolbarButton::OpenCommandPrompt):
	case IDM_FILE_OPENCOMMANDPROMPT:
		m_commandController.ExecuteCommand(IDM_FILE_OPENCOMMANDPROMPT);
		break;

	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		m_commandController.ExecuteCommand(IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR);
		break;

	case IDM_FILE_COPYFOLDERPATH:
	case IDM_FILE_COPYITEMPATH:
	case IDM_FILE_COPYUNIVERSALFILEPATHS:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_FILE_COPYCOLUMNTEXT:
		CopyColumnInfoToClipboard();
		break;

	case IDM_FILE_SETFILEATTRIBUTES:
		m_commandController.ExecuteCommand(id);
		break;

	case ToUnderlying(MainToolbarButton::Delete):
	case IDM_FILE_DELETE:
		m_commandController.ExecuteCommand(IDM_FILE_DELETE);
		break;

	case ToUnderlying(MainToolbarButton::DeletePermanently):
	case IDM_FILE_DELETEPERMANENTLY:
		m_commandController.ExecuteCommand(IDM_FILE_DELETEPERMANENTLY);
		break;

	case IDM_FILE_RENAME:
		m_commandController.ExecuteCommand(id);
		break;

	case ToUnderlying(MainToolbarButton::Properties):
	case IDM_FILE_PROPERTIES:
		m_commandController.ExecuteCommand(IDM_FILE_PROPERTIES);
		break;

	case IDM_FILE_EXIT:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_EDIT_UNDO:
		m_fileActionHandler.Undo();
		break;

	case ToUnderlying(MainToolbarButton::Cut):
	case IDM_EDIT_CUT:
		m_commandController.ExecuteCommand(IDM_EDIT_CUT);
		break;

	case ToUnderlying(MainToolbarButton::Copy):
	case IDM_EDIT_COPY:
		m_commandController.ExecuteCommand(IDM_EDIT_COPY);
		break;

	case ToUnderlying(MainToolbarButton::Paste):
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

	case ToUnderlying(MainToolbarButton::MoveTo):
	case IDM_EDIT_MOVETOFOLDER:
		m_commandController.ExecuteCommand(IDM_EDIT_MOVETOFOLDER);
		break;

	case ToUnderlying(MainToolbarButton::CopyTo):
	case IDM_EDIT_COPYTOFOLDER:
		m_commandController.ExecuteCommand(IDM_EDIT_COPYTOFOLDER);
		break;

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
		m_commandController.ExecuteCommand(id);
		break;

	case ToUnderlying(MainToolbarButton::Folders):
	case IDM_VIEW_FOLDERS:
		m_commandController.ExecuteCommand(IDM_VIEW_FOLDERS);
		break;

	case IDM_VIEW_DISPLAYWINDOW:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_VIEW_DECREASE_TEXT_SIZE:
	case IDM_VIEW_INCREASE_TEXT_SIZE:
	case IDA_RESET_TEXT_SIZE:
	case IDM_VIEW_CHANGEDISPLAYCOLOURS:
	case IDM_FILTER_FILTERRESULTS:
	case IDM_FILTER_ENABLE_FILTER:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_VIEW_AUTOARRANGE:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_VIEW_SHOWHIDDENFILES:
		OnShowHiddenFiles();
		break;

	case ToUnderlying(MainToolbarButton::Refresh):
	case IDM_VIEW_REFRESH:
		m_commandController.ExecuteCommand(IDM_VIEW_REFRESH);
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

	case ToUnderlying(MainToolbarButton::NewFolder):
	case IDM_ACTIONS_NEWFOLDER:
		m_commandController.ExecuteCommand(IDM_ACTIONS_NEWFOLDER);
		break;

	case ToUnderlying(MainToolbarButton::SplitFile):
	case IDM_ACTIONS_SPLITFILE:
		m_commandController.ExecuteCommand(IDM_ACTIONS_SPLITFILE);
		break;

	case ToUnderlying(MainToolbarButton::MergeFiles):
	case IDM_ACTIONS_MERGEFILES:
		m_commandController.ExecuteCommand(IDM_ACTIONS_MERGEFILES);
		break;

	case IDM_ACTIONS_DESTROYFILES:
		OnDestroyFiles();
		break;

	case ToUnderlying(MainToolbarButton::Back):
	case IDM_GO_BACK:
		m_commandController.ExecuteCommand(IDM_GO_BACK, DetermineOpenDisposition(false));
		break;

	case ToUnderlying(MainToolbarButton::Forward):
	case IDM_GO_FORWARD:
		m_commandController.ExecuteCommand(IDM_GO_FORWARD, DetermineOpenDisposition(false));
		break;

	case ToUnderlying(MainToolbarButton::Up):
	case IDM_GO_UP:
		m_commandController.ExecuteCommand(IDM_GO_UP, DetermineOpenDisposition(false));
		break;

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

	case ToUnderlying(MainToolbarButton::AddBookmark):
	case IDM_BOOKMARKS_BOOKMARK_THIS_TAB:
		BookmarkHelper::AddBookmarkItem(m_appServices->GetBookmarkTree(),
			BookmarkItem::Type::Bookmark, nullptr, std::nullopt, hwnd, this, m_acceleratorManager,
			m_resourceLoader, m_platformContext);
		break;

	case IDM_BOOKMARKS_BOOKMARK_ALL_TABS:
		m_commandController.ExecuteCommand(id);
		break;

	case ToUnderlying(MainToolbarButton::Bookmarks):
	case IDM_BOOKMARKS_MANAGE_BOOKMARKS:
		CreateOrSwitchToModelessDialog(m_appServices->GetModelessDialogList(),
			L"ManageBookmarksDialog",
			[this, hwnd]
			{
				return ManageBookmarksDialog::Create(m_resourceLoader, hwnd,
					m_appServices->GetBookmarkTree(), m_browserList, m_config, m_acceleratorManager,
					&m_iconFetcher, m_platformContext);
			});
		break;

	case ToUnderlying(MainToolbarButton::Search):
	case IDM_TOOLS_SEARCH:
		m_commandController.ExecuteCommand(IDM_TOOLS_SEARCH);
		break;

	case IDM_TOOLS_CUSTOMIZECOLORS:
	case IDM_TOOLS_RUNSCRIPT:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_TOOLS_OPTIONS:
		OnShowOptions();
		break;

	case IDM_WINDOW_SEARCH_TABS:
	case IDM_HELP_ONLINE_DOCUMENTATION:
	case IDM_HELP_CHECKFORUPDATES:
	case IDM_HELP_ABOUT:
	case IDA_SELECT_PREVIOUS_TAB:
	case IDA_SELECT_NEXT_TAB:
		m_commandController.ExecuteCommand(id);
		break;

	case IDA_ADDRESSBAR:
		if (m_config->showAddressBar.get())
		{
			SetFocus(m_addressBar->GetView()->GetHWND());
		}
		break;

	case IDA_COMBODROPDOWN:
		if (m_config->showAddressBar.get())
		{
			SetFocus(m_addressBar->GetView()->GetHWND());
			SendMessage(m_addressBar->GetView()->GetHWND(), CB_SHOWDROPDOWN, TRUE, 0);
		}
		break;

	case IDA_PREVIOUSWINDOW:
		m_view->FocusNextWindow(BrowserView::FocusChangeDirection::Previous);
		break;

	case IDA_NEXTWINDOW:
		m_view->FocusNextWindow(BrowserView::FocusChangeDirection::Next);
		break;

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
		m_commandController.ExecuteCommand(id);
		break;

	case ToUnderlying(MainToolbarButton::Views):
		OnToolbarViews();
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

/*
 * WM_NOTIFY handler for the main window.
 */
LRESULT CALLBACK Explorerplusplus::NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

	switch (nmhdr->code)
	{
	case LVN_KEYDOWN:
		return OnListViewKeyDown(lParam);

	case TBN_ENDADJUST:
		if (GetLifecycleState() == LifecycleState::Main)
		{
			OnRebarToolbarSizeUpdated(reinterpret_cast<NMHDR *>(lParam)->hwndFrom);
		}
		break;

	case RBN_HEIGHTCHANGE:
		// This message can be dispatched within the middle of an existing layout operation (if the
		// height of the rebar is updated). To avoid making re-entrant layout calls, the layout
		// update will be scheduled, instead of being immediately invoked.
		ScheduleUpdateLayout(m_weakPtrFactory.GetWeakPtr(), m_appServices->GetRuntime());
		break;

	case RBN_CHEVRONPUSHED:
		OnRebarChevronPushed(reinterpret_cast<NMREBARCHEVRON *>(lParam));
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
