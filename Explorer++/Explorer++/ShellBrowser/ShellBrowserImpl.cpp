// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "BrowserWindow.h"
#include "ClipboardOperations.h"
#include "ColorRuleModel.h"
#include "ColumnDataRetrieval.h"
#include "Config.h"
#include "DialogHelper.h"
#include "DirectoryOperationsHelper.h"
#include "FileOperations.h"
#include "FileProgressSink.h"
#include "FolderView.h"
#include "IconFetcherImpl.h"
#include "ItemData.h"
#include "MainResource.h"
#include "MassRenameDialog.h"
#include "MergeFilesDialog.h"
#include "PreservedShellBrowser.h"
#include "ResourceLoader.h"
#include "ServiceProvider.h"
#include "ShellEnumeratorImpl.h"
#include "ShellNavigationController.h"
#include "SortModes.h"
#include "SplitFileDialog.h"
#include "ThemeManager.h"
#include "ViewModeHelper.h"
#include "ViewModes.h"
#include "WildcardSelectDialog.h"
#include "../Helper/Controls.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileDialogs.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <list>

ShellBrowserImpl::ShellBrowserImpl(HWND owner, App *app, BrowserWindow *browser,
	FileActionHandler *fileActionHandler, const PreservedShellBrowser &preservedShellBrowser) :
	ShellBrowserImpl(owner, app, browser, fileActionHandler, preservedShellBrowser.folderSettings,
		&preservedShellBrowser.folderColumns)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, browser,
		&m_navigationManager, m_app->GetNavigationEvents(), preservedShellBrowser.history,
		preservedShellBrowser.currentEntry);

	ChangeToInitialFolder();
}

ShellBrowserImpl::ShellBrowserImpl(HWND owner, App *app, BrowserWindow *browser,
	FileActionHandler *fileActionHandler, const PidlAbsolute &initialPidl,
	const FolderSettings &folderSettings, const FolderColumns *initialColumns) :
	ShellBrowserImpl(owner, app, browser, fileActionHandler, folderSettings, initialColumns)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, browser,
		&m_navigationManager, m_app->GetNavigationEvents(), initialPidl);

	ChangeToInitialFolder();
}

ShellBrowserImpl::ShellBrowserImpl(HWND owner, App *app, BrowserWindow *browser,
	FileActionHandler *fileActionHandler, const FolderSettings &folderSettings,
	const FolderColumns *initialColumns) :
	ShellDropTargetWindow(CreateListView(owner)),
	m_listView(GetHWND()),
	m_owner(owner),
	m_app(app),
	m_browser(browser),
	m_shellEnumerator(std::make_shared<ShellEnumeratorImpl>(owner,
		folderSettings.showHidden ? ShellEnumeratorImpl::HiddenItemsPolicy::IncludeHidden
								  : ShellEnumeratorImpl::HiddenItemsPolicy::ExcludeHidden)),
	m_navigationManager(this, app->GetNavigationEvents(), m_shellEnumerator,
		app->GetFeatureList()->IsEnabled(Feature::BackgroundThreadEnumeration)
			? app->GetRuntime()->GetComStaExecutor()
			: app->GetRuntime()->GetInlineExecutor(),
		app->GetFeatureList()->IsEnabled(Feature::BackgroundThreadEnumeration)
			? app->GetRuntime()->GetUiThreadExecutor()
			: app->GetRuntime()->GetInlineExecutor()),
	m_progressCursor(LoadCursor(nullptr, IDC_APPSTARTING)),
	m_commandTarget(browser->GetCommandTargetManager(), this),
	m_fileActionHandler(fileActionHandler),
	m_fontSetter(GetHWND(), app->GetConfig()),
	m_tooltipFontSetter(reinterpret_cast<HWND>(SendMessage(GetHWND(), LVM_GETTOOLTIPS, 0, 0)),
		app->GetConfig()),
	m_columnThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_columnResultIDCounter(0),
	m_cachedIcons(app->GetCachedIcons()),
	m_thumbnailThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_thumbnailResultIDCounter(0),
	m_infoTipsThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_infoTipResultIDCounter(0),
	m_resourceInstance(app->GetResourceInstance()),
	m_acceleratorManager(app->GetAcceleratorManager()),
	m_config(app->GetConfig()),
	m_folderSettings(folderSettings),
	m_shellChangeWatcher(GetHWND(),
		std::bind_front(&ShellBrowserImpl::ProcessShellChangeNotifications, this)),
	m_shellWindowRegistered(false),
	m_folderColumns(
		initialColumns ? *initialColumns : app->GetConfig()->globalFolderSettings.folderColumns),
	m_draggedDataObject(nullptr),
	m_weakPtrFactory(this)
{
	InitializeListView();
	m_iconFetcher = std::make_unique<IconFetcherImpl>(m_listView, m_cachedIcons);

	m_connections.push_back(m_app->GetNavigationEvents()->AddStartedObserver(
		std::bind_front(&ShellBrowserImpl::OnNavigationStarted, this),
		NavigationEventScope::ForShellBrowser(*this)));
	m_connections.push_back(m_app->GetNavigationEvents()->AddWillCommitObserver(
		std::bind_front(&ShellBrowserImpl::OnNavigationWillCommit, this),
		NavigationEventScope::ForShellBrowser(*this), boost::signals2::at_front,
		SlotGroup::HighPriority));
	m_connections.push_back(m_app->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&ShellBrowserImpl::OnNavigationComitted, this),
		NavigationEventScope::ForShellBrowser(*this), boost::signals2::at_front,
		SlotGroup::HighPriority));

	m_connections.push_back(m_app->GetClipboardWatcher()->updateSignal.AddObserver(
		std::bind_front(&ShellBrowserImpl::OnClipboardUpdate, this)));

	m_getDragImageMessage = RegisterWindowMessage(DI_GETDRAGIMAGE);

	m_performingDrag = false;
	m_nCurrentColumns = 0;
	m_pActiveColumns = nullptr;
	m_nActiveColumns = 0;
	m_middleButtonItem = -1;

	m_uniqueFolderId = 0;

	m_PreviousSortColumnExists = false;

	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_iFolderIcon));
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_iFileIcon));

	m_shellWindows = winrt::try_create_instance<IShellWindows>(CLSID_ShellWindows, CLSCTX_ALL);
}

ShellBrowserImpl::~ShellBrowserImpl()
{
	m_destroyedSignal();

	if (m_clipboardDataObject
		&& m_app->GetClipboardStore()->IsDataObjectCurrent(m_clipboardDataObject.get()))
	{
		// Ensure that any data that was copied to the clipboard remains there. Technically, this
		// only needs to be done when the application is closed. However, determining whether the
		// application set the current data on the clipboard when the tab that set the data has
		// already been closed is difficult, so the easiest thing to do is just flush the clipboard
		// here.
		m_app->GetClipboardStore()->FlushDataObject();
	}

	DestroyWindow(m_listView);

	m_columnThreadPool.clear_queue();
	m_thumbnailThreadPool.clear_queue();
	m_infoTipsThreadPool.clear_queue();
}

HWND ShellBrowserImpl::CreateListView(HWND parent)
{
	// Note that the only reason LVS_REPORT is specified here is so that the listview header theme
	// can be set immediately when in dark mode. Without this style, ListView_GetHeader() will
	// return NULL. The actual view mode set here doesn't matter, since it will be updated when
	// navigating to a folder.
	return ::CreateListView(parent,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_EDITLABELS
			| LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE | WS_TABSTOP
			| LVS_ALIGNTOP);
}

void ShellBrowserImpl::InitializeListView()
{
	auto dwExtendedStyle = ListView_GetExtendedListViewStyle(m_listView);

	if (m_config->useFullRowSelect.get())
	{
		dwExtendedStyle |= LVS_EX_FULLROWSELECT;
	}

	m_connections.push_back(m_config->useFullRowSelect.addObserver(
		std::bind_front(&ShellBrowserImpl::OnFullRowSelectUpdated, this)));

	if (m_config->checkBoxSelection.get())
	{
		dwExtendedStyle |= LVS_EX_CHECKBOXES;
	}

	m_connections.push_back(m_config->checkBoxSelection.addObserver(
		std::bind_front(&ShellBrowserImpl::OnCheckBoxSelectionUpdated, this)));

	ListView_SetExtendedListViewStyle(m_listView, dwExtendedStyle);

	ListViewHelper::SetAutoArrange(m_listView, m_folderSettings.autoArrangeEnabled);
	ListViewHelper::AddRemoveExtendedStyles(m_listView, LVS_EX_GRIDLINES,
		m_config->globalFolderSettings.showGridlines.get());

	m_connections.push_back(m_config->globalFolderSettings.showGridlines.addObserver(
		std::bind_front(&ShellBrowserImpl::OnShowGridlinesUpdated, this)));

	ListViewHelper::ActivateOneClickSelect(m_listView,
		m_config->globalFolderSettings.oneClickActivate.get(),
		m_config->globalFolderSettings.oneClickActivateHoverTime.get());

	m_connections.push_back(m_config->globalFolderSettings.oneClickActivate.addObserver(
		std::bind_front(&ShellBrowserImpl::OnOneClickActivateUpdated, this)));
	m_connections.push_back(m_config->globalFolderSettings.oneClickActivateHoverTime.addObserver(
		std::bind_front(&ShellBrowserImpl::OnOneClickActivateHoverTimeUpdated, this)));

	m_app->GetThemeManager()->ApplyThemeToWindowAndChildren(m_listView);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_listView,
		std::bind_front(&ShellBrowserImpl::ListViewProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(m_listView),
		std::bind_front(&ShellBrowserImpl::ListViewParentProc, this)));

	m_connections.push_back(m_app->GetColorRuleModel()->AddItemAddedObserver(
		std::bind(&ShellBrowserImpl::OnColorRulesUpdated, this)));
	m_connections.push_back(m_app->GetColorRuleModel()->AddItemUpdatedObserver(
		std::bind(&ShellBrowserImpl::OnColorRulesUpdated, this)));
	m_connections.push_back(m_app->GetColorRuleModel()->AddItemMovedObserver(
		std::bind(&ShellBrowserImpl::OnColorRulesUpdated, this)));
	m_connections.push_back(m_app->GetColorRuleModel()->AddItemRemovedObserver(
		std::bind(&ShellBrowserImpl::OnColorRulesUpdated, this)));
	m_connections.push_back(m_app->GetColorRuleModel()->AddAllItemsRemovedObserver(
		std::bind(&ShellBrowserImpl::OnColorRulesUpdated, this)));

	if (m_folderSettings.showInGroups)
	{
		ListView_EnableGroupView(m_listView, true);
	}
}

void ShellBrowserImpl::ChangeToInitialFolder()
{
	// This class always needs to represent a folder. Therefore, it's necessary to immediately
	// switch to a folder, in this case, the folder represented by the current entry. Normally, this
	// is only done once a navigation commits.
	auto *currentEntry = m_navigationController->GetCurrentEntry();
	ChangeFolders(currentEntry->GetPidl());
}

bool ShellBrowserImpl::IsAutoArrangeEnabled() const
{
	return m_folderSettings.autoArrangeEnabled;
}

void ShellBrowserImpl::SetAutoArrangeEnabled(bool enabled)
{
	m_folderSettings.autoArrangeEnabled = enabled;

	ListViewHelper::SetAutoArrange(m_listView, m_folderSettings.autoArrangeEnabled);
}

ViewMode ShellBrowserImpl::GetViewMode() const
{
	return m_folderSettings.viewMode;
}

/* This function is only called on 'hard' view changes
(i.e. view changes resulting from user requests). It is
not called when a tab is first set up (in which case
the view mode still needs to be setup), or when entering
a folder. */
void ShellBrowserImpl::SetViewMode(ViewMode viewMode)
{
	if (viewMode == m_folderSettings.viewMode)
	{
		return;
	}

	// The view mode is being changed from thumbnails view to either another thumbnails view, or a
	// non-thumbnails view. In both cases, the existing thumbnails view needs to be removed.
	if (IsThumbnailsViewMode(m_folderSettings.viewMode))
	{
		RemoveThumbnailsView();
	}

	SetViewModeInternal(viewMode);

	switch (viewMode)
	{
	case ViewMode::Tiles:
		SetTileViewInfo();
		break;

	case ViewMode::Icons:
	case ViewMode::SmallIcons:
	case ViewMode::List:
	case ViewMode::Details:
	case ViewMode::Thumbnails:
	case ViewMode::ExtraLargeIcons:
	case ViewMode::LargeIcons:
	case ViewMode::ExtraLargeThumbnails:
	case ViewMode::LargeThumbnails:
		break;
	}
}

/* Explicitly sets the view mode within in the listview.
This function also initializes any items needed to support
the current view mode. This MUST be done within this
function, as when a tab is first opened, the view settings
will need to be initialized. */
void ShellBrowserImpl::SetViewModeInternal(ViewMode viewMode)
{
	DWORD dwStyle;

	ListView_SetImageList(m_listView, nullptr, LVSIL_SMALL);
	ListView_SetImageList(m_listView, nullptr, LVSIL_NORMAL);

	switch (viewMode)
	{
	case ViewMode::ExtraLargeIcons:
	{
		wil::com_ptr_nothrow<IImageList> pImageList;
		SHGetImageList(SHIL_JUMBO, IID_PPV_ARGS(&pImageList));
		ListView_SetImageList(m_listView, reinterpret_cast<HIMAGELIST>(pImageList.get()),
			LVSIL_NORMAL);
	}
	break;

	case ViewMode::LargeIcons:
	{
		wil::com_ptr_nothrow<IImageList> pImageList;
		SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&pImageList));
		ListView_SetImageList(m_listView, reinterpret_cast<HIMAGELIST>(pImageList.get()),
			LVSIL_NORMAL);
	}
	break;

	/* Do nothing. The image list will be set up below. */
	case ViewMode::ExtraLargeThumbnails:
	case ViewMode::LargeThumbnails:
	case ViewMode::Thumbnails:
		break;

	case ViewMode::Tiles:
	case ViewMode::Icons:
	{
		wil::com_ptr_nothrow<IImageList> pImageList;
		SHGetImageList(SHIL_LARGE, IID_PPV_ARGS(&pImageList));
		ListView_SetImageList(m_listView, reinterpret_cast<HIMAGELIST>(pImageList.get()),
			LVSIL_NORMAL);
	}
	break;

	case ViewMode::SmallIcons:
	case ViewMode::List:
	case ViewMode::Details:
	{
		wil::com_ptr_nothrow<IImageList> pImageList;
		SHGetImageList(SHIL_SMALL, IID_PPV_ARGS(&pImageList));
		ListView_SetImageList(m_listView, reinterpret_cast<HIMAGELIST>(pImageList.get()),
			LVSIL_SMALL);
	}
	break;
	}

	DeleteAllColumns();

	switch (viewMode)
	{
	case ViewMode::Tiles:
		dwStyle = LV_VIEW_TILE;

		InsertTileViewColumns();
		break;

	case ViewMode::ExtraLargeIcons:
	case ViewMode::LargeIcons:
	case ViewMode::Icons:
		dwStyle = LV_VIEW_ICON;
		break;

	case ViewMode::SmallIcons:
		dwStyle = LV_VIEW_SMALLICON;
		break;

	case ViewMode::List:
		dwStyle = LV_VIEW_LIST;
		break;

	case ViewMode::Details:
		dwStyle = LV_VIEW_DETAILS;

		SetUpListViewColumns();
		ApplyHeaderSortArrow();
		break;

	case ViewMode::ExtraLargeThumbnails:
		dwStyle = LV_VIEW_ICON;
		m_thumbnailItemWidth = 256;
		m_thumbnailItemHeight = 256;
		SetupThumbnailsView(SHIL_JUMBO);
		break;

	case ViewMode::LargeThumbnails:
		dwStyle = LV_VIEW_ICON;
		m_thumbnailItemWidth = 128;
		m_thumbnailItemHeight = 128;
		SetupThumbnailsView(SHIL_EXTRALARGE);
		break;

	case ViewMode::Thumbnails:
		dwStyle = LV_VIEW_ICON;
		m_thumbnailItemWidth = 64;
		m_thumbnailItemHeight = 64;
		SetupThumbnailsView(SHIL_LARGE);
		break;

	default:
		dwStyle = LV_VIEW_ICON;
		viewMode = ViewMode::Icons;
		break;
	}

	if (viewMode != +ViewMode::Details)
	{
		m_columnThreadPool.clear_queue();
		m_columnResults.clear();
	}

	if (viewMode != +ViewMode::Details && viewMode != +ViewMode::Tiles)
	{
		AddFirstColumn();
	}

	ViewMode previousViewMode = m_folderSettings.viewMode;
	m_folderSettings.viewMode = viewMode;

	SendMessage(m_listView, LVM_SETVIEW, dwStyle, 0);

	if (previousViewMode != +ViewMode::Details && viewMode == +ViewMode::Details)
	{
		auto firstColumn = GetFirstCheckedColumn();

		if (firstColumn.type != +ColumnType::Name)
		{
			SetFirstColumnTextToCallback();
		}
	}
	else if (previousViewMode == +ViewMode::Details && viewMode != +ViewMode::Details)
	{
		auto firstColumn = GetFirstCheckedColumn();

		// The item text in non-details view is always the filename.
		if (firstColumn.type != +ColumnType::Name)
		{
			SetFirstColumnTextToFilename();
		}
	}
}

void ShellBrowserImpl::SetFirstColumnTextToCallback()
{
	int numItems = ListView_GetItemCount(m_listView);

	for (int i = 0; i < numItems; i++)
	{
		ListView_SetItemText(m_listView, i, 0, LPSTR_TEXTCALLBACK);
	}
}

void ShellBrowserImpl::SetFirstColumnTextToFilename()
{
	int numItems = ListView_GetItemCount(m_listView);

	for (int i = 0; i < numItems; i++)
	{
		int internalIndex = GetItemInternalIndex(i);

		BasicItemInfo_t basicItemInfo = getBasicItemInfo(internalIndex);
		std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);

		ListView_SetItemText(m_listView, i, 0, filename.data());
	}
}

void ShellBrowserImpl::CycleViewMode(bool cycleForward)
{
	ViewMode newViewMode;

	if (cycleForward)
	{
		newViewMode = GetNextViewMode(VIEW_MODES, m_folderSettings.viewMode);
	}
	else
	{
		newViewMode = GetPreviousViewMode(VIEW_MODES, m_folderSettings.viewMode);
	}

	SetViewMode(newViewMode);
}

SortMode ShellBrowserImpl::GetSortMode() const
{
	return m_folderSettings.sortMode;
}

void ShellBrowserImpl::SetSortMode(SortMode sortMode)
{
	if (sortMode == m_folderSettings.sortMode)
	{
		return;
	}

	m_folderSettings.sortMode = sortMode;

	SortFolder();
}

SortMode ShellBrowserImpl::GetGroupMode() const
{
	return m_folderSettings.groupMode;
}

void ShellBrowserImpl::SetGroupMode(SortMode sortMode)
{
	m_folderSettings.groupMode = sortMode;

	if (m_folderSettings.showInGroups)
	{
		MoveItemsIntoGroups();
	}
}

SortDirection ShellBrowserImpl::GetSortDirection() const
{
	return m_folderSettings.sortDirection;
}

void ShellBrowserImpl::SetSortDirection(SortDirection direction)
{
	if (direction == m_folderSettings.sortDirection)
	{
		return;
	}

	m_folderSettings.sortDirection = direction;

	SortFolder();
}

SortDirection ShellBrowserImpl::GetGroupSortDirection() const
{
	return m_folderSettings.groupSortDirection;
}

void ShellBrowserImpl::SetGroupSortDirection(SortDirection direction)
{
	if (direction == m_folderSettings.groupSortDirection)
	{
		return;
	}

	m_folderSettings.groupSortDirection = direction;

	ListView_SortGroups(m_listView, GroupComparisonStub, this);
}

std::wstring ShellBrowserImpl::GetItemName(int index) const
{
	return GetItemByIndex(index).wfd.cFileName;
}

// Returns the name of the item as it's shown to the user. Note that this name may not be unique.
// For example, if file extensions are hidden, then two or more items might share the same display
// name (because they only differ in their extensions, which are hidden). Because of this, the
// display name shouldn't be used to perform item lookups.
std::wstring ShellBrowserImpl::GetItemDisplayName(int index) const
{
	// Although the display name for an item is retrieved and cached, that might not be exactly the
	// same as the text that's displayed. For example, if extensions are shown within Explorer, but
	// hidden within Explorer++, then the display name will contain the file extension, but the text
	// displayed to the user won't. Processing the filename here ensures that the extension is
	// removed, if necessary.
	BasicItemInfo_t basicItemInfo = getBasicItemInfo(GetItemInternalIndex(index));
	return ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);
}

std::wstring ShellBrowserImpl::GetItemFullName(int index) const
{
	return GetItemByIndex(index).parsingName;
}

std::wstring ShellBrowserImpl::GetDirectoryPath() const
{
	return m_directoryState.directory;
}

unique_pidl_absolute ShellBrowserImpl::GetDirectoryIdl() const
{
	unique_pidl_absolute pidlDirectory(ILCloneFull(m_directoryState.pidlDirectory.Raw()));
	return pidlDirectory;
}

void ShellBrowserImpl::SelectItems(const std::vector<PidlAbsolute> &pidls)
{
	ListViewHelper::SelectAllItems(m_listView, false);

	int smallestIndex = INT_MAX;

	for (auto &pidl : pidls)
	{
		auto index = GetItemIndexForPidl(pidl.Raw());

		if (!index)
		{
			m_directoryState.filesToSelect.emplace_back(pidl);
			continue;
		}

		ListViewHelper::SelectItem(m_listView, *index, true);

		if (*index < smallestIndex)
		{
			smallestIndex = *index;
		}
	}

	if (smallestIndex != INT_MAX)
	{
		ListViewHelper::FocusItem(m_listView, smallestIndex, true);
		ListView_EnsureVisible(m_listView, smallestIndex, FALSE);
	}
}

int ShellBrowserImpl::LocateFileItemIndex(const TCHAR *szFileName) const
{
	LV_FINDINFO lvFind;
	int iItem;
	int iInternalIndex;

	iInternalIndex = LocateFileItemInternalIndex(szFileName);

	if (iInternalIndex != -1)
	{
		lvFind.flags = LVFI_PARAM;
		lvFind.lParam = iInternalIndex;
		iItem = ListView_FindItem(m_listView, -1, &lvFind);

		return iItem;
	}

	return -1;
}

int ShellBrowserImpl::LocateFileItemInternalIndex(const TCHAR *szFileName) const
{
	for (int i = 0; i < m_directoryState.numItems; i++)
	{
		const auto &item = GetItemByIndex(i);

		if (lstrcmp(item.wfd.cFileName, szFileName) == 0)
		{
			return GetItemInternalIndex(i);
		}
	}

	return -1;
}

std::optional<int> ShellBrowserImpl::GetItemIndexForPidl(PCIDLIST_ABSOLUTE pidl) const
{
	auto internalIndex = GetItemInternalIndexForPidl(pidl);

	if (!internalIndex)
	{
		return std::nullopt;
	}

	return LocateItemByInternalIndex(*internalIndex);
}

std::optional<int> ShellBrowserImpl::GetItemInternalIndexForPidl(PCIDLIST_ABSOLUTE pidl) const
{
	auto itr = std::find_if(m_itemInfoMap.begin(), m_itemInfoMap.end(), [pidl](const auto &pair)
		{ return ArePidlsEquivalent(pidl, pair.second.pidlComplete.Raw()); });

	if (itr == m_itemInfoMap.end())
	{
		return std::nullopt;
	}

	return itr->first;
}

std::optional<int> ShellBrowserImpl::LocateItemByInternalIndex(int internalIndex) const
{
	LVFINDINFO lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = internalIndex;
	int item = ListView_FindItem(m_listView, -1, &lvfi);

	if (item == -1)
	{
		return std::nullopt;
	}

	return item;
}

WIN32_FIND_DATA ShellBrowserImpl::GetItemFileFindData(int index) const
{
	return GetItemByIndex(index).wfd;
}

unique_pidl_absolute ShellBrowserImpl::GetItemCompleteIdl(int index) const
{
	return unique_pidl_absolute(ILCloneFull(GetItemByIndex(index).pidlComplete.Raw()));
}

unique_pidl_child ShellBrowserImpl::GetItemChildIdl(int index) const
{
	return unique_pidl_child(ILCloneChild(GetItemByIndex(index).pridl.Raw()));
}

bool ShellBrowserImpl::InVirtualFolder() const
{
	return m_directoryState.virtualFolder;
}

BOOL ShellBrowserImpl::CompareVirtualFolders(UINT uFolderCSIDL) const
{
	std::wstring parsingPath;
	GetCsidlDisplayName(uFolderCSIDL, SHGDN_FORPARSING, parsingPath);

	if (parsingPath == m_directoryState.directory)
	{
		return TRUE;
	}

	return FALSE;
}

int ShellBrowserImpl::GenerateUniqueItemId()
{
	return m_directoryState.itemIDCounter++;
}

int ShellBrowserImpl::DetermineItemSortedPosition(LPARAM lParam) const
{
	LVITEM lvItem;
	BOOL bItem;
	int res = 1;
	int nItems = 0;
	int i = 0;

	nItems = ListView_GetItemCount(m_listView);

	while (res > 0 && i < nItems)
	{
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		bItem = ListView_GetItem(m_listView, &lvItem);

		if (bItem)
		{
			res = Sort(static_cast<int>(lParam), static_cast<int>(lvItem.lParam));
		}
		else
		{
			res = 0;
		}

		i++;
	}

	/* The item will always be inserted BEFORE
	the item at position i that we specify here.
	For example, specifying 0, will place the item
	at 0 (and push 0 to 1), specifying 1 will place
	the item at 1 (and push 1 to 2).
	Therefore, to place in the last position, need
	to place AFTER last item. */
	if ((i - 1) == nItems - 1 && res > 0)
	{
		i++;
	}

	return i - 1;
}

int ShellBrowserImpl::GetNumItems() const
{
	return m_directoryState.numItems;
}

int ShellBrowserImpl::GetNumSelectedFiles() const
{
	return m_directoryState.numFilesSelected;
}

int ShellBrowserImpl::GetNumSelectedFolders() const
{
	return m_directoryState.numFoldersSelected;
}

int ShellBrowserImpl::GetNumSelected() const
{
	return m_directoryState.numFilesSelected + m_directoryState.numFoldersSelected;
}

// Returns the total size of the items in the current directory (not including any sub-directories).
uint64_t ShellBrowserImpl::GetTotalDirectorySize()
{
	return m_directoryState.totalDirSize;
}

// Returns the size of the currently selected items.
uint64_t ShellBrowserImpl::GetSelectionSize()
{
	return m_directoryState.fileSelectionSize;
}

void ShellBrowserImpl::VerifySortMode()
{
	const std::vector<Column_t> *columns = nullptr;

	if (CompareVirtualFolders(CSIDL_CONTROLS))
	{
		columns = &m_folderColumns.controlPanelColumns;
	}
	else if (CompareVirtualFolders(CSIDL_DRIVES))
	{
		columns = &m_folderColumns.myComputerColumns;
	}
	else if (CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		columns = &m_folderColumns.recycleBinColumns;
	}
	else if (CompareVirtualFolders(CSIDL_PRINTERS))
	{
		columns = &m_folderColumns.printersColumns;
	}
	else if (CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		columns = &m_folderColumns.networkConnectionsColumns;
	}
	else if (CompareVirtualFolders(CSIDL_NETWORK))
	{
		columns = &m_folderColumns.myNetworkPlacesColumns;
	}
	else
	{
		columns = &m_folderColumns.realFolderColumns;
	}

	auto firstChecked = GetFirstCheckedColumn();

	auto itr = std::find_if(columns->begin(), columns->end(), [this](const Column_t &column)
		{ return DetermineColumnSortMode(column.type) == m_folderSettings.sortMode; });

	if (itr == columns->end())
	{
		m_folderSettings.sortMode = DetermineColumnSortMode(firstChecked.type);
	}

	itr = std::find_if(columns->begin(), columns->end(), [this](const Column_t &column)
		{ return DetermineColumnSortMode(column.type) == m_folderSettings.groupMode; });

	if (itr == columns->end())
	{
		m_folderSettings.groupMode = DetermineColumnSortMode(firstChecked.type);
	}
}

bool ShellBrowserImpl::GetShowHidden() const
{
	return m_folderSettings.showHidden;
}

void ShellBrowserImpl::SetShowHidden(bool showHidden)
{
	m_folderSettings.showHidden = showHidden;

	m_shellEnumerator->SetHiddenItemsPolicy(showHidden
			? ShellEnumeratorImpl::HiddenItemsPolicy::IncludeHidden
			: ShellEnumeratorImpl::HiddenItemsPolicy::ExcludeHidden);
}

std::vector<SortMode> ShellBrowserImpl::GetAvailableSortModes() const
{
	std::vector<SortMode> sortModes;

	for (const auto &column : *m_pActiveColumns)
	{
		if (column.checked)
		{
			sortModes.push_back(DetermineColumnSortMode(column.type));
		}
	}

	return sortModes;
}

/* Queues an in-place rename for an item.
This method is used when a file is created
using the shell new menu, and the item
may or may not have been inserted into
the listview yet. */
void ShellBrowserImpl::QueueRename(PCIDLIST_ABSOLUTE pidlItem)
{
	auto index = MaybeGetItemIndex(pidlItem);

	if (!index)
	{
		m_directoryState.queuedRenameItem = pidlItem;
		return;
	}

	ListView_EditLabel(m_listView, *index);
}

int ShellBrowserImpl::GetUniqueFolderId() const
{
	return m_uniqueFolderId;
}

BasicItemInfo_t ShellBrowserImpl::getBasicItemInfo(int internalIndex) const
{
	const ItemInfo_t &itemInfo = m_itemInfoMap.at(internalIndex);

	BasicItemInfo_t basicItemInfo;
	basicItemInfo.pidlComplete.reset(ILCloneFull(itemInfo.pidlComplete.Raw()));
	basicItemInfo.pridl.reset(ILCloneChild(itemInfo.pridl.Raw()));
	basicItemInfo.wfd = itemInfo.wfd;
	basicItemInfo.isFindDataValid = itemInfo.isFindDataValid;
	StringCchCopy(basicItemInfo.szDisplayName, std::size(basicItemInfo.szDisplayName),
		itemInfo.displayName.c_str());
	basicItemInfo.isRoot = itemInfo.bDrive;

	return basicItemInfo;
}

HWND ShellBrowserImpl::GetListView() const
{
	return m_listView;
}

const FolderSettings &ShellBrowserImpl::GetFolderSettings() const
{
	return m_folderSettings;
}

void ShellBrowserImpl::DeleteSelectedItems(bool permanent)
{
	std::vector<PCIDLIST_ABSOLUTE> pidls;
	int item = -1;

	while ((item = ListView_GetNextItem(m_listView, item, LVNI_SELECTED)) != -1)
	{
		auto &itemInfo = GetItemByIndex(item);
		pidls.push_back(itemInfo.pidlComplete.Raw());
	}

	if (pidls.empty())
	{
		return;
	}

	m_fileActionHandler->DeleteFiles(m_listView, pidls, permanent, false);
}

void ShellBrowserImpl::StartRenamingSelectedItems()
{
	StartRenamingItems(GetSelectedItemPidls());
}

void ShellBrowserImpl::StartRenamingItems(const std::vector<PidlAbsolute> &items)
{
	std::vector<PidlAbsolute> renameableItems;
	std::ranges::copy_if(items, std::back_inserter(renameableItems),
		[](const auto &item) { return DoesItemHaveAttributes(item.Raw(), SFGAO_CANRENAME); });

	if (renameableItems.empty())
	{
		return;
	}

	if (renameableItems.size() == 1)
	{
		StartRenamingSingleItem(renameableItems[0]);
	}
	else
	{
		StartRenamingMultipleItems(items);
	}
}

void ShellBrowserImpl::StartRenamingSingleItem(const PidlAbsolute &item)
{
	auto index = MaybeGetItemIndex(item.Raw());

	if (!index)
	{
		return;
	}

	ListView_EditLabel(m_listView, *index);
}

void ShellBrowserImpl::StartRenamingMultipleItems(const std::vector<PidlAbsolute> &items)
{
	std::list<std::wstring> fullFilenameList;

	for (const auto &item : items)
	{
		std::wstring fullFilename;
		HRESULT hr = GetDisplayName(item.Raw(), SHGDN_FORPARSING, fullFilename);

		if (FAILED(hr))
		{
			continue;
		}

		fullFilenameList.push_back(fullFilename);
	}

	if (fullFilenameList.empty())
	{
		return;
	}

	MassRenameDialog massRenameDialog(m_app->GetResourceLoader(), m_resourceInstance, m_listView,
		fullFilenameList, m_fileActionHandler);
	massRenameDialog.ShowModalDialog();
}

HRESULT ShellBrowserImpl::CopySelectedItemsToClipboard(ClipboardAction action)
{
	return CopyItemsToClipboard(GetSelectedItemPidls(), action);
}

HRESULT ShellBrowserImpl::CopyItemsToClipboard(const std::vector<PidlAbsolute> &items,
	ClipboardAction action)
{
	if (items.empty())
	{
		return E_UNEXPECTED;
	}

	wil::com_ptr_nothrow<IDataObject> clipboardDataObject;
	HRESULT hr;

	if (action == ClipboardAction::Copy)
	{
		hr = CopyFiles(m_app->GetClipboardStore(), items, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);
		}
	}
	else
	{
		hr = CutFiles(m_app->GetClipboardStore(), items, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);

			int item = -1;

			while ((item = ListView_GetNextItem(m_listView, item, LVNI_SELECTED)) != -1)
			{
				std::wstring filename = GetItemName(item);
				m_cutFileNames.emplace_back(filename);

				MarkItemAsCut(item, true);
			}
		}
	}

	return hr;
}

void ShellBrowserImpl::UpdateCurrentClipboardObject(
	wil::com_ptr_nothrow<IDataObject> clipboardDataObject)
{
	RestoreStateOfCutItems();

	m_clipboardDataObject = clipboardDataObject;
}

void ShellBrowserImpl::OnClipboardUpdate()
{
	if (m_clipboardDataObject
		&& !m_app->GetClipboardStore()->IsDataObjectCurrent(m_clipboardDataObject.get()))
	{
		RestoreStateOfCutItems();

		m_cutFileNames.clear();
		m_clipboardDataObject.reset();
	}
}

void ShellBrowserImpl::RestoreStateOfCutItems()
{
	// FIXME: Should base this off something other than the filename (which can change).
	for (const auto &filename : m_cutFileNames)
	{
		int item = LocateFileItemIndex(filename.c_str());

		if (item != -1)
		{
			MarkItemAsCut(item, false);
		}
	}
}

void ShellBrowserImpl::PasteShortcut()
{
	auto serviceProvider = winrt::make_self<ServiceProvider>();
	serviceProvider->RegisterService(IID_IFolderView,
		winrt::make<FolderView>(m_weakPtrFactory.GetWeakPtr()));

	ExecuteActionFromContextMenu(m_directoryState.pidlDirectory.Raw(), {}, m_listView, L"pastelink",
		0, serviceProvider.get());
}

void ShellBrowserImpl::PasteHardLinks()
{
	auto pastedItems =
		ClipboardOperations::PasteHardLinks(m_app->GetClipboardStore(), GetDirectoryPath());
	OnInternalPaste(pastedItems);
}

void ShellBrowserImpl::PasteSymLinks()
{
	auto pastedItems =
		ClipboardOperations::PasteSymLinks(m_app->GetClipboardStore(), GetDirectoryPath());
	OnInternalPaste(pastedItems);
}

void ShellBrowserImpl::OnInternalPaste(const ClipboardOperations::PastedItems &pastedItems)
{
	std::vector<PidlAbsolute> pidls;

	for (const auto &pastedItem :
		pastedItems | std::views::filter([](const auto &item) { return !item.error; }))
	{
		unique_pidl_absolute pidl(SHSimpleIDListFromPath(pastedItem.path.c_str()));

		if (pidl)
		{
			pidls.push_back(pidl.get());
		}
	}

	SelectItems(pidls);
}

WeakPtr<ShellBrowserImpl> ShellBrowserImpl::GetWeakPtr()
{
	return m_weakPtrFactory.GetWeakPtr();
}

NavigationManager *ShellBrowserImpl::GetNavigationManager()
{
	return &m_navigationManager;
}

const NavigationManager *ShellBrowserImpl::GetNavigationManager() const
{
	return &m_navigationManager;
}

bool ShellBrowserImpl::IsCommandEnabled(int command) const
{
	switch (command)
	{
	case IDM_FILE_COPYITEMPATH:
	case IDM_FILE_COPYUNIVERSALFILEPATHS:
		return ListView_GetSelectedCount(m_listView) > 0;

	case IDM_FILE_SETFILEATTRIBUTES:
		return DialogHelper::CanShowSetFileAttributesDialogForItems(GetSelectedItemPidls());

	case IDM_FILE_DELETE:
	case IDM_FILE_DELETEPERMANENTLY:
		return DoAllSelectedItemsHaveAttributes(SFGAO_CANDELETE);

	case IDM_FILE_RENAME:
		return DoAllSelectedItemsHaveAttributes(SFGAO_CANRENAME);

	case IDM_FILE_PROPERTIES:
		return DoAllSelectedItemsHaveAttributes(SFGAO_HASPROPSHEET);

	case IDM_EDIT_MOVETOFOLDER:
	case IDM_EDIT_CUT:
		return DoAllSelectedItemsHaveAttributes(SFGAO_CANMOVE);

	case IDM_EDIT_COPYTOFOLDER:
	case IDM_EDIT_COPY:
		return DoAllSelectedItemsHaveAttributes(SFGAO_CANCOPY);
	}

	return false;
}

void ShellBrowserImpl::ExecuteCommand(int command)
{
	switch (command)
	{
	case IDM_FILE_COPYITEMPATH:
		CopySelectedItemPaths(PathType::Parsing);
		break;

	case IDM_FILE_COPYUNIVERSALFILEPATHS:
		CopySelectedItemPaths(PathType::UniversalPath);
		break;

	case IDM_FILE_SETFILEATTRIBUTES:
		SetFileAttributesForSelectedItems();
		break;

	case IDM_FILE_DELETE:
		DeleteSelectedItems(false);
		break;

	case IDM_FILE_DELETEPERMANENTLY:
		DeleteSelectedItems(true);
		break;

	case IDM_FILE_RENAME:
		StartRenamingSelectedItems();
		break;

	case IDM_FILE_PROPERTIES:
		ShowPropertiesForSelectedItems();
		break;

	case IDM_EDIT_CUT:
		CopySelectedItemsToClipboard(ClipboardAction::Cut);
		break;

	case IDM_EDIT_COPY:
		CopySelectedItemsToClipboard(ClipboardAction::Copy);
		break;

	case IDM_EDIT_MOVETOFOLDER:
		CopySelectedItemsToFolder(TransferAction::Move);
		break;

	case IDM_EDIT_COPYTOFOLDER:
		CopySelectedItemsToFolder(TransferAction::Copy);
		break;
	}
}

void ShellBrowserImpl::CopySelectedItemPaths(PathType pathType) const
{
	auto selectedItems = GetSelectedItemPidls();
	CopyItemPathsToClipboard(m_app->GetClipboardStore(), selectedItems, pathType);
}

void ShellBrowserImpl::SetFileAttributesForSelectedItems()
{
	std::vector<DialogHelper::ItemPidlAndFindData> selectedItems;
	int index = -1;

	while ((index = ListView_GetNextItem(m_listView, index, LVNI_SELECTED)) != -1)
	{
		const ItemInfo_t &item = GetItemByIndex(index);
		selectedItems.emplace_back(item.pidlComplete, item.wfd);
	}

	DialogHelper::MaybeShowSetFileAttributesDialog(m_app->GetResourceLoader(), m_owner,
		selectedItems);
}

bool ShellBrowserImpl::CanCreateNewFolder() const
{
	return CanCreateInDirectory(m_directoryState.pidlDirectory.Raw());
}

void ShellBrowserImpl::CreateNewFolder()
{
	wil::com_ptr_nothrow<IShellItem> directoryShellItem;
	HRESULT hr = SHCreateItemFromIDList(m_directoryState.pidlDirectory.Raw(),
		IID_PPV_ARGS(&directoryShellItem));

	if (FAILED(hr))
	{
		return;
	}

	auto sink = winrt::make_self<FileProgressSink>();
	sink->SetPostNewItemObserver(
		[this](PIDLIST_ABSOLUTE pidl)
		{
			ListViewHelper::SelectAllItems(m_listView, false);
			SetFocus(m_listView);

			QueueRename(pidl);
		});

	auto newFolderName = m_app->GetResourceLoader()->LoadString(IDS_NEW_FOLDER_NAME);
	FileOperations::CreateNewFolder(directoryShellItem.get(), newFolderName, sink.get());
}

bool ShellBrowserImpl::CanSplitFile() const
{
	return !!GetFilePathForSplit();
}

void ShellBrowserImpl::SplitFile()
{
	auto itemPath = GetFilePathForSplit();

	if (!itemPath)
	{
		return;
	}

	SplitFileDialog splitFileDialog(m_app->GetResourceLoader(), m_owner, *itemPath);
	splitFileDialog.ShowModalDialog();
}

std::optional<std::wstring> ShellBrowserImpl::GetFilePathForSplit() const
{
	if (m_directoryState.virtualFolder)
	{
		return std::nullopt;
	}

	if (DoesItemHaveAttributes(m_directoryState.pidlDirectory.Raw(), SFGAO_STREAM))
	{
		// The current directory is the top-level folder in a container file. It's not possible to
		// split a file in this case.
		return std::nullopt;
	}

	int selectedItem = ListView_GetNextItem(m_listView, -1, LVNI_SELECTED);

	if (selectedItem == -1)
	{
		return std::nullopt;
	}

	const auto &itemInfo = GetItemByIndex(selectedItem);

	if (WI_IsFlagSet(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		return std::nullopt;
	}

	return itemInfo.parsingName;
}

bool ShellBrowserImpl::CanMergeFiles() const
{
	return !!GetFilePathsForMerge();
}

void ShellBrowserImpl::MergeFiles()
{
	auto items = GetFilePathsForMerge();

	if (!items)
	{
		return;
	}

	MergeFilesDialog mergeFilesDialog(m_app->GetResourceLoader(), m_owner,
		m_directoryState.directory, *items, m_config->globalFolderSettings.showFriendlyDates);
	mergeFilesDialog.ShowModalDialog();
}

std::optional<std::vector<std::wstring>> ShellBrowserImpl::GetFilePathsForMerge() const
{
	if (m_directoryState.virtualFolder)
	{
		return std::nullopt;
	}

	if (DoesItemHaveAttributes(m_directoryState.pidlDirectory.Raw(), SFGAO_STREAM))
	{
		return std::nullopt;
	}

	std::vector<std::wstring> items;
	int index = -1;

	while ((index = ListView_GetNextItem(m_listView, index, LVNI_SELECTED)) != -1)
	{
		const auto &itemInfo = GetItemByIndex(index);

		if (WI_IsFlagSet(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			return std::nullopt;
		}

		items.push_back(itemInfo.parsingName);
	}

	if (items.size() < 2)
	{
		// There have to be at least 2 items to perform a merge.
		return std::nullopt;
	}

	return items;
}

void ShellBrowserImpl::CopySelectedItemsToFolder(TransferAction action)
{
	auto pidls = GetSelectedItemPidls();

	if (pidls.empty())
	{
		return;
	}

	std::vector<PCIDLIST_ABSOLUTE> rawPidls;
	std::ranges::transform(pidls, std::back_inserter(rawPidls),
		[](const auto &pidl) { return pidl.Raw(); });

	Epp::FileOperations::CopyFilesToFolder(m_owner, rawPidls, action, m_app->GetResourceLoader());
}

void ShellBrowserImpl::SelectAllItems()
{
	ListViewHelper::SelectAllItems(m_listView, true);
	SetFocus(m_listView);
}

void ShellBrowserImpl::InvertSelection()
{
	ListViewHelper::InvertSelection(m_listView);
	SetFocus(m_listView);
}

bool ShellBrowserImpl::CanStartWildcardSelection(SelectionType selectionType) const
{
	if (selectionType == SelectionType::Select)
	{
		return true;
	}

	int numSelected = ListView_GetSelectedCount(m_listView);
	return numSelected > 0;
}

void ShellBrowserImpl::StartWildcardSelection(SelectionType selectionType)
{
	WildcardSelectDialog wilcardSelectDialog(m_app->GetResourceLoader(), m_owner, this,
		selectionType);
	wilcardSelectDialog.ShowModalDialog();
}

void ShellBrowserImpl::SelectItemsMatchingPattern(const std::wstring &pattern,
	SelectionType selectionType)
{
	int numItems = ListView_GetItemCount(m_listView);

	for (int i = 0; i < numItems; i++)
	{
		std::wstring filename = GetItemName(i);

		if (CheckWildcardMatch(pattern.c_str(), filename.c_str(), false))
		{
			ListViewHelper::SelectItem(m_listView, i, selectionType == SelectionType::Select);
		}
	}
}

bool ShellBrowserImpl::CanClearSelection() const
{
	return ListView_GetSelectedCount(m_listView) > 0;
}

void ShellBrowserImpl::ClearSelection()
{
	ListViewHelper::SelectAllItems(m_listView, false);
	SetFocus(m_listView);
}

bool ShellBrowserImpl::CanSaveDirectoryListing() const
{
	return !m_directoryState.virtualFolder;
}

void ShellBrowserImpl::SaveDirectoryListing()
{
	if (!CanSaveDirectoryListing())
	{
		return;
	}

	const auto *resourceLoader = m_app->GetResourceLoader();
	auto defaultFileName = resourceLoader->LoadString(IDS_DIRECTORY_LISTING_FILENAME);

	std::vector<FileDialogs::FileType> fileTypes = {
		{ resourceLoader->LoadString(IDS_DIRECTORY_LISTING_TEXT_DOCUMENT), L"*.txt" }
	};

	std::wstring filePath;
	HRESULT hr = FileDialogs::ShowSaveAsDialog(m_listView, m_directoryState.directory,
		defaultFileName, fileTypes, 0, filePath);

	if (FAILED(hr))
	{
		return;
	}

	FileOperations::SaveDirectoryListing(m_directoryState.directory, filePath);
}

boost::signals2::connection ShellBrowserImpl::AddDestroyedObserver(
	const DestroyedSignal::slot_type &observer)
{
	return m_destroyedSignal.connect(observer);
}
