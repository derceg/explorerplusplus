// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "AddressBarView.h"
#include "App.h"
#include "Application.h"
#include "ApplicationEditorDialog.h"
#include "ApplicationToolbar.h"
#include "ApplicationToolbarView.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/ManageBookmarksDialog.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "ClipboardOperations.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "DrivesToolbar.h"
#include "DrivesToolbarView.h"
#include "Explorer++_internal.h"
#include "HolderWindow.h"
#include "MainMenuSubMenuView.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MainToolbarButtons.h"
#include "MenuRanges.h"
#include "ModelessDialogHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "StatusBar.h"
#include "TabBacking.h"
#include "TabContainerImpl.h"
#include "TabRestorer.h"
#include "TabRestorerMenu.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Controls.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"

static const int FOLDER_SIZE_LINE_INDEX = 1;

/* Defines the distance between the cursor
and the right edge of the treeview during
a resizing operation. */
static const int TREEVIEW_DRAG_OFFSET = 8;

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

	case WM_INITMENU:
		OnInitMenu(reinterpret_cast<HMENU>(wParam));
		break;

	case WM_EXITMENULOOP:
		OnExitMenuLoop(wParam);
		break;

	case WM_MENUSELECT:
		m_statusBar->OnMenuSelect(reinterpret_cast<HMENU>(lParam), LOWORD(wParam), HIWORD(wParam));
		break;

	case WM_MBUTTONUP:
		OnMenuMiddleButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			WI_IsFlagSet(wParam, MK_CONTROL), WI_IsFlagSet(wParam, MK_SHIFT));
		break;

	case WM_MENURBUTTONUP:
	{
		POINT pt;
		DWORD messagePos = GetMessagePos();
		POINTSTOPOINT(pt, MAKEPOINTS(messagePos));
		OnMenuRightButtonUp(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam), pt);
	}
	break;

	case WM_TIMER:
		if (wParam == LISTVIEW_ITEM_CHANGED_TIMER_ID)
		{
			Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();

			UpdateDisplayWindow(selectedTab);
			m_mainToolbar->UpdateToolbarButtonStates();

			KillTimer(m_hContainer, LISTVIEW_ITEM_CHANGED_TIMER_ID);
		}
		break;

	case WM_USER_DISPLAYWINDOWRESIZED:
		OnDisplayWindowResized(wParam);
		break;

		// See https://github.com/derceg/explorerplusplus/issues/169.
		/*case WM_APP_ASSOC_CHANGED:
			OnAssocChanged();
			break;*/

	case WM_APP_FOLDERSIZECOMPLETED:
	{
		DWFolderSizeCompletion *pDWFolderSizeCompletion = nullptr;
		TCHAR szSizeString[64];
		TCHAR szTotalSize[64];
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
				if (itr->iTabId == GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetId())
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
				: +SizeDisplayFormat::None;
			auto folderSizeText =
				FormatSizeString(pDWFolderSizeCompletion->liFolderSize.QuadPart, displayFormat);

			LoadString(m_app->GetResourceInstance(), IDS_GENERAL_TOTALSIZE, szTotalSize,
				std::size(szTotalSize));

			StringCchPrintf(szSizeString, std::size(szSizeString), _T("%s: %s"), szTotalSize,
				folderSizeText.c_str());

			/* TODO: The line index should be stored in some other (variable) way. */
			DisplayWindow_SetLine(m_displayWindow->GetHWND(), FOLDER_SIZE_LINE_INDEX, szSizeString);
		}

		free(pDWFolderSizeCompletion);
	}
	break;

	case WM_NDW_RCLICK:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnDisplayWindowRClick(&pt);
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
			m_app->SessionEnding();
		}
		return 0;

	case WM_DESTROY:
		return OnDestroy();

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::CommandHandler(HWND hwnd, HWND control, int id,
	UINT notificationCode)
{
	// Several toolbars will handle their own items.
	if (control
		&& ((m_drivesToolbar && control == m_drivesToolbar->GetView()->GetHWND())
			|| m_applicationToolbar && control == m_applicationToolbar->GetView()->GetHWND()
			|| m_bookmarksToolbar && control == m_bookmarksToolbar->GetView()->GetHWND()))
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
LRESULT Explorerplusplus::HandleMenuOrToolbarButtonOrAccelerator(HWND hwnd, int id,
	UINT notificationCode)
{
	if (notificationCode == 0 && id >= MENU_BOOKMARK_START_ID && id < MENU_BOOKMARK_END_ID)
	{
		m_bookmarksMainMenu->OnMenuItemClicked(id);
		return 0;
	}
	else if (notificationCode == 0 && id >= MENU_PLUGIN_START_ID && id < MENU_PLUGIN_END_ID)
	{
		m_pluginMenuManager.OnMenuItemClicked(id);
		return 0;
	}
	else if (notificationCode == 1 && id >= ACCELERATOR_PLUGIN_START_ID
		&& id < ACCELERATOR_PLUGIN_END_ID)
	{
		m_pluginCommandManager.onAcceleratorPressed(id);
		return 0;
	}
	else if (notificationCode == 0 && MaybeHandleMainMenuItemSelection(id))
	{
		return 0;
	}

	switch (id)
	{
	case MainToolbarButton::NewTab:
	case IDM_FILE_NEWTAB:
		OnNewTab();
		break;

	case MainToolbarButton::CloseTab:
	case TABTOOLBAR_CLOSE:
	case IDM_FILE_CLOSETAB:
		OnCloseTab();
		break;

	case IDM_FILE_NEW_WINDOW:
		CreateNewWindow();
		break;

	case IDM_FILE_CLONEWINDOW:
		OnCloneWindow();
		break;

	case IDM_FILE_SAVEDIRECTORYLISTING:
		OnSaveDirectoryListing();
		break;

	case MainToolbarButton::OpenCommandPrompt:
	case IDM_FILE_OPENCOMMANDPROMPT:
		StartCommandPrompt(m_pActiveShellBrowser->GetDirectoryPath());
		break;

	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		StartCommandPrompt(m_pActiveShellBrowser->GetDirectoryPath(), LaunchProcessFlags::Elevated);
		break;

	case IDM_FILE_COPYFOLDERPATH:
	{
		BulkClipboardWriter clipboardWriter;
		clipboardWriter.WriteText(m_pActiveShellBrowser->GetDirectoryPath());
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

	case MainToolbarButton::Delete:
	case IDM_FILE_DELETE:
		m_commandController.ExecuteCommand(IDM_FILE_DELETE);
		break;

	case MainToolbarButton::DeletePermanently:
	case IDM_FILE_DELETEPERMANENTLY:
		m_commandController.ExecuteCommand(IDM_FILE_DELETEPERMANENTLY);
		break;

	case IDM_FILE_RENAME:
		OnFileRename();
		break;

	case MainToolbarButton::Properties:
	case IDM_FILE_PROPERTIES:
		m_commandController.ExecuteCommand(IDM_FILE_PROPERTIES);
		break;

	case IDM_FILE_EXIT:
		m_app->TryExit();
		break;

	case IDM_EDIT_UNDO:
		m_FileActionHandler.Undo();
		break;

	case MainToolbarButton::Cut:
	case IDM_EDIT_CUT:
		m_commandController.ExecuteCommand(IDM_EDIT_CUT);
		break;

	case MainToolbarButton::Copy:
	case IDM_EDIT_COPY:
		m_commandController.ExecuteCommand(IDM_EDIT_COPY);
		break;

	case MainToolbarButton::Paste:
	case IDM_EDIT_PASTE:
	case IDM_BACKGROUND_CONTEXT_MENU_PASTE:
		OnPaste();
		break;

	case IDM_EDIT_PASTESHORTCUT:
	case IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT:
		OnPasteShortcut();
		break;

	case IDM_EDIT_PASTEHARDLINK:
		GetActiveShellBrowserImpl()->PasteHardLinks();
		break;

	case IDM_EDIT_PASTE_SYMBOLIC_LINK:
		GetActiveShellBrowserImpl()->PasteSymLinks();
		break;

	case MainToolbarButton::MoveTo:
	case IDM_EDIT_MOVETOFOLDER:
		m_commandController.ExecuteCommand(IDM_EDIT_MOVETOFOLDER);
		break;

	case IDM_EDIT_COPYTOFOLDER:
	case MainToolbarButton::CopyTo:
		m_commandController.ExecuteCommand(IDM_EDIT_COPYTOFOLDER);
		break;

	case IDM_EDIT_SELECTALL:
		ListViewHelper::SelectAllItems(m_hActiveListView, true);
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
		ListViewHelper::SelectAllItems(m_hActiveListView, false);
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

	case IDM_VIEW_DUAL_PANE:
		ToggleDualPane();
		break;

	case IDM_VIEW_STATUSBAR:
		m_config->showStatusBar = !m_config->showStatusBar;
		UpdateLayout();
		break;

	case MainToolbarButton::Folders:
	case IDM_VIEW_FOLDERS:
		ToggleFolders();
		break;

	case IDM_VIEW_DISPLAYWINDOW:
		m_config->showDisplayWindow = !m_config->showDisplayWindow.get();
		break;

	case IDM_DISPLAYWINDOW_VERTICAL:
		m_config->displayWindowVertical = !m_config->displayWindowVertical;
		ApplyDisplayWindowPosition();
		UpdateLayout();
		break;

	case IDM_VIEW_TOOLBARS_ADDRESS_BAR:
	case IDM_VIEW_TOOLBARS_MAIN_TOOLBAR:
	case IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR:
	case IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR:
	case IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR:
	case IDM_VIEW_TOOLBARS_LOCK_TOOLBARS:
	case IDM_VIEW_TOOLBARS_CUSTOMIZE:
		m_commandController.ExecuteCommand(id);
		break;

	case IDM_VIEW_DECREASE_TEXT_SIZE:
		OnChangeMainFontSize(FontSizeType::Decrease);
		break;

	case IDM_VIEW_INCREASE_TEXT_SIZE:
		OnChangeMainFontSize(FontSizeType::Increase);
		break;

	case IDA_RESET_TEXT_SIZE:
		OnResetMainFontSize();
		break;

	case IDM_VIEW_EXTRALARGEICONS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::ExtraLargeIcons);
		break;

	case IDM_VIEW_LARGEICONS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::LargeIcons);
		break;

	case IDM_VIEW_ICONS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::Icons);
		break;

	case IDM_VIEW_SMALLICONS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::SmallIcons);
		break;

	case IDM_VIEW_LIST:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::List);
		break;

	case IDM_VIEW_DETAILS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::Details);
		break;

	case IDM_VIEW_EXTRALARGETHUMBNAILS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::ExtraLargeThumbnails);
		break;

	case IDM_VIEW_LARGETHUMBNAILS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::LargeThumbnails);
		break;

	case IDM_VIEW_THUMBNAILS:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::Thumbnails);
		break;

	case IDM_VIEW_TILES:
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl()->SetViewMode(
			ViewMode::Tiles);
		break;

	case IDM_VIEW_CHANGEDISPLAYCOLOURS:
		OnChangeDisplayColors();
		break;

	case IDM_FILTER_FILTERRESULTS:
		OnFilterResults();
		break;

	case IDM_FILTER_APPLYFILTER:
		m_pActiveShellBrowser->SetFilterApplied(!m_pActiveShellBrowser->IsFilterApplied());
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

	case IDM_GROUP_BY_NONE:
		OnGroupByNone();
		break;

	case IDM_SORT_ASCENDING:
		OnSortDirectionSelected(SortDirection::Ascending);
		break;

	case IDM_SORT_DESCENDING:
		OnSortDirectionSelected(SortDirection::Descending);
		break;

	case IDM_GROUP_SORT_ASCENDING:
		OnGroupSortDirectionSelected(SortDirection::Ascending);
		break;

	case IDM_GROUP_SORT_DESCENDING:
		OnGroupSortDirectionSelected(SortDirection::Descending);
		break;

	case IDM_VIEW_AUTOARRANGE:
		GetActivePane()
			->GetTabContainerImpl()
			->GetSelectedTab()
			.GetShellBrowserImpl()
			->SetAutoArrange(!GetActivePane()
					->GetTabContainerImpl()
					->GetSelectedTab()
					.GetShellBrowserImpl()
					->GetAutoArrange());
		break;

	case IDM_VIEW_SHOWHIDDENFILES:
		OnShowHiddenFiles();
		break;

	case MainToolbarButton::Refresh:
	case IDM_VIEW_REFRESH:
	case IDM_BACKGROUND_CONTEXT_MENU_REFRESH:
		OnRefresh();
		break;

	case IDM_SORTBY_MORE:
	case IDM_VIEW_SELECTCOLUMNS:
		OnSelectColumns();
		break;

	case IDM_VIEW_AUTOSIZECOLUMNS:
		m_pActiveShellBrowser->AutoSizeColumns();
		break;

	case IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT:
	{
		/* Dump the columns from the current tab, and save
		them as the default columns for the appropriate folder
		type.. */
		auto currentColumns = m_pActiveShellBrowser->GetCurrentColumns();
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
		SHGetFolderLocation(nullptr, CSIDL_CONNECTIONS, nullptr, 0,
			wil::out_param(pidlConnections));

		unique_pidl_absolute pidlNetwork;
		SHGetFolderLocation(nullptr, CSIDL_NETWORK, nullptr, 0, wil::out_param(pidlNetwork));

		IShellFolder *pShellFolder;
		SHGetDesktopFolder(&pShellFolder);

		if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlDrives.get()) == 0)
		{
			m_config->globalFolderSettings.folderColumns.myComputerColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlControls.get())
			== 0)
		{
			m_config->globalFolderSettings.folderColumns.controlPanelColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlBitBucket.get())
			== 0)
		{
			m_config->globalFolderSettings.folderColumns.recycleBinColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlPrinters.get())
			== 0)
		{
			m_config->globalFolderSettings.folderColumns.printersColumns = currentColumns;
		}
		else if (pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl.get(), pidlConnections.get())
			== 0)
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

	case MainToolbarButton::NewFolder:
	case IDM_ACTIONS_NEWFOLDER:
		m_commandController.ExecuteCommand(IDM_ACTIONS_NEWFOLDER);
		break;

	case MainToolbarButton::MergeFiles:
	case IDM_ACTIONS_MERGEFILES:
		OnMergeFiles();
		break;

	case MainToolbarButton::SplitFile:
	case IDM_ACTIONS_SPLITFILE:
		OnSplitFile();
		break;

	case IDM_ACTIONS_DESTROYFILES:
		OnDestroyFiles();
		break;

	case MainToolbarButton::Back:
	case IDM_GO_BACK:
		m_commandController.ExecuteCommand(IDM_GO_BACK, DetermineOpenDisposition(false));
		break;

	case MainToolbarButton::Forward:
	case IDM_GO_FORWARD:
		m_commandController.ExecuteCommand(IDM_GO_FORWARD, DetermineOpenDisposition(false));
		break;

	case MainToolbarButton::Up:
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

	case MainToolbarButton::AddBookmark:
	case IDM_BOOKMARKS_BOOKMARKTHISTAB:
		BookmarkHelper::AddBookmarkItem(m_app->GetBookmarkTree(), BookmarkItem::Type::Bookmark,
			nullptr, std::nullopt, hwnd, this, m_app->GetAcceleratorManager(),
			m_app->GetResourceLoader());
		break;

	case IDM_BOOKMARKS_BOOKMARK_ALL_TABS:
		BookmarkHelper::BookmarkAllTabs(m_app->GetBookmarkTree(), m_app->GetResourceLoader(), hwnd,
			this, this, m_app->GetAcceleratorManager());
		break;

	case MainToolbarButton::Bookmarks:
	case IDM_BOOKMARKS_MANAGEBOOKMARKS:
		CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"ManageBookmarksDialog",
			[this, hwnd]
			{
				return new ManageBookmarksDialog(m_app->GetResourceLoader(),
					m_app->GetResourceInstance(), hwnd, this, m_config,
					m_app->GetAcceleratorManager(), &m_iconFetcher, m_app->GetBookmarkTree());
			});
		break;

	case MainToolbarButton::Search:
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

	case IDM_WINDOW_SEARCH_TABS:
		OnSearchTabs();
		break;

	case IDM_HELP_ONLINE_DOCUMENTATION:
		OnOpenOnlineDocumentation();
		break;

	case IDM_HELP_CHECKFORUPDATES:
		OnCheckForUpdates();
		break;

	case IDM_HELP_ABOUT:
		OnAbout();
		break;

	case IDA_NEXTTAB:
		GetActivePane()->GetTabContainerImpl()->SelectAdjacentTab(TRUE);
		break;

	case IDA_PREVIOUSTAB:
		GetActivePane()->GetTabContainerImpl()->SelectAdjacentTab(FALSE);
		break;

	case IDA_ADDRESSBAR:
		SetFocus(m_addressBar->GetView()->GetHWND());
		break;

	case IDA_COMBODROPDOWN:
		SetFocus(m_addressBar->GetView()->GetHWND());
		SendMessage(m_addressBar->GetView()->GetHWND(), CB_SHOWDROPDOWN, TRUE, 0);
		break;

	case IDA_PREVIOUSWINDOW:
		OnFocusNextWindow(FocusChangeDirection::Previous);
		break;

	case IDA_NEXTWINDOW:
		OnFocusNextWindow(FocusChangeDirection::Next);
		break;

	case IDA_TAB_DUPLICATETAB:
		GetActivePane()->GetTabContainerImpl()->DuplicateTab(
			GetActivePane()->GetTabContainerImpl()->GetSelectedTab());
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
		m_app->GetTabRestorer()->RestoreLastTab();
		break;

	case MainToolbarButton::Views:
		OnToolbarViews();
		break;

		/* Display window menus. */
	case IDM_DW_HIDEDISPLAYWINDOW:
		m_config->showDisplayWindow = false;
		break;

	case IDM_BACKGROUND_CONTEXT_MENU_CUSTOMIZE:
		// Note that the call below won't always result in the customize tab being selected. That's
		// because the properties dialog will select the tab based on its display name, which can
		// change as the display language is changed in Windows.
		// In Explorer, the title of the dialog is dynamically retrieved. Although it might be
		// possible to do that here as well, that strategy would break if the customize dialog
		// resource ID ever changed.
		// Another alternative might be to load the "customize" string from the string table. But
		// the language used by the application has nothing to do with the language used by Windows
		// itself. Also, the text would have to be exactly the same as that used by Windows for a
		// given language, which probably wouldn't be clear to translators. Minor variations within
		// a language (e.g. customize vs customise) could cause the tab to not be selected.
		// Therefore, this will only work when the actual title of the properties dialog is
		// "customize" (ignoring case). That's not ideal, but not too much of an issue, since the
		// properties dialog will always be opened, just not always on the customize tab.
		ExecuteFileAction(m_hContainer, GetActiveShellBrowserImpl()->GetDirectoryIdl().get(),
			L"properties", L"customize", L"");
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
	case NM_CLICK:
		if (nmhdr->hwndFrom == m_hActiveListView)
		{
			OnListViewClick(reinterpret_cast<NMITEMACTIVATE *>(lParam));
		}
		break;

	case NM_DBLCLK:
		if (nmhdr->hwndFrom == m_hActiveListView)
		{
			OnListViewDoubleClick(reinterpret_cast<NMITEMACTIVATE *>(lParam));
		}
		break;

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
		ScheduleUpdateLayout(m_weakPtrFactory.GetWeakPtr(), m_app->GetRuntime());
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

		pnmrc = (NMREBARCHEVRON *) lParam;

		POINT ptMenu;
		ptMenu.x = pnmrc->rc.left;
		ptMenu.y = pnmrc->rc.bottom;
		ClientToScreen(m_mainRebarView->GetHWND(), &ptMenu);

		if (pnmrc->wID == REBAR_BAND_ID_BOOKMARKS_TOOLBAR)
		{
			m_bookmarksToolbar->ShowOverflowMenu(ptMenu);
			return 0;
		}

		hMenu = CreatePopupMenu();

		HIMAGELIST himlMenu = nullptr;

		Shell_GetImageLists(nullptr, &himlSmall);

		switch (pnmrc->wID)
		{
		case REBAR_BAND_ID_MAIN_TOOLBAR:
			hToolbar = m_mainToolbar->GetHWND();
			break;

		case REBAR_BAND_ID_DRIVES_TOOLBAR:
			hToolbar = m_drivesToolbar->GetView()->GetHWND();
			himlMenu = himlSmall;
			break;

		case REBAR_BAND_ID_APPLICATIONS_TOOLBAR:
			hToolbar = m_applicationToolbar->GetView()->GetHWND();
			himlMenu = himlSmall;
			break;
		}

		nButtons = (int) SendMessage(hToolbar, TB_BUTTONCOUNT, 0, 0);

		GetClientRect(hToolbar, &rcToolbar);

		for (i = 0; i < nButtons; i++)
		{
			lResult = SendMessage(hToolbar, TB_GETITEMRECT, i, (LPARAM) &rcButton);

			if (lResult)
			{
				bIntersect = IntersectRect(&rcIntersect, &rcToolbar, &rcButton);

				if (!bIntersect || rcButton.right > rcToolbar.right)
				{
					SendMessage(hToolbar, TB_GETBUTTON, i, (LPARAM) &tbButton);

					if (tbButton.fsStyle & BTNS_SEP)
					{
						mii.cbSize = sizeof(mii);
						mii.fMask = MIIM_FTYPE;
						mii.fType = MFT_SEPARATOR;
						InsertMenuItem(hMenu, i, TRUE, &mii);
					}
					else
					{
						if (IS_INTRESOURCE(tbButton.iString))
						{
							SendMessage(hToolbar, TB_GETSTRING,
								MAKEWPARAM(std::size(szText), tbButton.iString), (LPARAM) szText);
						}
						else
						{
							StringCchCopy(szText, std::size(szText), (LPCWSTR) tbButton.iString);
						}

						HMENU hSubMenu = nullptr;
						UINT fMask;

						fMask = MIIM_ID | MIIM_STRING;
						hSubMenu = nullptr;

						switch (pnmrc->wID)
						{
						case REBAR_BAND_ID_MAIN_TOOLBAR:
						{
							switch (tbButton.idCommand)
							{
							case MainToolbarButton::Back:
								hSubMenu = CreateRebarHistoryMenu(TRUE);
								fMask |= MIIM_SUBMENU;
								break;

							case MainToolbarButton::Forward:
								hSubMenu = CreateRebarHistoryMenu(FALSE);
								fMask |= MIIM_SUBMENU;
								break;

							case MainToolbarButton::Views:
							{
								auto viewsMenu = BuildViewsMenu();

								// The submenu will be destroyed when the parent menu is
								// destroyed.
								hSubMenu = viewsMenu.release();

								fMask |= MIIM_SUBMENU;
							}
							break;
							}
						}
						break;
						}

						mii.cbSize = sizeof(mii);
						mii.fMask = fMask;
						mii.wID = tbButton.idCommand;
						mii.hSubMenu = hSubMenu;
						mii.dwTypeData = szText;
						InsertMenuItem(hMenu, iMenu, TRUE, &mii);

						/* TODO: Update the image
						for this menu item. */
					}
					iMenu++;
				}
			}
		}

		UINT uFlags = TPM_LEFTALIGN | TPM_RETURNCMD;
		int iCmd;

		iCmd = TrackPopupMenu(hMenu, uFlags, ptMenu.x, ptMenu.y, 0, m_mainRebarView->GetHWND(),
			nullptr);

		if (iCmd != 0)
		{
			/* We'll handle the back and forward buttons
			in place, and send the rest of the messages
			back to the main window. */
			if ((iCmd >= ID_REBAR_MENU_BACK_START && iCmd <= ID_REBAR_MENU_BACK_END)
				|| (iCmd >= ID_REBAR_MENU_FORWARD_START && iCmd <= ID_REBAR_MENU_FORWARD_END))
			{
				if (iCmd >= ID_REBAR_MENU_BACK_START && iCmd <= ID_REBAR_MENU_BACK_END)
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
				SendMessage(m_hContainer, WM_COMMAND, MAKEWPARAM(iCmd, 0), 0);
			}
		}

		DestroyMenu(hMenu);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
