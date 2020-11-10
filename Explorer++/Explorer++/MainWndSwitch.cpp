// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "ApplicationToolbar.h"
#include "Bookmarks/UI/AddBookmarkDialog.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/ManageBookmarksDialog.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "DrivesToolbar.h"
#include "Explorer++_internal.h"
#include "HolderWindow.h"
#include "IModelessDialogNotification.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MenuRanges.h"
#include "ModelessDialogs.h"
#include "Navigation.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabBacking.h"
#include "TabContainer.h"
#include "TabRestorerUI.h"
#include "ToolbarButtons.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"

static const int FOLDER_SIZE_LINE_INDEX = 1;

/* Defines the distance between the cursor
and the right edge of the treeview during
a resizing operation. */
static const int TREEVIEW_DRAG_OFFSET = 8;

LRESULT CALLBACK Explorerplusplus::WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	auto *pContainer = (Explorerplusplus *)GetWindowLongPtr(hwnd,GWLP_USERDATA);

	switch(Msg)
	{
		case WM_NCCREATE:
			/* Create a new Explorerplusplus object to assign to this window. */
			pContainer = new Explorerplusplus(hwnd);

			if(!pContainer)
			{
				PostQuitMessage(0);
				return 0;
			}

			/* Store the Explorerplusplus object pointer into the extra window bytes
			for this window. */
			SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)pContainer);
			break;

		case WM_NCDESTROY:
			SetWindowLongPtr(hwnd,GWLP_USERDATA,0);
			delete pContainer;
			return 0;
	}

	/* Jump across to the member window function (will handle all requests). */
	if (pContainer != nullptr)
	{
		return pContainer->WindowProcedure(hwnd, Msg, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hwnd, Msg, wParam, lParam);
	}
}

LRESULT CALLBACK Explorerplusplus::WindowProcedure(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_CREATE:
		OnCreate();
		break;

	case WM_SETFOCUS:
		OnSetFocus();
		return 0;

	case WM_INITMENU:
		SetProgramMenuItemStates(reinterpret_cast<HMENU>(wParam));

		if (reinterpret_cast<HMENU>(wParam) == GetMenu(m_hContainer))
		{
			m_mainMenuPreShowSignal(reinterpret_cast<HMENU>(wParam));
		}
		break;

	case WM_MENUSELECT:
		StatusBarMenuSelect(wParam,lParam);
		break;

	case WM_DEVICECHANGE:
		OnDeviceChange(wParam,lParam);
		break;

	case WM_TIMER:
		if (wParam == AUTOSAVE_TIMER_ID)
		{
			SaveAllSettings();
		}
		else if (wParam == LISTVIEW_ITEM_CHANGED_TIMER_ID)
		{
			Tab &selectedTab = m_tabContainer->GetSelectedTab();

			UpdateDisplayWindow(selectedTab);
			UpdateStatusBarText(selectedTab);
			m_mainToolbar->UpdateToolbarButtonStates();

			KillTimer(m_hContainer, LISTVIEW_ITEM_CHANGED_TIMER_ID);
		}
		break;

	case WM_USER_UPDATEWINDOWS:
		UpdateWindowStates(m_tabContainer->GetSelectedTab());
		break;

	case WM_USER_FILESADDED:
	{
		Tab *tab = m_tabContainer->GetTabOptional(static_cast<int>(wParam));

		if (tab)
		{
			tab->GetShellBrowser()->DirectoryAltered();
		}
	}
		break;

	case WM_USER_DISPLAYWINDOWRESIZED:
		OnDisplayWindowResized(wParam);
		break;

	case WM_USER_STARTEDBROWSING:
		OnStartedBrowsing((int)wParam,(TCHAR *)lParam);
		break;

	case WM_USER_NEWITEMINSERTED:
		OnShellNewItemCreated(lParam);
		break;

	case WM_USER_DIRECTORYMODIFIED:
		OnDirectoryModified((int)wParam);
		break;

	// See https://github.com/derceg/explorerplusplus/issues/169.
	/*case WM_APP_ASSOCCHANGED:
		OnAssocChanged();
		break;*/

	case WM_USER_HOLDERRESIZED:
		{
			RECT	rc;

			m_config->treeViewWidth = (int)lParam + TREEVIEW_DRAG_OFFSET;

			GetClientRect(m_hContainer,&rc);

			SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
		}
		break;

	case WM_APP_FOLDERSIZECOMPLETED:
		{
			DWFolderSizeCompletion *pDWFolderSizeCompletion = nullptr;
			TCHAR szFolderSize[32];
			TCHAR szSizeString[64];
			TCHAR szTotalSize[64];
			BOOL bValid = FALSE;

			pDWFolderSizeCompletion = (DWFolderSizeCompletion *)wParam;

			std::list<DWFolderSize>::iterator itr;

			/* First, make sure we should still display the
			results (we won't if the listview selection has
			changed, or this folder size was calculated for
			a tab other than the current one). */
			for(itr = m_DWFolderSizes.begin();itr != m_DWFolderSizes.end();itr++)
			{
				if(itr->uId == pDWFolderSizeCompletion->uId)
				{
					if(itr->iTabId == m_tabContainer->GetSelectedTab().GetId())
					{
						bValid = itr->bValid;
					}

					m_DWFolderSizes.erase(itr);

					break;
				}
			}

			if(bValid)
			{
				FormatSizeString(pDWFolderSizeCompletion->liFolderSize,szFolderSize,
					SIZEOF_ARRAY(szFolderSize),m_config->globalFolderSettings.forceSize,
					m_config->globalFolderSettings.sizeDisplayFormat);

				LoadString(m_hLanguageModule,IDS_GENERAL_TOTALSIZE,
					szTotalSize,SIZEOF_ARRAY(szTotalSize));

				StringCchPrintf(szSizeString,SIZEOF_ARRAY(szSizeString),
					_T("%s: %s"),szTotalSize,szFolderSize);

				/* TODO: The line index should be stored in some other (variable) way. */
				DisplayWindow_SetLine(m_hDisplayWindow,FOLDER_SIZE_LINE_INDEX,szSizeString);
			}

			free(pDWFolderSizeCompletion);
		}
		break;

	case WM_COPYDATA:
		{
			auto *pcds = reinterpret_cast<COPYDATASTRUCT *>(lParam);

			if (pcds->lpData != nullptr)
			{
				m_tabContainer->CreateNewTab((TCHAR *)pcds->lpData, TabSettings(_selected = true));
			}
			else
			{
				m_tabContainer->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
			}

			return TRUE;
		}

	case WM_NDW_RCLICK:
		{
			POINT pt;
			POINTSTOPOINT(pt, MAKEPOINTS(lParam));
			OnDisplayWindowRClick(&pt);
		}
		break;

	case WM_NDW_ICONRCLICK:
		{
			POINT pt;
			POINTSTOPOINT(pt, MAKEPOINTS(lParam));
			OnDisplayWindowIconRClick(&pt);
		}
		break;

	case WM_APPCOMMAND:
		OnAppCommand(GET_APPCOMMAND_LPARAM(lParam));
		break;

	case WM_COMMAND:
		return CommandHandler(hwnd,wParam);

	case WM_NOTIFY:
		return NotifyHandler(hwnd, Msg, wParam, lParam);

	case WM_SIZE:
		return OnSize(LOWORD(lParam),HIWORD(lParam));

	case WM_DPICHANGED:
		OnDpiChanged(reinterpret_cast<RECT *>(lParam));
		return 0;

	case WM_CTLCOLORSTATIC:
		if (auto res = OnCtlColorStatic(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam)))
		{
			return *res;
		}
		break;

	case WM_CLOSE:
		return OnClose();

	case WM_DESTROY:
		return OnDestroy();
	}

	return DefWindowProc(hwnd,Msg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::CommandHandler(HWND hwnd,WPARAM wParam)
{
	if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
	{
		return HandleMenuOrAccelerator(hwnd, wParam);
	}
	else
	{
		return HandleControlNotification(hwnd, wParam);
	}
}

LRESULT Explorerplusplus::HandleMenuOrAccelerator(HWND hwnd, WPARAM wParam)
{
	if (HIWORD(wParam) == 0 && LOWORD(wParam) >= MENU_BOOKMARK_STARTID &&
		LOWORD(wParam) <= MENU_BOOKMARK_ENDID)
	{
		m_bookmarksMainMenu->OnMenuItemClicked(LOWORD(wParam));
		return 0;
	}
	else if (HIWORD(wParam) == 0 && LOWORD(wParam) >= MENU_RECENT_TABS_STARTID &&
		LOWORD(wParam) < MENU_RECENT_TABS_ENDID)
	{
		m_tabRestorerUI->OnMenuItemClicked(LOWORD(wParam));
		return 0;
	}
	else if (HIWORD(wParam) == 0 && LOWORD(wParam) >= MENU_PLUGIN_STARTID &&
		LOWORD(wParam) < MENU_PLUGIN_ENDID)
	{
		m_pluginMenuManager.OnMenuItemClicked(LOWORD(wParam));
		return 0;
	}
	else if (HIWORD(wParam) == 1 && LOWORD(wParam) >= ACCELERATOR_PLUGIN_STARTID &&
		LOWORD(wParam) < ACCELERATOR_PLUGIN_ENDID)
	{
		m_pluginCommandManager.onAcceleratorPressed(LOWORD(wParam));
		return 0;
	}

	switch (LOWORD(wParam))
	{
	case ToolbarButton::NewTab:
	case IDM_FILE_NEWTAB:
		OnNewTab();
		break;

	case ToolbarButton::CloseTab:
	case TABTOOLBAR_CLOSE:
	case IDM_FILE_CLOSETAB:
		OnCloseTab();
		break;

	case IDM_FILE_CLONEWINDOW:
		OnCloneWindow();
		break;

	case IDM_FILE_SAVEDIRECTORYLISTING:
		OnSaveDirectoryListing();
		break;

	case ToolbarButton::OpenCommandPrompt:
	case IDM_FILE_OPENCOMMANDPROMPT:
		StartCommandPrompt(m_CurrentDirectory, false);
		break;

	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		StartCommandPrompt(m_CurrentDirectory, true);
		break;

	case IDM_FILE_COPYFOLDERPATH:
	{
		BulkClipboardWriter clipboardWriter;
		clipboardWriter.WriteText(m_CurrentDirectory);
	}
		break;

	case IDM_FILE_COPYITEMPATH:
		OnCopyItemPath();
		break;

	case IDM_FILE_COPYUNIVERSALFILEPATHS:
		OnCopyUniversalPaths();
		break;

	case IDM_FILE_COPYCOLUMNTEXT:
		CopyColumnInfoToClipboard();
		break;

	case IDM_FILE_SETFILEATTRIBUTES:
		OnSetFileAttributes();
		break;

	case ToolbarButton::Delete:
	case IDM_FILE_DELETE:
		OnFileDelete(false);
		break;

	case ToolbarButton::DeletePermanently:
	case IDM_FILE_DELETEPERMANENTLY:
		OnFileDelete(true);
		break;

	case IDM_FILE_RENAME:
		OnFileRename();
		break;

	case ToolbarButton::Properties:
	case IDM_FILE_PROPERTIES:
	case IDM_RCLICK_PROPERTIES:
		OnShowFileProperties();
		break;

	case IDM_FILE_EXIT:
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		break;

	case IDM_EDIT_UNDO:
		m_FileActionHandler.Undo();
		break;

	case ToolbarButton::Copy:
	case IDM_EDIT_COPY:
		OnCopy(TRUE);
		break;

	case ToolbarButton::Cut:
	case IDM_EDIT_CUT:
		OnCopy(FALSE);
		break;

	case ToolbarButton::Paste:
	case IDM_EDIT_PASTE:
		OnPaste();
		break;

	case IDM_EDIT_PASTESHORTCUT:
		PasteLinksToClipboardFiles(m_CurrentDirectory.c_str());
		break;

	case IDM_EDIT_PASTEHARDLINK:
		PasteHardLinks(m_CurrentDirectory.c_str());
		break;

	case IDM_EDIT_COPYTOFOLDER:
	case ToolbarButton::CopyTo:
		CopyToFolder(false);
		break;

	case ToolbarButton::MoveTo:
	case IDM_EDIT_MOVETOFOLDER:
		CopyToFolder(true);
		break;

	case IDM_EDIT_SELECTALL:
		ListViewHelper::SelectAllItems(m_hActiveListView, TRUE);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_INVERTSELECTION:
		ListViewHelper::InvertSelection(m_hActiveListView);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_SELECTALLOFSAMETYPE:
		HighlightSimilarFiles(m_hActiveListView);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_SELECTNONE:
		ListViewHelper::SelectAllItems(m_hActiveListView, FALSE);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_WILDCARDSELECTION:
		OnWildcardSelect(TRUE);
		break;

	case IDM_EDIT_WILDCARDDESELECT:
		OnWildcardSelect(FALSE);
		break;

	case IDM_EDIT_RESOLVELINK:
		OnResolveLink();
		break;

	case IDM_VIEW_STATUSBAR:
		m_config->showStatusBar = !m_config->showStatusBar;
		lShowWindow(m_hStatusBar, m_config->showStatusBar);
		ResizeWindows();
		break;

	case ToolbarButton::Folders:
	case IDM_VIEW_FOLDERS:
		ToggleFolders();
		break;

	case IDM_VIEW_DISPLAYWINDOW:
		m_config->showDisplayWindow = !m_config->showDisplayWindow;
		lShowWindow(m_hDisplayWindow, m_config->showDisplayWindow);
		ResizeWindows();
		break;

	case IDM_DISPLAYWINDOW_VERTICAL:
		m_config->displayWindowVertical = !m_config->displayWindowVertical;
		ApplyDisplayWindowPosition();
		ResizeWindows();
		break;

	case IDM_TOOLBARS_ADDRESSBAR:
		m_config->showAddressBar = !m_config->showAddressBar;
		ShowMainRebarBand(m_addressBar->GetHWND(), m_config->showAddressBar);
		AdjustFolderPanePosition();
		ResizeWindows();
		break;

	case IDM_TOOLBARS_MAINTOOLBAR:
		m_config->showMainToolbar = !m_config->showMainToolbar;
		ShowMainRebarBand(m_mainToolbar->GetHWND(), m_config->showMainToolbar);
		AdjustFolderPanePosition();
		ResizeWindows();
		break;

	case IDM_TOOLBARS_BOOKMARKSTOOLBAR:
		m_config->showBookmarksToolbar = !m_config->showBookmarksToolbar;
		ShowMainRebarBand(m_hBookmarksToolbar, m_config->showBookmarksToolbar);
		AdjustFolderPanePosition();
		ResizeWindows();
		break;

	case IDM_TOOLBARS_DRIVES:
		m_config->showDrivesToolbar = !m_config->showDrivesToolbar;
		ShowMainRebarBand(m_pDrivesToolbar->GetHWND(), m_config->showDrivesToolbar);
		AdjustFolderPanePosition();
		ResizeWindows();
		break;

	case IDM_TOOLBARS_APPLICATIONTOOLBAR:
		m_config->showApplicationToolbar = !m_config->showApplicationToolbar;
		ShowMainRebarBand(m_pApplicationToolbar->GetHWND(), m_config->showApplicationToolbar);
		AdjustFolderPanePosition();
		ResizeWindows();
		break;

	case IDM_TOOLBARS_LOCKTOOLBARS:
		OnLockToolbars();
		break;

	case IDM_TOOLBARS_CUSTOMIZE:
		SendMessage(m_mainToolbar->GetHWND(), TB_CUSTOMIZE, 0, 0);
		break;

	case IDM_VIEW_EXTRALARGEICONS:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::ExtraLargeIcons);
		break;

	case IDM_VIEW_LARGEICONS:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::LargeIcons);
		break;

	case IDM_VIEW_ICONS:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::Icons);
		break;

	case IDM_VIEW_SMALLICONS:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::SmallIcons);
		break;

	case IDM_VIEW_LIST:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::List);
		break;

	case IDM_VIEW_DETAILS:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::Details);
		break;

	case IDM_VIEW_THUMBNAILS:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::Thumbnails);
		break;

	case IDM_VIEW_TILES:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetViewMode(ViewMode::Tiles);
		break;

	case IDM_VIEW_CHANGEDISPLAYCOLOURS:
		OnChangeDisplayColors();
		break;

	case IDM_FILTER_FILTERRESULTS:
		OnFilterResults();
		break;

	case IDM_FILTER_APPLYFILTER:
		m_pActiveShellBrowser->SetFilterStatus(!m_pActiveShellBrowser->GetFilterStatus());
		break;

	case IDM_SORTBY_NAME:
		OnSortBy(SortMode::Name);
		break;

	case IDM_SORTBY_SIZE:
		OnSortBy(SortMode::Size);
		break;

	case IDM_SORTBY_TYPE:
		OnSortBy(SortMode::Type);
		break;

	case IDM_SORTBY_DATEMODIFIED:
		OnSortBy(SortMode::DateModified);
		break;

	case IDM_SORTBY_TOTALSIZE:
		OnSortBy(SortMode::TotalSize);
		break;

	case IDM_SORTBY_FREESPACE:
		OnSortBy(SortMode::FreeSpace);
		break;

	case IDM_SORTBY_DATEDELETED:
		OnSortBy(SortMode::DateDeleted);
		break;

	case IDM_SORTBY_ORIGINALLOCATION:
		OnSortBy(SortMode::OriginalLocation);
		break;

	case IDM_SORTBY_ATTRIBUTES:
		OnSortBy(SortMode::Attributes);
		break;

	case IDM_SORTBY_REALSIZE:
		OnSortBy(SortMode::RealSize);
		break;

	case IDM_SORTBY_SHORTNAME:
		OnSortBy(SortMode::ShortName);
		break;

	case IDM_SORTBY_OWNER:
		OnSortBy(SortMode::Owner);
		break;

	case IDM_SORTBY_PRODUCTNAME:
		OnSortBy(SortMode::ProductName);
		break;

	case IDM_SORTBY_COMPANY:
		OnSortBy(SortMode::Company);
		break;

	case IDM_SORTBY_DESCRIPTION:
		OnSortBy(SortMode::Description);
		break;

	case IDM_SORTBY_FILEVERSION:
		OnSortBy(SortMode::FileVersion);
		break;

	case IDM_SORTBY_PRODUCTVERSION:
		OnSortBy(SortMode::ProductVersion);
		break;

	case IDM_SORTBY_SHORTCUTTO:
		OnSortBy(SortMode::ShortcutTo);
		break;

	case IDM_SORTBY_HARDLINKS:
		OnSortBy(SortMode::HardLinks);
		break;

	case IDM_SORTBY_EXTENSION:
		OnSortBy(SortMode::Extension);
		break;

	case IDM_SORTBY_CREATED:
		OnSortBy(SortMode::Created);
		break;

	case IDM_SORTBY_ACCESSED:
		OnSortBy(SortMode::Accessed);
		break;

	case IDM_SORTBY_TITLE:
		OnSortBy(SortMode::Title);
		break;

	case IDM_SORTBY_SUBJECT:
		OnSortBy(SortMode::Subject);
		break;

	case IDM_SORTBY_AUTHOR:
		OnSortBy(SortMode::Authors);
		break;

	case IDM_SORTBY_KEYWORDS:
		OnSortBy(SortMode::Keywords);
		break;

	case IDM_SORTBY_COMMENTS:
		OnSortBy(SortMode::Comments);
		break;

	case IDM_SORTBY_CAMERAMODEL:
		OnSortBy(SortMode::CameraModel);
		break;

	case IDM_SORTBY_DATETAKEN:
		OnSortBy(SortMode::DateTaken);
		break;

	case IDM_SORTBY_WIDTH:
		OnSortBy(SortMode::Width);
		break;

	case IDM_SORTBY_HEIGHT:
		OnSortBy(SortMode::Height);
		break;

	case IDM_SORTBY_VIRTUALCOMMENTS:
		OnSortBy(SortMode::VirtualComments);
		break;

	case IDM_SORTBY_FILESYSTEM:
		OnSortBy(SortMode::FileSystem);
		break;

	case IDM_SORTBY_NUMPRINTERDOCUMENTS:
		OnSortBy(SortMode::NumPrinterDocuments);
		break;

	case IDM_SORTBY_PRINTERSTATUS:
		OnSortBy(SortMode::PrinterStatus);
		break;

	case IDM_SORTBY_PRINTERCOMMENTS:
		OnSortBy(SortMode::PrinterComments);
		break;

	case IDM_SORTBY_PRINTERLOCATION:
		OnSortBy(SortMode::PrinterLocation);
		break;

	case IDM_SORTBY_NETWORKADAPTER_STATUS:
		OnSortBy(SortMode::NetworkAdapterStatus);
		break;

	case IDM_SORTBY_MEDIA_BITRATE:
		OnSortBy(SortMode::MediaBitrate);
		break;

	case IDM_SORTBY_MEDIA_COPYRIGHT:
		OnSortBy(SortMode::MediaCopyright);
		break;

	case IDM_SORTBY_MEDIA_DURATION:
		OnSortBy(SortMode::MediaDuration);
		break;

	case IDM_SORTBY_MEDIA_PROTECTED:
		OnSortBy(SortMode::MediaProtected);
		break;

	case IDM_SORTBY_MEDIA_RATING:
		OnSortBy(SortMode::MediaRating);
		break;

	case IDM_SORTBY_MEDIA_ALBUM_ARTIST:
		OnSortBy(SortMode::MediaAlbumArtist);
		break;

	case IDM_SORTBY_MEDIA_ALBUM:
		OnSortBy(SortMode::MediaAlbum);
		break;

	case IDM_SORTBY_MEDIA_BEATS_PER_MINUTE:
		OnSortBy(SortMode::MediaBeatsPerMinute);
		break;

	case IDM_SORTBY_MEDIA_COMPOSER:
		OnSortBy(SortMode::MediaComposer);
		break;

	case IDM_SORTBY_MEDIA_CONDUCTOR:
		OnSortBy(SortMode::MediaConductor);
		break;

	case IDM_SORTBY_MEDIA_DIRECTOR:
		OnSortBy(SortMode::MediaDirector);
		break;

	case IDM_SORTBY_MEDIA_GENRE:
		OnSortBy(SortMode::MediaGenre);
		break;

	case IDM_SORTBY_MEDIA_LANGUAGE:
		OnSortBy(SortMode::MediaLanguage);
		break;

	case IDM_SORTBY_MEDIA_BROADCAST_DATE:
		OnSortBy(SortMode::MediaBroadcastDate);
		break;

	case IDM_SORTBY_MEDIA_CHANNEL:
		OnSortBy(SortMode::MediaChannel);
		break;

	case IDM_SORTBY_MEDIA_STATION_NAME:
		OnSortBy(SortMode::MediaStationName);
		break;

	case IDM_SORTBY_MEDIA_MOOD:
		OnSortBy(SortMode::MediaMood);
		break;

	case IDM_SORTBY_MEDIA_PARENTAL_RATING:
		OnSortBy(SortMode::MediaParentalRating);
		break;

	case IDM_SORTBY_MEDIA_PARENTAL_RATNG_REASON:
		OnSortBy(SortMode::MediaParentalRatingReason);
		break;

	case IDM_SORTBY_MEDIA_PERIOD:
		OnSortBy(SortMode::MediaPeriod);
		break;

	case IDM_SORTBY_MEDIA_PRODUCER:
		OnSortBy(SortMode::MediaProducer);
		break;

	case IDM_SORTBY_MEDIA_PUBLISHER:
		OnSortBy(SortMode::MediaPublisher);
		break;

	case IDM_SORTBY_MEDIA_WRITER:
		OnSortBy(SortMode::MediaWriter);
		break;

	case IDM_SORTBY_MEDIA_YEAR:
		OnSortBy(SortMode::MediaYear);
		break;

	case IDM_GROUPBY_NAME:
		OnGroupBy(SortMode::Name);
		break;

	case IDM_GROUPBY_SIZE:
		OnGroupBy(SortMode::Size);
		break;

	case IDM_GROUPBY_TYPE:
		OnGroupBy(SortMode::Type);
		break;

	case IDM_GROUPBY_DATEMODIFIED:
		OnGroupBy(SortMode::DateModified);
		break;

	case IDM_GROUPBY_TOTALSIZE:
		OnGroupBy(SortMode::TotalSize);
		break;

	case IDM_GROUPBY_FREESPACE:
		OnGroupBy(SortMode::FreeSpace);
		break;

	case IDM_GROUPBY_DATEDELETED:
		OnGroupBy(SortMode::DateDeleted);
		break;

	case IDM_GROUPBY_ORIGINALLOCATION:
		OnGroupBy(SortMode::OriginalLocation);
		break;

	case IDM_GROUPBY_ATTRIBUTES:
		OnGroupBy(SortMode::Attributes);
		break;

	case IDM_GROUPBY_REALSIZE:
		OnGroupBy(SortMode::RealSize);
		break;

	case IDM_GROUPBY_SHORTNAME:
		OnGroupBy(SortMode::ShortName);
		break;

	case IDM_GROUPBY_OWNER:
		OnGroupBy(SortMode::Owner);
		break;

	case IDM_GROUPBY_PRODUCTNAME:
		OnGroupBy(SortMode::ProductName);
		break;

	case IDM_GROUPBY_COMPANY:
		OnGroupBy(SortMode::Company);
		break;

	case IDM_GROUPBY_DESCRIPTION:
		OnGroupBy(SortMode::Description);
		break;

	case IDM_GROUPBY_FILEVERSION:
		OnGroupBy(SortMode::FileVersion);
		break;

	case IDM_GROUPBY_PRODUCTVERSION:
		OnGroupBy(SortMode::ProductVersion);
		break;

	case IDM_GROUPBY_SHORTCUTTO:
		OnGroupBy(SortMode::ShortcutTo);
		break;

	case IDM_GROUPBY_HARDLINKS:
		OnGroupBy(SortMode::HardLinks);
		break;

	case IDM_GROUPBY_EXTENSION:
		OnGroupBy(SortMode::Extension);
		break;

	case IDM_GROUPBY_CREATED:
		OnGroupBy(SortMode::Created);
		break;

	case IDM_GROUPBY_ACCESSED:
		OnGroupBy(SortMode::Accessed);
		break;

	case IDM_GROUPBY_TITLE:
		OnGroupBy(SortMode::Title);
		break;

	case IDM_GROUPBY_SUBJECT:
		OnGroupBy(SortMode::Subject);
		break;

	case IDM_GROUPBY_AUTHOR:
		OnGroupBy(SortMode::Authors);
		break;

	case IDM_GROUPBY_KEYWORDS:
		OnGroupBy(SortMode::Keywords);
		break;

	case IDM_GROUPBY_COMMENTS:
		OnGroupBy(SortMode::Comments);
		break;

	case IDM_GROUPBY_CAMERAMODEL:
		OnGroupBy(SortMode::CameraModel);
		break;

	case IDM_GROUPBY_DATETAKEN:
		OnGroupBy(SortMode::DateTaken);
		break;

	case IDM_GROUPBY_WIDTH:
		OnGroupBy(SortMode::Width);
		break;

	case IDM_GROUPBY_HEIGHT:
		OnGroupBy(SortMode::Height);
		break;

	case IDM_GROUPBY_VIRTUALCOMMENTS:
		OnGroupBy(SortMode::VirtualComments);
		break;

	case IDM_GROUPBY_FILESYSTEM:
		OnGroupBy(SortMode::FileSystem);
		break;

	case IDM_GROUPBY_NUMPRINTERDOCUMENTS:
		OnGroupBy(SortMode::NumPrinterDocuments);
		break;

	case IDM_GROUPBY_PRINTERSTATUS:
		OnGroupBy(SortMode::PrinterStatus);
		break;

	case IDM_GROUPBY_PRINTERCOMMENTS:
		OnGroupBy(SortMode::PrinterComments);
		break;

	case IDM_GROUPBY_PRINTERLOCATION:
		OnGroupBy(SortMode::PrinterLocation);
		break;

	case IDM_GROUPBY_NETWORKADAPTER_STATUS:
		OnGroupBy(SortMode::NetworkAdapterStatus);
		break;

	case IDM_GROUPBY_MEDIA_BITRATE:
		OnGroupBy(SortMode::MediaBitrate);
		break;

	case IDM_GROUPBY_MEDIA_COPYRIGHT:
		OnGroupBy(SortMode::MediaCopyright);
		break;

	case IDM_GROUPBY_MEDIA_DURATION:
		OnGroupBy(SortMode::MediaDuration);
		break;

	case IDM_GROUPBY_MEDIA_PROTECTED:
		OnGroupBy(SortMode::MediaProtected);
		break;

	case IDM_GROUPBY_MEDIA_RATING:
		OnGroupBy(SortMode::MediaRating);
		break;

	case IDM_GROUPBY_MEDIA_ALBUM_ARTIST:
		OnGroupBy(SortMode::MediaAlbumArtist);
		break;

	case IDM_GROUPBY_MEDIA_ALBUM:
		OnGroupBy(SortMode::MediaAlbum);
		break;

	case IDM_GROUPBY_MEDIA_BEATS_PER_MINUTE:
		OnGroupBy(SortMode::MediaBeatsPerMinute);
		break;

	case IDM_GROUPBY_MEDIA_COMPOSER:
		OnGroupBy(SortMode::MediaComposer);
		break;

	case IDM_GROUPBY_MEDIA_CONDUCTOR:
		OnGroupBy(SortMode::MediaConductor);
		break;

	case IDM_GROUPBY_MEDIA_DIRECTOR:
		OnGroupBy(SortMode::MediaDirector);
		break;

	case IDM_GROUPBY_MEDIA_GENRE:
		OnGroupBy(SortMode::MediaGenre);
		break;

	case IDM_GROUPBY_MEDIA_LANGUAGE:
		OnGroupBy(SortMode::MediaLanguage);
		break;

	case IDM_GROUPBY_MEDIA_BROADCAST_DATE:
		OnGroupBy(SortMode::MediaBroadcastDate);
		break;

	case IDM_GROUPBY_MEDIA_CHANNEL:
		OnGroupBy(SortMode::MediaChannel);
		break;

	case IDM_GROUPBY_MEDIA_STATION_NAME:
		OnGroupBy(SortMode::MediaStationName);
		break;

	case IDM_GROUPBY_MEDIA_MOOD:
		OnGroupBy(SortMode::MediaMood);
		break;

	case IDM_GROUPBY_MEDIA_PARENTAL_RATING:
		OnGroupBy(SortMode::MediaParentalRating);
		break;

	case IDM_GROUPBY_MEDIA_PARENTAL_RATING_REASON:
		OnGroupBy(SortMode::MediaParentalRatingReason);
		break;

	case IDM_GROUPBY_MEDIA_PERIOD:
		OnGroupBy(SortMode::MediaPeriod);
		break;

	case IDM_GROUPBY_MEDIA_PRODUCER:
		OnGroupBy(SortMode::MediaProducer);
		break;

	case IDM_GROUPBY_MEDIA_PUBLISHER:
		OnGroupBy(SortMode::MediaPublisher);
		break;

	case IDM_GROUPBY_MEDIA_WRITER:
		OnGroupBy(SortMode::MediaWriter);
		break;

	case IDM_GROUPBY_MEDIA_YEAR:
		OnGroupBy(SortMode::MediaYear);
		break;

	case IDM_SORT_ASCENDING:
		OnSortByAscending(TRUE);
		break;

	case IDM_SORT_DESCENDING:
		OnSortByAscending(FALSE);
		break;

	case IDM_VIEW_AUTOARRANGE:
		m_tabContainer->GetSelectedTab().GetShellBrowser()->SetAutoArrange(!m_tabContainer->GetSelectedTab().GetShellBrowser()->GetAutoArrange());
		break;

	case IDM_VIEW_SHOWHIDDENFILES:
		OnShowHiddenFiles();
		break;

	case ToolbarButton::Refresh:
	case IDM_VIEW_REFRESH:
		OnRefresh();
		break;

	case IDM_SORTBY_MORE:
	case IDM_VIEW_SELECTCOLUMNS:
		OnSelectColumns();
		break;

	case IDM_VIEW_AUTOSIZECOLUMNS:
		OnAutoSizeColumns();
		break;

	case IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT:
	{
		/* Dump the columns from the current tab, and save
		them as the default columns for the appropriate folder
		type.. */
		auto currentColumns = m_pActiveShellBrowser->ExportCurrentColumns();
		auto pidl = m_pActiveShellBrowser->GetDirectoryIdl();

		unique_pidl_absolute pidlDrives;
		SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, wil::out_param(pidlDrives));

		unique_pidl_absolute pidlControls;
		SHGetFolderLocation(nullptr, CSIDL_CONTROLS, nullptr, 0, wil::out_param(pidlControls));

		unique_pidl_absolute pidlBitBucket;
		SHGetFolderLocation(nullptr, CSIDL_BITBUCKET, nullptr, 0, wil::out_param(pidlBitBucket));

		unique_pidl_absolute pidlPrinters;
		SHGetFolderLocation(nullptr, CSIDL_PRINTERS, nullptr, 0, wil::out_param(pidlPrinters));

		unique_pidl_absolute pidlConnections;
		SHGetFolderLocation(nullptr, CSIDL_CONNECTIONS, nullptr, 0, wil::out_param(pidlConnections));

		unique_pidl_absolute pidlNetwork;
		SHGetFolderLocation(nullptr, CSIDL_NETWORK, nullptr, 0, wil::out_param(pidlNetwork));

		IShellFolder *pShellFolder;
		SHGetDesktopFolder(&pShellFolder);

		if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlDrives.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.myComputerColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlControls.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.controlPanelColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlBitBucket.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.recycleBinColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlPrinters.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.printersColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlConnections.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.networkConnectionsColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlNetwork.get()) == 0)
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

	case ToolbarButton::NewFolder:
	case IDM_ACTIONS_NEWFOLDER:
		OnCreateNewFolder();
		break;

	case ToolbarButton::MergeFiles:
	case IDM_ACTIONS_MERGEFILES:
		OnMergeFiles();
		break;

	case ToolbarButton::SplitFile:
	case IDM_ACTIONS_SPLITFILE:
		OnSplitFile();
		break;

	case IDM_ACTIONS_DESTROYFILES:
		OnDestroyFiles();
		break;

	case ToolbarButton::Back:
	case IDM_GO_BACK:
		OnGoBack();
		break;

	case ToolbarButton::Forward:
	case IDM_GO_FORWARD:
		OnGoForward();
		break;

	case ToolbarButton::Up:
	case IDM_GO_UPONELEVEL:
		m_navigation->OnNavigateUp();
		break;

	case IDM_GO_MYCOMPUTER:
		OnGoToKnownFolder(FOLDERID_ComputerFolder);
		break;

	case IDM_GO_MYDOCUMENTS:
		OnGoToKnownFolder(FOLDERID_Documents);
		break;

	case IDM_GO_MYMUSIC:
		OnGoToKnownFolder(FOLDERID_Music);
		break;

	case IDM_GO_MYPICTURES:
		OnGoToKnownFolder(FOLDERID_Pictures);
		break;

	case IDM_GO_DESKTOP:
		OnGoToKnownFolder(FOLDERID_Desktop);
		break;

	case IDM_GO_RECYCLEBIN:
		OnGoToKnownFolder(FOLDERID_RecycleBinFolder);
		break;

	case IDM_GO_CONTROLPANEL:
		OnGoToKnownFolder(FOLDERID_ControlPanelFolder);
		break;

	case IDM_GO_PRINTERS:
		OnGoToKnownFolder(FOLDERID_PrintersFolder);
		break;

	case IDM_GO_CDBURNING:
		OnGoToKnownFolder(FOLDERID_CDBurning);
		break;

	case IDM_GO_MYNETWORKPLACES:
		OnGoToKnownFolder(FOLDERID_NetworkFolder);
		break;

	case IDM_GO_NETWORKCONNECTIONS:
		OnGoToKnownFolder(FOLDERID_ConnectionsFolder);
		break;

	case ToolbarButton::AddBookmark:
	case IDM_BOOKMARKS_BOOKMARKTHISTAB:
		BookmarkHelper::AddBookmarkItem(&m_bookmarkTree, BookmarkItem::Type::Bookmark,
			nullptr, std::nullopt, hwnd, this);
		break;

	case IDM_BOOKMARKS_BOOKMARK_ALL_TABS:
		BookmarkHelper::BookmarkAllTabs(&m_bookmarkTree, m_hLanguageModule, hwnd, this);
		break;

	case ToolbarButton::Bookmarks:
	case IDM_BOOKMARKS_MANAGEBOOKMARKS:
		if (g_hwndManageBookmarks == nullptr)
		{
			auto *pManageBookmarksDialog = new ManageBookmarksDialog(m_hLanguageModule,
				hwnd, this, m_navigation.get(), &m_bookmarkIconFetcher, &m_bookmarkTree);
			g_hwndManageBookmarks = pManageBookmarksDialog->ShowModelessDialog(new ModelessDialogNotification());
		}
		else
		{
			SetFocus(g_hwndManageBookmarks);
		}
		break;

	case ToolbarButton::Search:
	case IDM_TOOLS_SEARCH:
		OnSearch();
		break;

	case IDM_TOOLS_CUSTOMIZECOLORS:
		OnCustomizeColors();
		break;

	case IDM_TOOLS_RUNSCRIPT:
		OnRunScript();
		break;

	case IDM_TOOLS_OPTIONS:
		OnShowOptions();
		break;

	case IDM_HELP_HELP:
		OnShowHelp();
		break;

	case IDM_HELP_CHECKFORUPDATES:
		OnCheckForUpdates();
		break;

	case IDM_HELP_ABOUT:
		OnAbout();
		break;


	case IDA_NEXTTAB:
		m_tabContainer->SelectAdjacentTab(TRUE);
		break;

	case IDA_PREVIOUSTAB:
		m_tabContainer->SelectAdjacentTab(FALSE);
		break;

	case IDA_ADDRESSBAR:
		SetFocus(m_addressBar->GetHWND());
		break;

	case IDA_COMBODROPDOWN:
		SetFocus(m_addressBar->GetHWND());
		SendMessage(m_addressBar->GetHWND(), CB_SHOWDROPDOWN, TRUE, 0);
		break;

	case IDA_PREVIOUSWINDOW:
		OnPreviousWindow();
		break;

	case IDA_NEXTWINDOW:
		OnNextWindow();
		break;

	case IDA_RCLICK:
		OnIdaRClick();
		break;

	case IDA_TAB_DUPLICATETAB:
		m_tabContainer->DuplicateTab(m_tabContainer->GetSelectedTab());
		break;

	case IDA_HOME:
		OnGoHome();
		break;

	case IDA_TAB1:
		OnSelectTabByIndex(0);
		break;

	case IDA_TAB2:
		OnSelectTabByIndex(1);
		break;

	case IDA_TAB3:
		OnSelectTabByIndex(2);
		break;

	case IDA_TAB4:
		OnSelectTabByIndex(3);
		break;

	case IDA_TAB5:
		OnSelectTabByIndex(4);
		break;

	case IDA_TAB6:
		OnSelectTabByIndex(5);
		break;

	case IDA_TAB7:
		OnSelectTabByIndex(6);
		break;

	case IDA_TAB8:
		OnSelectTabByIndex(7);
		break;

	case IDA_LASTTAB:
		OnSelectTabByIndex(-1);
		break;

	case IDA_RESTORE_LAST_TAB:
		m_tabRestorer->RestoreLastTab();
		break;

	case ToolbarButton::Views:
		OnToolbarViews();
		break;

		/* Display window menus. */
	case IDM_DW_HIDEDISPLAYWINDOW:
		m_config->showDisplayWindow = FALSE;
		lShowWindow(m_hDisplayWindow, m_config->showDisplayWindow);
		ResizeWindows();
		break;
	}

	return 1;
}

LRESULT Explorerplusplus::HandleControlNotification(HWND hwnd, WPARAM wParam)
{
	UNREFERENCED_PARAMETER(hwnd);

	switch (HIWORD(wParam))
	{
	case CBN_DROPDOWN:
		AddPathsToComboBoxEx(m_addressBar->GetHWND(), m_CurrentDirectory.c_str());
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

	switch(nmhdr->code)
	{
		case NM_CLICK:
			if(m_config->globalFolderSettings.oneClickActivate)
			{
				OnListViewDoubleClick(&((NMITEMACTIVATE *)lParam)->hdr);
			}
			break;

		case NM_DBLCLK:
			OnListViewDoubleClick(nmhdr);
			break;

		case NM_RCLICK:
			OnRightClick(nmhdr);
			break;

		case NM_CUSTOMDRAW:
			return OnCustomDraw(lParam);

		case LVN_KEYDOWN:
			return OnListViewKeyDown(lParam);

		case LVN_ITEMCHANGING:
			return OnListViewItemChanging(reinterpret_cast<NMLISTVIEW *>(lParam));

		case LVN_BEGINDRAG:
			OnListViewBeginDrag(lParam,DragType::LeftClick);
			break;

		case LVN_BEGINLABELEDIT:
			return OnListViewBeginLabelEdit(lParam);

		case LVN_ENDLABELEDIT:
			return OnListViewEndLabelEdit(lParam);

		case LVN_DELETEALLITEMS:
			// Respond to the notification in order to speed up calls to ListView_DeleteAllItems
			// per http://www.verycomputer.com/5_0c959e6a4fd713e2_1.htm
			return TRUE;

		case TBN_ENDADJUST:
			UpdateToolbarBandSizing(m_hMainRebar,((NMHDR *)lParam)->hwndFrom);
			break;

		case RBN_BEGINDRAG:
			SendMessage(m_hMainRebar,RB_DRAGMOVE,0,-1);
			return 0;

		case RBN_HEIGHTCHANGE:
			/* The listview and treeview may
			need to be moved to accommodate the new
			rebar size. */
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case RBN_CHEVRONPUSHED:
			{
				NMREBARCHEVRON *pnmrc = nullptr;
				HWND hToolbar = nullptr;
				HMENU hMenu;
				HIMAGELIST himlSmall;
				MENUITEMINFO mii;
				TCHAR szText[512];
				TBBUTTON tbButton;
				RECT rcToolbar;
				RECT rcButton;
				RECT rcIntersect;
				LRESULT lResult;
				BOOL bIntersect;
				int iMenu = 0;
				int nButtons;
				int i = 0;

				pnmrc = (NMREBARCHEVRON *)lParam;

				POINT ptMenu;
				ptMenu.x = pnmrc->rc.left;
				ptMenu.y = pnmrc->rc.bottom;
				ClientToScreen(m_hMainRebar, &ptMenu);

				if (pnmrc->wID == ID_BOOKMARKSTOOLBAR)
				{
					m_pBookmarksToolbar->ShowOverflowMenu(ptMenu);
					return 0;
				}

				hMenu = CreatePopupMenu();

				HIMAGELIST himlMenu = nullptr;

				Shell_GetImageLists(nullptr,&himlSmall);

				switch(pnmrc->wID)
				{
				case ID_MAINTOOLBAR:
					hToolbar = m_mainToolbar->GetHWND();
					break;

				case ID_DRIVESTOOLBAR:
					hToolbar = m_pDrivesToolbar->GetHWND();
					himlMenu = himlSmall;
					break;

				case ID_APPLICATIONSTOOLBAR:
					hToolbar = m_pApplicationToolbar->GetHWND();
					himlMenu = himlSmall;
					break;
				}

				nButtons = (int)SendMessage(hToolbar,TB_BUTTONCOUNT,0,0);

				GetClientRect(hToolbar,&rcToolbar);

				for(i = 0;i < nButtons;i++)
				{
					lResult = SendMessage(hToolbar,TB_GETITEMRECT,i,(LPARAM)&rcButton);

					if(lResult)
					{
						bIntersect = IntersectRect(&rcIntersect,&rcToolbar,&rcButton);

						if(!bIntersect || rcButton.right > rcToolbar.right)
						{
							SendMessage(hToolbar,TB_GETBUTTON,i,(LPARAM)&tbButton);

							if(tbButton.fsStyle & BTNS_SEP)
							{
								mii.cbSize		= sizeof(mii);
								mii.fMask		= MIIM_FTYPE;
								mii.fType		= MFT_SEPARATOR;
								InsertMenuItem(hMenu,i,TRUE,&mii);
							}
							else
							{
								if(IS_INTRESOURCE(tbButton.iString))
								{
									SendMessage(hToolbar,TB_GETSTRING,MAKEWPARAM(SIZEOF_ARRAY(szText),
										tbButton.iString),(LPARAM)szText);
								}
								else
								{
									StringCchCopy(szText,SIZEOF_ARRAY(szText),(LPCWSTR)tbButton.iString);
								}

								HMENU hSubMenu = nullptr;
								UINT fMask;

								fMask = MIIM_ID|MIIM_STRING;
								hSubMenu = nullptr;

								switch(pnmrc->wID)
								{
								case ID_MAINTOOLBAR:
									{
										switch(tbButton.idCommand)
										{
										case ToolbarButton::Back:
											hSubMenu = CreateRebarHistoryMenu(TRUE);
											fMask |= MIIM_SUBMENU;
											break;

										case ToolbarButton::Forward:
											hSubMenu = CreateRebarHistoryMenu(FALSE);
											fMask |= MIIM_SUBMENU;
											break;

										case ToolbarButton::Views:
											hSubMenu = BuildViewsMenu();
											fMask |= MIIM_SUBMENU;
											break;
										}
									}
									break;
								}

								mii.cbSize		= sizeof(mii);
								mii.fMask		= fMask;
								mii.wID			= tbButton.idCommand;
								mii.hSubMenu	= hSubMenu;
								mii.dwTypeData	= szText;
								InsertMenuItem(hMenu,iMenu,TRUE,&mii);

								/* TODO: Update the image
								for this menu item. */
							}
							iMenu++;
						}
					}
				}

				UINT uFlags = TPM_LEFTALIGN|TPM_RETURNCMD;
				int iCmd;

				iCmd = TrackPopupMenu(hMenu,uFlags,
					ptMenu.x,ptMenu.y,0,m_hMainRebar, nullptr);

				if(iCmd != 0)
				{
					/* We'll handle the back and forward buttons
					in place, and send the rest of the messages
					back to the main window. */
					if((iCmd >= ID_REBAR_MENU_BACK_START &&
						iCmd <= ID_REBAR_MENU_BACK_END) ||
						(iCmd >= ID_REBAR_MENU_FORWARD_START &&
						iCmd <= ID_REBAR_MENU_FORWARD_END))
					{
						if(iCmd >= ID_REBAR_MENU_BACK_START &&
							iCmd <= ID_REBAR_MENU_BACK_END)
						{
							iCmd = -(iCmd - ID_REBAR_MENU_BACK_START);
						}
						else
						{
							iCmd = iCmd - ID_REBAR_MENU_FORWARD_START;
						}

						OnGoToOffset(iCmd);
					}
					else
					{
						SendMessage(m_hContainer,WM_COMMAND,MAKEWPARAM(iCmd,0),0);
					}
				}

				DestroyMenu(hMenu);
			}
			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}