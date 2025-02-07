// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "ClipboardOperations.h"
#include "ColorRuleModel.h"
#include "Config.h"
#include "CoreInterface.h"
#include "FolderView.h"
#include "IconFetcherImpl.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ShellEnumerator.h"
#include "MassRenameDialog.h"
#include "PreservedFolderState.h"
#include "ServiceProvider.h"
#include "ShellBrowserEmbedder.h"
#include "ShellBrowserHelper.h"
#include "ShellNavigationController.h"
#include "SortModes.h"
#include "ThemeManager.h"
#include "ViewModeHelper.h"
#include "ViewModes.h"
#include "../Helper/Controls.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileOperations.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <list>

ShellBrowserImpl::ShellBrowserImpl(HWND hOwner, ShellBrowserEmbedder *embedder, App *app,
	CoreInterface *coreInterface, TabNavigationInterface *tabNavigation,
	FileActionHandler *fileActionHandler,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
	const PreservedFolderState &preservedFolderState) :
	ShellBrowserImpl(hOwner, embedder, app, coreInterface, tabNavigation, fileActionHandler,
		preservedFolderState.folderSettings, nullptr)
{
	m_navigationController =
		std::make_unique<ShellNavigationController>(this, tabNavigation, history, currentEntry);
}

ShellBrowserImpl::ShellBrowserImpl(HWND hOwner, ShellBrowserEmbedder *embedder, App *app,
	CoreInterface *coreInterface, TabNavigationInterface *tabNavigation,
	FileActionHandler *fileActionHandler, const FolderSettings &folderSettings,
	const FolderColumns *initialColumns) :
	ShellDropTargetWindow(CreateListView(hOwner)),
	m_hListView(GetHWND()),
	m_hOwner(hOwner),
	m_app(app),
	m_tabNavigation(tabNavigation),
	m_fileActionHandler(fileActionHandler),
	m_fontSetter(GetHWND(), app->GetConfig()),
	m_tooltipFontSetter(reinterpret_cast<HWND>(SendMessage(GetHWND(), LVM_GETTOOLTIPS, 0, 0)),
		app->GetConfig()),
	m_columnThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_columnResultIDCounter(0),
	m_cachedIcons(coreInterface->GetCachedIcons()),
	m_thumbnailThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_thumbnailResultIDCounter(0),
	m_infoTipsThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_infoTipResultIDCounter(0),
	m_resourceInstance(coreInterface->GetResourceInstance()),
	m_acceleratorManager(coreInterface->GetAcceleratorManager()),
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
	m_iconFetcher = std::make_unique<IconFetcherImpl>(m_hListView, m_cachedIcons);
	m_navigationController = std::make_unique<ShellNavigationController>(this, tabNavigation);

	m_getDragImageMessage = RegisterWindowMessage(DI_GETDRAGIMAGE);

	m_bFolderVisited = FALSE;

	m_performingDrag = false;
	m_nCurrentColumns = 0;
	m_pActiveColumns = nullptr;
	m_nActiveColumns = 0;
	m_middleButtonItem = -1;

	m_uniqueFolderId = 0;

	m_PreviousSortColumnExists = false;

	InitializeCriticalSection(&m_csDirectoryAltered);

	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_iFolderIcon));
	FAIL_FAST_IF_FAILED(GetDefaultFileIconIndex(m_iFileIcon));

	AddClipboardFormatListener(m_hListView);

	m_connections.push_back(coreInterface->AddDeviceChangeObserver(
		std::bind_front(&ShellBrowserImpl::OnDeviceChange, this)));

	m_shellWindows = winrt::try_create_instance<IShellWindows>(CLSID_ShellWindows, CLSCTX_ALL);

	embedder->OnShellBrowserCreated(this);
}

ShellBrowserImpl::~ShellBrowserImpl()
{
	if (m_clipboardDataObject && OleIsCurrentClipboard(m_clipboardDataObject.get()) == S_OK)
	{
		// Ensure that any data that was copied to the clipboard remains there. Technically, this
		// only needs to be done when the application is closed. However, determining whether the
		// application set the current data on the clipboard when the tab that set the data has
		// already been closed is difficult, so the easiest thing to do is just flush the clipboard
		// here.
		OleFlushClipboard();
	}

	RemoveClipboardFormatListener(m_hListView);

	DestroyWindow(m_hListView);

	m_columnThreadPool.clear_queue();
	m_thumbnailThreadPool.clear_queue();
	m_infoTipsThreadPool.clear_queue();

	DeleteCriticalSection(&m_csDirectoryAltered);
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
	auto dwExtendedStyle = ListView_GetExtendedListViewStyle(m_hListView);

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

	ListView_SetExtendedListViewStyle(m_hListView, dwExtendedStyle);

	ListViewHelper::SetAutoArrange(m_hListView, m_folderSettings.autoArrange);
	ListViewHelper::AddRemoveExtendedStyles(m_hListView, LVS_EX_GRIDLINES,
		m_config->globalFolderSettings.showGridlines.get());

	m_connections.push_back(m_config->globalFolderSettings.showGridlines.addObserver(
		std::bind_front(&ShellBrowserImpl::OnShowGridlinesUpdated, this)));

	ListViewHelper::ActivateOneClickSelect(m_hListView,
		m_config->globalFolderSettings.oneClickActivate.get(),
		m_config->globalFolderSettings.oneClickActivateHoverTime.get());

	m_connections.push_back(m_config->globalFolderSettings.oneClickActivate.addObserver(
		std::bind_front(&ShellBrowserImpl::OnOneClickActivateUpdated, this)));
	m_connections.push_back(m_config->globalFolderSettings.oneClickActivateHoverTime.addObserver(
		std::bind_front(&ShellBrowserImpl::OnOneClickActivateHoverTimeUpdated, this)));

	m_app->GetThemeManager()->ApplyThemeToWindowAndChildren(m_hListView);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hListView,
		std::bind_front(&ShellBrowserImpl::ListViewProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(m_hListView),
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
		ListView_EnableGroupView(m_hListView, true);
	}
}

HIMAGELIST ShellBrowserImpl::GetIconsList(int margin)
{
	wil::com_ptr_nothrow<IImageList> pImageList;
	SHGetImageList(SHIL_SMALL, IID_PPV_ARGS(&pImageList));
	HIMAGELIST hIconsList = reinterpret_cast<HIMAGELIST>(pImageList.get());
	if (margin <= 0)
		return hIconsList;

	// The approach to add margin is to build a copy of the icons list containing the same icons
	// but including the margin.

	/* It is required to previously call SHGetFileInfo() for each file in current directory
	in order to load all the icons in the list pointed by pImageList.
	Otherwise the current directory could contain some new icon, not previously seen and so missing
	in the image list. This is because the list is not initially complete,
	it goes growing whenever new icons are required with SHGetFileInfo(,,,,SHGFI_SYSICONINDEX). */
	std::vector<ItemInfo_t> items;
	EnumerateFolder(m_directoryState.pidlDirectory.get(),
		m_hOwner, m_folderSettings.showHidden, items);
	SHFILEINFO sfi;
	for (const auto &item : items)
		SHGetFileInfo((LPCTSTR) item.pidlComplete.get(), 0, &sfi, sizeof(sfi),
			SHGFI_PIDL | SHGFI_SYSICONINDEX);

	HDC hdc, hdcBacking;
	HBITMAP hBackingBitmap, hBackingBitmapOld;
	HICON hIcon;
	int iIconWidth, iIconHeight, numItems;
	ImageList_GetIconSize(hIconsList, &iIconWidth, &iIconHeight);
	numItems = ImageList_GetImageCount(hIconsList);
	HIMAGELIST hNewIconsList =
		ImageList_Create(iIconWidth + margin * 2, iIconWidth + margin * 2, ILC_COLOR32, numItems, 0);
	hdc = GetDC(m_hListView);
	for (int i = 0; i < numItems; i++)
	{
		hIcon = ImageList_GetIcon(hIconsList, i, ILD_NORMAL);
		hdcBacking = CreateCompatibleDC(hdc);
		hBackingBitmap = CreateCompatibleBitmap(hdc, iIconWidth + margin * 2, iIconHeight + margin * 2);
		hBackingBitmapOld = (HBITMAP) SelectObject(hdcBacking, hBackingBitmap);

		DrawIconEx(hdcBacking, margin * 2, margin, hIcon, 0, 0, 0, nullptr, DI_NORMAL);
		DestroyIcon(hIcon);
		SelectObject(hdcBacking, hBackingBitmapOld);
		DeleteDC(hdcBacking);

		ImageList_Add(hNewIconsList, hBackingBitmap, nullptr);
		DeleteObject(hBackingBitmap);
	}
	ReleaseDC(m_hListView, hdc);
	return hNewIconsList;
}

bool ShellBrowserImpl::GetAutoArrange() const
{
	return m_folderSettings.autoArrange;
}

void ShellBrowserImpl::SetAutoArrange(bool autoArrange)
{
	m_folderSettings.autoArrange = autoArrange;

	ListViewHelper::SetAutoArrange(m_hListView, m_folderSettings.autoArrange);
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

	ListView_SetImageList(m_hListView, nullptr, LVSIL_SMALL);
	ListView_SetImageList(m_hListView, nullptr, LVSIL_NORMAL);

	switch (viewMode)
	{
	case ViewMode::ExtraLargeIcons:
	{
		wil::com_ptr_nothrow<IImageList> pImageList;
		SHGetImageList(SHIL_JUMBO, IID_PPV_ARGS(&pImageList));
		ListView_SetImageList(m_hListView, reinterpret_cast<HIMAGELIST>(pImageList.get()),
			LVSIL_NORMAL);
	}
	break;

	case ViewMode::LargeIcons:
	{
		wil::com_ptr_nothrow<IImageList> pImageList;
		SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&pImageList));
		ListView_SetImageList(m_hListView, reinterpret_cast<HIMAGELIST>(pImageList.get()),
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
		ListView_SetImageList(m_hListView, reinterpret_cast<HIMAGELIST>(pImageList.get()),
			LVSIL_NORMAL);
	}
	break;

	case ViewMode::SmallIcons:
	case ViewMode::List:
	case ViewMode::Details:
	{
		int margin = 1; // It would be interesting to include in the "Window" options.
		HIMAGELIST hIconsList = GetIconsList(margin);
		ListView_SetImageList(m_hListView, hIconsList, LVSIL_SMALL);
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

	SendMessage(m_hListView, LVM_SETVIEW, dwStyle, 0);

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
	int numItems = ListView_GetItemCount(m_hListView);

	for (int i = 0; i < numItems; i++)
	{
		ListView_SetItemText(m_hListView, i, 0, LPSTR_TEXTCALLBACK);
	}
}

void ShellBrowserImpl::SetFirstColumnTextToFilename()
{
	int numItems = ListView_GetItemCount(m_hListView);

	for (int i = 0; i < numItems; i++)
	{
		int internalIndex = GetItemInternalIndex(i);

		BasicItemInfo_t basicItemInfo = getBasicItemInfo(internalIndex);
		std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);

		ListView_SetItemText(m_hListView, i, 0, filename.data());
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

	ListView_SortGroups(m_hListView, GroupComparisonStub, this);
}

void ShellBrowserImpl::SetID(int id)
{
	assert(!m_ID);

	m_ID = id;
}

int ShellBrowserImpl::GetId() const
{
	assert(m_ID);

	return *m_ID;
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

std::wstring ShellBrowserImpl::GetDirectory() const
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
	ListViewHelper::SelectAllItems(m_hListView, false);

	int smallestIndex = INT_MAX;

	for (auto &pidl : pidls)
	{
		auto index = GetItemIndexForPidl(pidl.Raw());

		if (!index)
		{
			m_directoryState.filesToSelect.emplace_back(pidl);
			continue;
		}

		ListViewHelper::SelectItem(m_hListView, *index, true);

		if (*index < smallestIndex)
		{
			smallestIndex = *index;
		}
	}

	if (smallestIndex != INT_MAX)
	{
		ListViewHelper::FocusItem(m_hListView, smallestIndex, true);
		ListView_EnsureVisible(m_hListView, smallestIndex, FALSE);
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
		iItem = ListView_FindItem(m_hListView, -1, &lvFind);

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
	int item = ListView_FindItem(m_hListView, -1, &lvfi);

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

/* We can create files in this folder if it is
part of the filesystem, or if it is the root of
the namespace (i.e. the desktop). */
BOOL ShellBrowserImpl::CanCreate() const
{
	BOOL bCanCreate = FALSE;
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(nullptr, CSIDL_DESKTOP, nullptr, 0, wil::out_param(pidl));

	if (SUCCEEDED(hr))
	{
		bCanCreate = !InVirtualFolder()
			|| ArePidlsEquivalent(m_directoryState.pidlDirectory.Raw(), pidl.get());
	}

	return bCanCreate;
}

void ShellBrowserImpl::SetDirMonitorId(int dirMonitorId)
{
	m_dirMonitorId = dirMonitorId;
}

void ShellBrowserImpl::ClearDirMonitorId()
{
	m_dirMonitorId.reset();
}

std::optional<int> ShellBrowserImpl::GetDirMonitorId() const
{
	return m_dirMonitorId;
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

	nItems = ListView_GetItemCount(m_hListView);

	while (res > 0 && i < nItems)
	{
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		bItem = ListView_GetItem(m_hListView, &lvItem);

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
	int numItems = ListView_GetItemCount(m_hListView);

	for (int i = 0; i < numItems; i++)
	{
		const auto &item = GetItemByIndex(i);

		if (ArePidlsEquivalent(pidlItem, item.pidlComplete.Raw()))
		{
			ListView_EditLabel(m_hListView, i);
			return;
		}
	}

	m_directoryState.queuedRenameItem = pidlItem;
}

void ShellBrowserImpl::OnDeviceChange(UINT eventType, LONG_PTR eventData)
{
	// If shell change notifications are enabled, drive additions/removals will be handled through
	// that.
	if (m_config->shellChangeNotificationType == ShellChangeNotificationType::All
		|| m_config->shellChangeNotificationType == ShellChangeNotificationType::NonFilesystem)
	{
		return;
	}

	/* Note changes made here may have no effect. Since
	the icon for the cd/dvd/etc. may not have been
	updated by the time this function is called, it's
	possible this may not change anything. */

	/* If we are currently not in my computer, this
	message can be safely ignored (drives are only
	shown in my computer). */
	if (CompareVirtualFolders(CSIDL_DRIVES))
	{
		switch (eventType)
		{
			/* Device has being added/inserted into the system. Update the
			drives toolbar as necessary. */
		case DBT_DEVICEARRIVAL:
		{
			DEV_BROADCAST_HDR *dbh = nullptr;

			dbh = (DEV_BROADCAST_HDR *) eventData;

			if (dbh->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				DEV_BROADCAST_VOLUME *pdbv = nullptr;
				TCHAR chDrive;
				TCHAR szDrive[4];

				pdbv = (DEV_BROADCAST_VOLUME *) dbh;

				/* Build a string that will form the drive name. */
				chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
				StringCchPrintf(szDrive, std::size(szDrive), _T("%c:\\"), chDrive);

				if (pdbv->dbcv_flags & DBTF_MEDIA)
				{
					UpdateDriveIcon(szDrive);
				}
				else
				{
					PidlAbsolute simplePidl;
					HRESULT hr = CreateSimplePidl(szDrive, simplePidl);

					if (SUCCEEDED(hr))
					{
						OnItemAdded(simplePidl.Raw());
					}
				}
			}
		}
		break;

		case DBT_DEVICEREMOVECOMPLETE:
		{
			DEV_BROADCAST_HDR *dbh = nullptr;

			dbh = (DEV_BROADCAST_HDR *) eventData;

			if (dbh->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				DEV_BROADCAST_VOLUME *pdbv = nullptr;
				TCHAR chDrive;
				TCHAR szDrive[4];

				pdbv = (DEV_BROADCAST_VOLUME *) dbh;

				/* Build a string that will form the drive name. */
				chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
				StringCchPrintf(szDrive, std::size(szDrive), _T("%c:\\"), chDrive);

				/* The device was removed from the system.
				Remove it from the listview (only if the drive
				was actually removed - the drive may not have
				been removed, for example, if a cd/dvd was
				changed). */
				if (pdbv->dbcv_flags & DBTF_MEDIA)
				{
					UpdateDriveIcon(szDrive);
				}
				else
				{
					/* At this point, the drive has been completely removed
					from the system. Therefore, its display name cannot be
					queried. Need to search for the drive using ONLY its
					drive letter/name. Once its index in the listview has
					been determined, it can simply be removed. */
					RemoveDrive(szDrive);
				}
			}
		}
		break;
		}
	}
}

void ShellBrowserImpl::UpdateDriveIcon(const TCHAR *szDrive)
{
	LVITEM lvItem;
	SHFILEINFO shfi;
	HRESULT hr;
	int iItem = -1;
	int iItemInternal = -1;
	int i = 0;

	/* Look for the item using its display name, NOT
	its drive letter/name. */
	std::wstring displayName;
	GetDisplayName(szDrive, SHGDN_INFOLDER, displayName);

	unique_pidl_absolute pidlDrive;
	hr = SHParseDisplayName(szDrive, nullptr, wil::out_param(pidlDrive), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		for (i = 0; i < m_directoryState.numItems; i++)
		{
			lvItem.mask = LVIF_PARAM;
			lvItem.iItem = i;
			lvItem.iSubItem = 0;
			ListView_GetItem(m_hListView, &lvItem);

			if (ArePidlsEquivalent(pidlDrive.get(),
					m_itemInfoMap.at((int) lvItem.lParam).pidlComplete.Raw()))
			{
				iItem = i;
				iItemInternal = (int) lvItem.lParam;

				break;
			}
		}
	}

	if (iItem != -1)
	{
		SHGetFileInfo(szDrive, 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX);

		m_itemInfoMap.at(iItemInternal).displayName = displayName;

		/* Update the drives icon and display name. */
		lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvItem.iImage = shfi.iIcon;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = displayName.data();
		ListView_SetItem(m_hListView, &lvItem);
	}
}

void ShellBrowserImpl::RemoveDrive(const TCHAR *szDrive)
{
	LVITEM lvItem;
	int iItemInternal = -1;
	int i = 0;

	for (i = 0; i < m_directoryState.numItems; i++)
	{
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		ListView_GetItem(m_hListView, &lvItem);

		if (m_itemInfoMap.at((int) lvItem.lParam).bDrive)
		{
			if (lstrcmp(szDrive, m_itemInfoMap.at((int) lvItem.lParam).szDrive) == 0)
			{
				iItemInternal = (int) lvItem.lParam;
				break;
			}
		}
	}

	if (iItemInternal != -1)
	{
		RemoveItem(iItemInternal);
	}
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
	return m_hListView;
}

FolderSettings ShellBrowserImpl::GetFolderSettings() const
{
	return m_folderSettings;
}

void ShellBrowserImpl::DeleteSelectedItems(bool permanent)
{
	std::vector<PCIDLIST_ABSOLUTE> pidls;
	int item = -1;

	while ((item = ListView_GetNextItem(m_hListView, item, LVNI_SELECTED)) != -1)
	{
		auto &itemInfo = GetItemByIndex(item);
		pidls.push_back(itemInfo.pidlComplete.Raw());
	}

	if (pidls.empty())
	{
		return;
	}

	m_fileActionHandler->DeleteFiles(m_hListView, pidls, permanent, false);
}

void ShellBrowserImpl::StartRenamingSelectedItems()
{
	int numSelected = ListView_GetSelectedCount(m_hListView);

	// If there is only item selected, start editing it in-place. If multiple items are selected,
	// show the mass rename dialog.
	if (numSelected == 1)
	{
		StartRenamingSingleFile();
	}
	else if (numSelected > 1)
	{
		StartRenamingMultipleFiles();
	}
}

void ShellBrowserImpl::StartRenamingSingleFile()
{
	int selectedItem = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED | LVNI_FOCUSED);

	if (selectedItem == -1)
	{
		return;
	}

	bool canRename = TestListViewItemAttributes(selectedItem, SFGAO_CANRENAME);

	if (!canRename)
	{
		return;
	}

	ListView_EditLabel(m_hListView, selectedItem);
}

void ShellBrowserImpl::StartRenamingMultipleFiles()
{
	std::list<std::wstring> fullFilenameList;
	int item = -1;

	while ((item = ListView_GetNextItem(m_hListView, item, LVNI_SELECTED)) != -1)
	{
		bool canRename = TestListViewItemAttributes(item, SFGAO_CANRENAME);

		if (!canRename)
		{
			continue;
		}

		fullFilenameList.push_back(GetItemFullName(item));
	}

	if (fullFilenameList.empty())
	{
		return;
	}

	MassRenameDialog massRenameDialog(m_resourceInstance, m_hListView, m_app->GetThemeManager(),
		fullFilenameList, m_app->GetIconResourceLoader(), m_fileActionHandler);
	massRenameDialog.ShowModalDialog();
}

HRESULT ShellBrowserImpl::CopySelectedItemsToClipboard(bool copy)
{
	auto pidls = GetSelectedItemPidls();

	if (pidls.empty())
	{
		return E_UNEXPECTED;
	}

	wil::com_ptr_nothrow<IDataObject> clipboardDataObject;
	HRESULT hr;

	if (copy)
	{
		hr = CopyFiles(pidls, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);
		}
	}
	else
	{
		hr = CutFiles(pidls, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);

			int item = -1;

			while ((item = ListView_GetNextItem(m_hListView, item, LVNI_SELECTED)) != -1)
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
	if (m_clipboardDataObject && OleIsCurrentClipboard(m_clipboardDataObject.get()) == S_FALSE)
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

	auto folderView = winrt::make<FolderView>(m_weakPtrFactory.GetWeakPtr());
	serviceProvider->RegisterService(IID_IFolderView, folderView.get());

	ExecuteActionFromContextMenu(m_directoryState.pidlDirectory.Raw(), {}, m_hListView,
		L"pastelink", 0, serviceProvider.get());
}

void ShellBrowserImpl::PasteHardLinks()
{
	auto pastedItems = ClipboardOperations::PasteHardLinks(GetDirectory());
	OnInternalPaste(pastedItems);
}

void ShellBrowserImpl::PasteSymLinks()
{
	auto pastedItems = ClipboardOperations::PasteSymLinks(GetDirectory());
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

void ShellBrowserImpl::AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper)
{
	m_helpers.push_back(std::move(helper));
}

WeakPtr<ShellBrowserImpl> ShellBrowserImpl::GetWeakPtr()
{
	return m_weakPtrFactory.GetWeakPtr();
}
