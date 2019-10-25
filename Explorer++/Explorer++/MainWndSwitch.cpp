// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddBookmarkDialog.h"
#include "AddressBar.h"
#include "ApplicationToolbar.h"
#include "Config.h"
#include "DrivesToolbar.h"
#include "HolderWindow.h"
#include "IModelessDialogNotification.h"
#include "MainResource.h"
#include "ManageBookmarksDialog.h"
#include "MenuRanges.h"
#include "ModelessDialogs.h"
#include "Navigation.h"
#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "TabBacking.h"
#include "ToolbarButtons.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/Controls.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../MyTreeView/MyTreeView.h"


static const int FOLDER_SIZE_LINE_INDEX = 1;

/* Defines the distance between the cursor
and the right edge of the treeview during
a resizing operation. */
static const int TREEVIEW_DRAG_OFFSET = 8;

LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)GetWindowLongPtr(hwnd,GWLP_USERDATA);

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
			break;
	}

	/* Jump across to the member window function (will handle all requests). */
	if(pContainer != NULL)
		return pContainer->WindowProcedure(hwnd,Msg,wParam,lParam);
	else
		return DefWindowProc(hwnd,Msg,wParam,lParam);
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
		break;

	case WM_INITMENU:
		SetProgramMenuItemStates((HMENU)wParam);
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
		break;

	case WM_USER_UPDATEWINDOWS:
		UpdateWindowStates();
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

	case WM_USER_TREEVIEW_GAINEDFOCUS:
		m_hLastActiveWindow = m_hTreeView;
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
			DWFolderSizeCompletion_t *pDWFolderSizeCompletion = NULL;
			TCHAR szFolderSize[32];
			TCHAR szSizeString[64];
			TCHAR szTotalSize[64];
			BOOL bValid = FALSE;

			pDWFolderSizeCompletion = (DWFolderSizeCompletion_t *)wParam;

			std::list<DWFolderSize_t>::iterator itr;

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
			COPYDATASTRUCT *pcds = reinterpret_cast<COPYDATASTRUCT *>(lParam);

			if (pcds->lpData != NULL)
			{
				m_tabContainer->CreateNewTab((TCHAR *)pcds->lpData, TabSettings(_selected = true));
			}
			else
			{
				HRESULT hr = m_tabContainer->CreateNewTab(m_config->defaultTabDirectory.c_str(), TabSettings(_selected = true));

				if (FAILED(hr))
				{
					m_tabContainer->CreateNewTab(m_config->defaultTabDirectoryStatic.c_str(), TabSettings(_selected = true));
				}
			}

			return TRUE;
		}
		break;

	case WM_NDW_RCLICK:
		{
			POINT pt;
			POINTSTOPOINT(pt, MAKEPOINTS(lParam));
			OnNdwRClick(&pt);
		}
		break;

	case WM_NDW_ICONRCLICK:
		{
			POINT pt;
			POINTSTOPOINT(pt, MAKEPOINTS(lParam));
			OnNdwIconRClick(&pt);
		}
		break;

	case WM_CHANGECBCHAIN:
		OnChangeCBChain(wParam,lParam);
		break;

	case WM_DRAWCLIPBOARD:
		OnDrawClipboard();
		break;

	case WM_APPCOMMAND:
		OnAppCommand(GET_APPCOMMAND_LPARAM(lParam));
		break;

	case WM_COMMAND:
		return CommandHandler(hwnd,wParam);
		break;

	case WM_NOTIFY:
		return NotifyHandler(hwnd, Msg, wParam, lParam);
		break;

	case WM_SIZE:
		return OnSize(LOWORD(lParam),HIWORD(lParam));
		break;

	case WM_CLOSE:
		return OnClose();
		break;

	case WM_DESTROY:
		return OnDestroy();
		break;
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
		/* TODO: [Bookmarks] Open bookmark. */
	}
	else if (HIWORD(wParam) == 0 && LOWORD(wParam) >= MENU_HEADER_STARTID &&
		LOWORD(wParam) <= MENU_HEADER_ENDID)
	{
		int iOffset;

		iOffset = LOWORD(wParam) - MENU_HEADER_STARTID;

		int							iItem = 0;
		unsigned int *pHeaderList = NULL;

		auto currentColumns = m_pActiveShellBrowser->ExportCurrentColumns();

		GetColumnHeaderMenuList(&pHeaderList);

		/* Loop through all current items to find the item that was clicked, and
		flip its active state. */
		for (auto itr = currentColumns.begin(); itr != currentColumns.end(); itr++)
		{
			if (itr->id == pHeaderList[iOffset])
			{
				itr->bChecked = !itr->bChecked;
				break;
			}

			iItem++;
		}

		/* If it was the first column that was changed, need to refresh
		all columns. */
		if (iOffset == 0)
		{
			m_pActiveShellBrowser->ImportColumns(currentColumns);

			Tab &tab = m_tabContainer->GetSelectedTab();
			RefreshTab(tab);
		}
		else
		{
			m_pActiveShellBrowser->ImportColumns(currentColumns);
		}
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
	case TOOLBAR_NEWTAB:
	case IDM_FILE_NEWTAB:
		OnNewTab();
		break;

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

	case TOOLBAR_OPENCOMMANDPROMPT:
	case IDM_FILE_OPENCOMMANDPROMPT:
		StartCommandPrompt(m_CurrentDirectory, false);
		break;

	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		StartCommandPrompt(m_CurrentDirectory, true);
		break;

	case IDM_FILE_COPYFOLDERPATH:
		CopyTextToClipboard(m_CurrentDirectory);
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

	case TOOLBAR_DELETE:
	case IDM_FILE_DELETE:
		OnFileDelete(false);
		break;

	case TOOLBAR_DELETEPERMANENTLY:
	case IDM_FILE_DELETEPERMANENTLY:
		OnFileDelete(true);
		break;

	case IDM_FILE_RENAME:
		OnFileRename();
		break;

	case TOOLBAR_PROPERTIES:
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

	case TOOLBAR_COPY:
	case IDM_EDIT_COPY:
		OnCopy(TRUE);
		break;

	case TOOLBAR_CUT:
	case IDM_EDIT_CUT:
		OnCopy(FALSE);
		break;

	case TOOLBAR_PASTE:
	case IDM_EDIT_PASTE:
		OnPaste();
		break;

	case IDM_EDIT_PASTESHORTCUT:
		PasteLinksToClipboardFiles(m_CurrentDirectory);
		break;

	case IDM_EDIT_PASTEHARDLINK:
		PasteHardLinks(m_CurrentDirectory);
		break;

	case IDM_EDIT_COPYTOFOLDER:
	case TOOLBAR_COPYTO:
		CopyToFolder(false);
		break;

	case TOOLBAR_MOVETO:
	case IDM_EDIT_MOVETOFOLDER:
		CopyToFolder(true);
		break;

	case IDM_EDIT_SELECTALL:
		m_bCountingUp = TRUE;
		NListView::ListView_SelectAllItems(m_hActiveListView, TRUE);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_INVERTSELECTION:
		m_bInverted = TRUE;
		m_nSelectedOnInvert = m_nSelected;
		NListView::ListView_InvertSelection(m_hActiveListView);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_SELECTALLFOLDERS:
		HighlightSimilarFiles(m_hActiveListView);
		SetFocus(m_hActiveListView);
		break;

	case IDM_EDIT_SELECTNONE:
		m_bCountingDown = TRUE;
		NListView::ListView_SelectAllItems(m_hActiveListView, FALSE);
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

	case IDM_VIEW_FOLDERS:
	case TOOLBAR_FOLDERS:
		ToggleFolders();
		break;

	case IDM_VIEW_DISPLAYWINDOW:
		m_config->showDisplayWindow = !m_config->showDisplayWindow;
		lShowWindow(m_hDisplayWindow, m_config->showDisplayWindow);
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
		m_pActiveShellBrowser->SetViewMode(ViewMode::ExtraLargeIcons);
		break;

	case IDM_VIEW_LARGEICONS:
		m_pActiveShellBrowser->SetViewMode(ViewMode::LargeIcons);
		break;

	case IDM_VIEW_ICONS:
		m_pActiveShellBrowser->SetViewMode(ViewMode::Icons);
		break;

	case IDM_VIEW_SMALLICONS:
		m_pActiveShellBrowser->SetViewMode(ViewMode::SmallIcons);
		break;

	case IDM_VIEW_LIST:
		m_pActiveShellBrowser->SetViewMode(ViewMode::List);
		break;

	case IDM_VIEW_DETAILS:
		m_pActiveShellBrowser->SetViewMode(ViewMode::Details);
		break;

	case IDM_VIEW_THUMBNAILS:
		m_pActiveShellBrowser->SetViewMode(ViewMode::Thumbnails);
		break;

	case IDM_VIEW_TILES:
		m_pActiveShellBrowser->SetViewMode(ViewMode::Tiles);
		break;

	case IDM_VIEW_CHANGEDISPLAYCOLOURS:
		OnChangeDisplayColors();
		break;

	case IDM_FILTER_FILTERRESULTS:
		OnFilterResults();
		break;

	case IDM_FILTER_APPLYFILTER:
		ToggleFilterStatus();
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

	case IDM_ARRANGEICONSBY_ASCENDING:
		OnSortByAscending(TRUE);
		break;

	case IDM_ARRANGEICONSBY_DESCENDING:
		OnSortByAscending(FALSE);
		break;

	case IDM_ARRANGEICONSBY_AUTOARRANGE:
		m_pActiveShellBrowser->SetAutoArrange(!m_pActiveShellBrowser->GetAutoArrange());
		break;

	case IDM_VIEW_SHOWHIDDENFILES:
		OnShowHiddenFiles();
		break;

	case TOOLBAR_REFRESH:
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
		IShellFolder *pShellFolder = NULL;
		LPITEMIDLIST pidl = NULL;
		LPITEMIDLIST pidlDrives = NULL;
		LPITEMIDLIST pidlControls = NULL;
		LPITEMIDLIST pidlBitBucket = NULL;
		LPITEMIDLIST pidlPrinters = NULL;
		LPITEMIDLIST pidlConnections = NULL;
		LPITEMIDLIST pidlNetwork = NULL;

		auto currentColumns = m_pActiveShellBrowser->ExportCurrentColumns();

		pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

		SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidlDrives);
		SHGetFolderLocation(NULL, CSIDL_CONTROLS, NULL, 0, &pidlControls);
		SHGetFolderLocation(NULL, CSIDL_BITBUCKET, NULL, 0, &pidlBitBucket);
		SHGetFolderLocation(NULL, CSIDL_PRINTERS, NULL, 0, &pidlPrinters);
		SHGetFolderLocation(NULL, CSIDL_CONNECTIONS, NULL, 0, &pidlConnections);
		SHGetFolderLocation(NULL, CSIDL_NETWORK, NULL, 0, &pidlNetwork);

		SHGetDesktopFolder(&pShellFolder);

		if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl, pidlDrives) == 0)
		{
			m_config->globalFolderSettings.folderColumns.myComputerColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl, pidlControls) == 0)
		{
			m_config->globalFolderSettings.folderColumns.controlPanelColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl, pidlBitBucket) == 0)
		{
			m_config->globalFolderSettings.folderColumns.recycleBinColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl, pidlPrinters) == 0)
		{
			m_config->globalFolderSettings.folderColumns.printersColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl, pidlConnections) == 0)
		{
			m_config->globalFolderSettings.folderColumns.networkConnectionsColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl, pidlNetwork) == 0)
		{
			m_config->globalFolderSettings.folderColumns.myNetworkPlacesColumns = currentColumns;
		}
		else
		{
			m_config->globalFolderSettings.folderColumns.realFolderColumns = currentColumns;
		}

		pShellFolder->Release();

		CoTaskMemFree(pidlNetwork);
		CoTaskMemFree(pidlConnections);
		CoTaskMemFree(pidlPrinters);
		CoTaskMemFree(pidlBitBucket);
		CoTaskMemFree(pidlControls);
		CoTaskMemFree(pidlDrives);
		CoTaskMemFree(pidl);
	}
	break;

	case TOOLBAR_NEWFOLDER:
	case IDM_ACTIONS_NEWFOLDER:
		OnCreateNewFolder();
		break;

	case IDM_ACTIONS_MERGEFILES:
		OnMergeFiles();
		break;

	case IDM_ACTIONS_SPLITFILE:
		OnSplitFile();
		break;

	case IDM_ACTIONS_DESTROYFILES:
		OnDestroyFiles();
		break;

	case IDM_GO_BACK:
	case TOOLBAR_BACK:
		m_navigation->OnBrowseBack();
		break;

	case IDM_GO_FORWARD:
	case TOOLBAR_FORWARD:
		m_navigation->OnBrowseForward();
		break;

	case IDM_GO_UPONELEVEL:
	case TOOLBAR_UP:
		m_navigation->OnNavigateUp();
		break;

	case IDM_GO_MYCOMPUTER:
		m_navigation->OnGotoFolder(CSIDL_DRIVES);
		break;

	case IDM_GO_MYDOCUMENTS:
		m_navigation->OnGotoFolder(CSIDL_PERSONAL);
		break;

	case IDM_GO_MYMUSIC:
		m_navigation->OnGotoFolder(CSIDL_MYMUSIC);
		break;

	case IDM_GO_MYPICTURES:
		m_navigation->OnGotoFolder(CSIDL_MYPICTURES);
		break;

	case IDM_GO_DESKTOP:
		m_navigation->OnGotoFolder(CSIDL_DESKTOP);
		break;

	case IDM_GO_RECYCLEBIN:
		m_navigation->OnGotoFolder(CSIDL_BITBUCKET);
		break;

	case IDM_GO_CONTROLPANEL:
		m_navigation->OnGotoFolder(CSIDL_CONTROLS);
		break;

	case IDM_GO_PRINTERS:
		m_navigation->OnGotoFolder(CSIDL_PRINTERS);
		break;

	case IDM_GO_CDBURNING:
		m_navigation->OnGotoFolder(CSIDL_CDBURN_AREA);
		break;

	case IDM_GO_MYNETWORKPLACES:
		m_navigation->OnGotoFolder(CSIDL_NETWORK);
		break;

	case IDM_GO_NETWORKCONNECTIONS:
		m_navigation->OnGotoFolder(CSIDL_CONNECTIONS);
		break;

	case TOOLBAR_ADDBOOKMARK:
	case IDM_BOOKMARKS_BOOKMARKTHISTAB:
	{
		TCHAR szCurrentDirectory[MAX_PATH];
		TCHAR szDisplayName[MAX_PATH];
		m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory), szCurrentDirectory);
		GetDisplayName(szCurrentDirectory, szDisplayName, SIZEOF_ARRAY(szDisplayName), SHGDN_INFOLDER);
		CBookmark Bookmark = CBookmark::Create(szDisplayName, szCurrentDirectory, EMPTY_STRING);

		CAddBookmarkDialog AddBookmarkDialog(m_hLanguageModule, IDD_ADD_BOOKMARK, hwnd, *m_bfAllBookmarks, Bookmark);
		AddBookmarkDialog.ShowModalDialog();
	}
	break;

	case TOOLBAR_ORGANIZEBOOKMARKS:
	case IDM_BOOKMARKS_MANAGEBOOKMARKS:
		if (g_hwndManageBookmarks == NULL)
		{
			CManageBookmarksDialog *pManageBookmarksDialog = new CManageBookmarksDialog(m_hLanguageModule, IDD_MANAGE_BOOKMARKS,
				hwnd, this, m_navigation, *m_bfAllBookmarks);
			g_hwndManageBookmarks = pManageBookmarksDialog->ShowModelessDialog(new CModelessDialogNotification());
		}
		else
		{
			SetFocus(g_hwndManageBookmarks);
		}
		break;

	case TOOLBAR_SEARCH:
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
		m_navigation->OnNavigateHome();
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

	case TOOLBAR_VIEWS:
		OnToolbarViews();
		break;

		/* Messages from the context menu that
		is used with the bookmarks toolbar. */
		/* TODO: [Bookmarks] Handle menu messages. */
	case IDM_BT_OPEN:
		break;

	case IDM_BT_OPENINNEWTAB:
		break;

	case IDM_BT_DELETE:
		break;

	case IDM_BT_PROPERTIES:
		break;

	case IDM_BT_NEWBOOKMARK:
		break;

	case IDM_BT_NEWFOLDER:
		break;

		/* Listview column header context menu. */
	case IDM_HEADER_MORE:
		OnSelectColumns();
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
		AddPathsToComboBoxEx(m_addressBar->GetHWND(), m_CurrentDirectory);
		break;
	}

	return 1;
}

/*
 * WM_NOTIFY handler for the main window.
 */
LRESULT CALLBACK Explorerplusplus::NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NMHDR *nmhdr = reinterpret_cast<NMHDR *>(lParam);

	switch(nmhdr->code)
	{
		case NM_CLICK:
			if(m_config->globalFolderSettings.oneClickActivate && !m_bSelectionFromNowhere)
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
			break;

		case LVN_KEYDOWN:
			return OnListViewKeyDown(lParam);
			break;

		case LVN_ITEMCHANGING:
			{
				int tabId = DetermineListViewObjectIndex(hwnd);

				if (tabId == -1)
				{
					return FALSE;
				}

				Tab &tab = m_tabContainer->GetTab(tabId);

				UINT uViewMode = tab.GetShellBrowser()->GetViewMode();

				if(uViewMode == ViewMode::List)
				{
					if(m_bBlockNext)
					{
						m_bBlockNext = FALSE;
						return TRUE;
					}
				}

				return FALSE;
			}
			break;

		case LVN_ITEMCHANGED:
			OnListViewItemChanged(lParam);
			break;

		case LVN_BEGINDRAG:
			OnListViewBeginDrag(lParam,DRAG_TYPE_LEFTCLICK);
			break;

		case LVN_BEGINLABELEDIT:
			return OnListViewBeginLabelEdit(lParam);
			break;

		case LVN_ENDLABELEDIT:
			return OnListViewEndLabelEdit(lParam);
			break;

		case TBN_ENDADJUST:
			UpdateToolbarBandSizing(m_hMainRebar,((NMHDR *)lParam)->hwndFrom);
			break;

		case RBN_BEGINDRAG:
			SendMessage(m_hMainRebar,RB_DRAGMOVE,0,-1);
			return 0;
			break;

		case RBN_HEIGHTCHANGE:
			/* The listview and treeview may
			need to be moved to accommodate the new
			rebar size. */
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case RBN_CHEVRONPUSHED:
			{
				NMREBARCHEVRON *pnmrc = NULL;
				HWND hToolbar = NULL;
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

				hMenu = CreatePopupMenu();

				pnmrc = (NMREBARCHEVRON *)lParam;

				HIMAGELIST himlMenu = nullptr;

				Shell_GetImageLists(NULL,&himlSmall);

				switch(pnmrc->wID)
				{
				case ID_MAINTOOLBAR:
					hToolbar = m_mainToolbar->GetHWND();
					break;

				case ID_BOOKMARKSTOOLBAR:
					hToolbar = m_hBookmarksToolbar;
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

								HMENU hSubMenu = NULL;
								UINT fMask;

								fMask = MIIM_ID|MIIM_STRING;
								hSubMenu = NULL;

								switch(pnmrc->wID)
								{
								case ID_MAINTOOLBAR:
									{
										switch(tbButton.idCommand)
										{
										case TOOLBAR_BACK:
											hSubMenu = CreateRebarHistoryMenu(TRUE);
											fMask |= MIIM_SUBMENU;
											break;

										case TOOLBAR_FORWARD:
											hSubMenu = CreateRebarHistoryMenu(FALSE);
											fMask |= MIIM_SUBMENU;
											break;

										case TOOLBAR_VIEWS:
											hSubMenu = BuildViewsMenu();
											fMask |= MIIM_SUBMENU;
											break;
										}
									}
									break;

								case ID_BOOKMARKSTOOLBAR:
									{
										/* TODO: [Bookmarks] Build menu. */
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

				POINT ptMenu;

				ptMenu.x = pnmrc->rc.left;
				ptMenu.y = pnmrc->rc.bottom;

				ClientToScreen(m_hMainRebar,&ptMenu);

				UINT uFlags = TPM_LEFTALIGN|TPM_RETURNCMD;
				int iCmd;

				iCmd = TrackPopupMenu(hMenu,uFlags,
					ptMenu.x,ptMenu.y,0,m_hMainRebar,NULL);

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
						LPITEMIDLIST pidl = NULL;

						if(iCmd >= ID_REBAR_MENU_BACK_START &&
							iCmd <= ID_REBAR_MENU_BACK_END)
						{
							iCmd = -(iCmd - ID_REBAR_MENU_BACK_START);
						}
						else
						{
							iCmd = iCmd - ID_REBAR_MENU_FORWARD_START;
						}

						pidl = m_pActiveShellBrowser->RetrieveHistoryItem(iCmd);

						m_navigation->BrowseFolderInCurrentTab(pidl,SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

						CoTaskMemFree(pidl);
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