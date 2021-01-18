// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "HistoryEntry.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ShellNavigationController.h"
#include "ViewModes.h"
#include "../Helper/Helper.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <propkey.h>
#include <propvarutil.h>
#include <list>

HRESULT ShellBrowser::BrowseFolder(const HistoryEntry &entry)
{
	HRESULT hr = BrowseFolder(entry.GetPidl().get(), false);

	if (SUCCEEDED(hr))
	{
		auto selectedItems = entry.GetSelectedItems();
		SelectItems(ShallowCopyPidls(selectedItems));
	}

	return hr;
}

HRESULT ShellBrowser::BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)
{
	SetCursor(LoadCursor(nullptr, IDC_WAIT));

	auto resetCursor = wil::scope_exit([] {
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
	});

	m_navigationStartedSignal(pidlDirectory);

	HRESULT hr = EnumerateFolder(pidlDirectory, addHistoryEntry);

	if (FAILED(hr))
	{
		m_navigationFailedSignal();
		return hr;
	}

	/* Stop the list view from redrawing itself each time is inserted.
	Redrawing will be allowed once all items have being inserted.
	(reduces lag when a large number of items are going to be inserted). */
	SendMessage(m_hListView, WM_SETREDRAW, FALSE, NULL);

	SetActiveColumnSet();
	SetViewModeInternal(m_folderSettings.viewMode);

	InsertAwaitingItems(FALSE);

	VerifySortMode();
	SortFolder(m_folderSettings.sortMode);

	ListView_EnsureVisible(m_hListView, 0, FALSE);

	/* Allow the listview to redraw itself once again. */
	SendMessage(m_hListView, WM_SETREDRAW, TRUE, NULL);

	if (m_config->registerForShellNotifications)
	{
		StartDirectoryMonitoring(pidlDirectory);
	}

	/* Set the focus back to the first item. */
	ListView_SetItemState(m_hListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);

	m_bFolderVisited = TRUE;

	m_navigationCompletedSignal(pidlDirectory);

	return hr;
}

void ShellBrowser::PrepareToChangeFolders()
{
	if (m_bFolderVisited)
	{
		SaveColumnWidths();
	}

	ClearPendingResults();

	if (m_config->registerForShellNotifications)
	{
		StopDirectoryMonitoring();

		KillTimer(m_hListView, PROCESS_SHELL_CHANGES_TIMER_ID);
	}

	EnterCriticalSection(&m_csDirectoryAltered);
	m_FileSelectionList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	StoreCurrentlySelectedItems();

	ListView_DeleteAllItems(m_hListView);

	if (m_bFolderVisited)
	{
		ResetFolderState();
	}
}

void ShellBrowser::ClearPendingResults()
{
	m_columnThreadPool.clear_queue();
	m_columnResults.clear();

	m_iconFetcher->ClearQueue();

	m_thumbnailThreadPool.clear_queue();
	m_thumbnailResults.clear();

	m_infoTipsThreadPool.clear_queue();
	m_infoTipResults.clear();
}

void ShellBrowser::ResetFolderState()
{
	/* If we're in thumbnails view, destroy the current
	imagelist, and create a new one. */
	if (m_folderSettings.viewMode == +ViewMode::Thumbnails)
	{
		auto himlOld = ListView_GetImageList(m_hListView, LVSIL_NORMAL);

		int nItems = ListView_GetItemCount(m_hListView);

		/* Create and set the new imagelist. */
		HIMAGELIST himl = ImageList_Create(
			THUMBNAIL_ITEM_WIDTH, THUMBNAIL_ITEM_HEIGHT, ILC_COLOR32, nItems, nItems + 100);
		ListView_SetImageList(m_hListView, himl, LVSIL_NORMAL);

		ImageList_Destroy(himlOld);
	}

	m_directoryState = DirectoryState();

	EnterCriticalSection(&m_csDirectoryAltered);
	m_AlteredList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	m_itemInfoMap.clear();
}

void ShellBrowser::StoreCurrentlySelectedItems()
{
	auto *entry = m_navigationController->GetCurrentEntry();

	if (!entry)
	{
		return;
	}

	std::vector<PCIDLIST_ABSOLUTE> selectedItems;
	int index = -1;

	while ((index = ListView_GetNextItem(m_hListView, index, LVNI_SELECTED)) != -1)
	{
		auto &item = GetItemByIndex(index);
		selectedItems.push_back(item.pidlComplete.get());
	}

	entry->SetSelectedItems(selectedItems);
}

HRESULT ShellBrowser::EnumerateFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)
{
	wil::com_ptr_nothrow<IShellFolder> parent;
	PCITEMID_CHILD child;
	HRESULT hr = SHBindToParent(pidlDirectory, IID_PPV_ARGS(&parent), &child);

	if (FAILED(hr))
	{
		return hr;
	}

	SFGAOF attr = SFGAO_FILESYSTEM;
	hr = parent->GetAttributesOf(1, &child, &attr);

	if (FAILED(hr))
	{
		return hr;
	}

	std::wstring parsingPath;
	hr = GetDisplayName(parent.get(), child, SHGDN_FORPARSING, parsingPath);

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return hr;
	}

	SHCONTF enumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

	if (m_folderSettings.showHidden)
	{
		WI_SetAllFlags(enumFlags, SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN);
	}

	wil::com_ptr_nothrow<IEnumIDList> enumerator;
	hr = shellFolder->EnumObjects(m_hOwner, enumFlags, &enumerator);

	if (FAILED(hr) || !enumerator)
	{
		return hr;
	}

	PrepareToChangeFolders();

	m_directoryState.pidlDirectory.reset(ILCloneFull(pidlDirectory));
	m_directoryState.directory = parsingPath;
	m_directoryState.virtualFolder = WI_IsFlagClear(attr, SFGAO_FILESYSTEM);
	m_uniqueFolderId++;

	m_navigationCommittedSignal(pidlDirectory, addHistoryEntry);

	ULONG numFetched = 1;
	unique_pidl_child pidlItem;

	while (enumerator->Next(1, wil::out_param(pidlItem), &numFetched) == S_OK && (numFetched == 1))
	{
		AddItemInternal(shellFolder.get(), pidlDirectory, pidlItem.get(), -1, FALSE);
	}

	return hr;
}

std::optional<int> ShellBrowser::AddItemInternal(IShellFolder *shellFolder,
	PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild, int itemIndex, BOOL setPosition)
{
	auto itemInfo = GetItemInformation(shellFolder, pidlDirectory, pidlChild);

	if (!itemInfo)
	{
		return std::nullopt;
	}

	return AddItemInternal(itemIndex, std::move(*itemInfo), setPosition);
}

int ShellBrowser::AddItemInternal(int itemIndex, ItemInfo_t itemInfo, BOOL setPosition)
{
	int itemId = GenerateUniqueItemId();
	m_itemInfoMap.insert({ itemId, std::move(itemInfo) });

	AwaitingAdd_t awaitingAdd;

	if (itemIndex == -1)
	{
		awaitingAdd.iItem =
			m_directoryState.numItems + static_cast<int>(m_directoryState.awaitingAddList.size());
	}
	else
	{
		awaitingAdd.iItem = itemIndex;
	}

	awaitingAdd.iItemInternal = itemId;
	awaitingAdd.bPosition = setPosition;
	awaitingAdd.iAfter = itemIndex - 1;

	m_directoryState.awaitingAddList.push_back(awaitingAdd);

	return itemId;
}

std::optional<ShellBrowser::ItemInfo_t> ShellBrowser::GetItemInformation(
	IShellFolder *shellFolder, PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild)
{
	ItemInfo_t itemInfo;

	unique_pidl_absolute pidlItem(ILCombine(pidlDirectory, pidlChild));

	itemInfo.pidlComplete.reset(ILCloneFull(pidlItem.get()));
	itemInfo.pridl.reset(ILCloneChild(pidlChild));

	std::wstring parsingName;
	HRESULT hr = GetDisplayName(shellFolder, pidlChild, SHGDN_FORPARSING, parsingName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	itemInfo.parsingName = parsingName;

	ULONG attributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
	PCITEMID_CHILD items[] = { pidlChild };
	hr = shellFolder->GetAttributesOf(1, items, &attributes);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	SHGDNF displayNameFlags = SHGDN_INFOLDER;

	bool isRecycleBin = m_recycleBinPidl
		&& m_desktopFolder->CompareIDs(SHCIDS_CANONICALONLY, pidlDirectory, m_recycleBinPidl.get())
			== 0;

	// SHGDN_INFOLDER | SHGDN_FORPARSING is used to ensure that the name retrieved for a filesystem
	// file contains an extension, even if extensions are hidden in Windows Explorer. When using
	// SHGDN_INFOLDER by itself, the resulting name won't contain an extension if extensions are
	// hidden in Windows Explorer.
	// Note that the recycle bin is excluded here, as the parsing names for the items are completely
	// different to their regular display names.
	if (!isRecycleBin && WI_IsFlagSet(attributes, SFGAO_FILESYSTEM)
		&& WI_IsFlagClear(attributes, SFGAO_FOLDER))
	{
		WI_SetFlag(displayNameFlags, SHGDN_FORPARSING);
	}

	std::wstring displayName;
	hr = GetDisplayName(shellFolder, pidlChild, displayNameFlags, displayName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	itemInfo.displayName = displayName;

	std::wstring editingName;
	hr = GetDisplayName(shellFolder, pidlChild, SHGDN_INFOLDER | SHGDN_FOREDITING, editingName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	itemInfo.editingName = editingName;

	if (PathIsRoot(parsingName.c_str()))
	{
		itemInfo.bDrive = TRUE;
		StringCchCopy(itemInfo.szDrive, SIZEOF_ARRAY(itemInfo.szDrive), parsingName.c_str());
	}
	else
	{
		itemInfo.bDrive = FALSE;
	}

	WIN32_FIND_DATA wfd;
	hr = SHGetDataFromIDList(shellFolder, pidlChild, SHGDFIL_FINDDATA, &wfd, sizeof(wfd));

	if (FAILED(hr))
	{
		hr = ExtractFindDataUsingPropertyStore(shellFolder, pidlChild, wfd);
	}

	if (SUCCEEDED(hr))
	{
		itemInfo.wfd = wfd;
		itemInfo.isFindDataValid = true;
	}
	else
	{
		StringCchCopy(
			itemInfo.wfd.cFileName, SIZEOF_ARRAY(itemInfo.wfd.cFileName), displayName.c_str());

		if (WI_IsFlagSet(attributes, SFGAO_FOLDER))
		{
			WI_SetFlag(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
		}
	}

	return std::move(itemInfo);
}

HRESULT ShellBrowser::ExtractFindDataUsingPropertyStore(
	IShellFolder *shellFolder, PCITEMID_CHILD pidlChild, WIN32_FIND_DATA &output)
{
	wil::com_ptr_nothrow<IPropertyStoreFactory> factory;
	HRESULT hr = shellFolder->BindToObject(pidlChild, nullptr, IID_PPV_ARGS(&factory));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IPropertyStore> store;
	PROPERTYKEY keys[] = { PKEY_FindData };
	hr = factory->GetPropertyStoreForKeys(
		keys, SIZEOF_ARRAY(keys), GPS_FASTPROPERTIESONLY, IID_PPV_ARGS(&store));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::unique_prop_variant findDataProp;
	hr = store->GetValue(PKEY_FindData, &findDataProp);

	if (FAILED(hr))
	{
		return hr;
	}

	if (PropVariantGetElementCount(findDataProp) != sizeof(WIN32_FIND_DATA))
	{
		return hr;
	}

	WIN32_FIND_DATA wfd;
	hr = PropVariantToBuffer(findDataProp, &wfd, sizeof(wfd));

	if (FAILED(hr))
	{
		return hr;
	}

	output = wfd;

	return hr;
}

void ShellBrowser::InsertAwaitingItems(BOOL bInsertIntoGroup)
{
	int nPrevItems = ListView_GetItemCount(m_hListView);

	if (nPrevItems == 0 && m_directoryState.awaitingAddList.empty())
	{
		if (m_folderSettings.applyFilter)
		{
			ApplyFilteringBackgroundImage(true);
		}
		else
		{
			ApplyFolderEmptyBackgroundImage(true);
		}

		m_directoryState.numItems = 0;

		return;
	}
	else if (!m_folderSettings.applyFilter)
	{
		ApplyFolderEmptyBackgroundImage(false);
	}

	/* Make the listview allocate space (for internal data structures)
	for all the items at once, rather than individually.
	Acts as a speed optimization. */
	ListView_SetItemCount(m_hListView, m_directoryState.awaitingAddList.size() + nPrevItems);

	if (m_folderSettings.autoArrange)
	{
		ListViewHelper::SetAutoArrange(m_hListView, FALSE);
	}

	int nAdded = 0;
	std::optional<int> itemToRename;

	for (const auto &awaitingItem : m_directoryState.awaitingAddList)
	{
		const auto &itemInfo = m_itemInfoMap.at(awaitingItem.iItemInternal);

		if (IsFileFiltered(itemInfo))
		{
			m_directoryState.filteredItemsList.insert(awaitingItem.iItemInternal);
			continue;
		}

		BasicItemInfo_t basicItemInfo = getBasicItemInfo(awaitingItem.iItemInternal);
		std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);

		LVITEM lv;
		lv.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

		if (bInsertIntoGroup)
		{
			int groupId = DetermineItemGroup(awaitingItem.iItemInternal);

			lv.mask |= LVIF_GROUPID;
			lv.iGroupId = groupId;

			EnsureGroupExistsInListView(groupId);
		}

		lv.iItem = awaitingItem.iItem;
		lv.iSubItem = 0;

		auto firstColumn = GetFirstCheckedColumn();

		if ((m_folderSettings.viewMode == +ViewMode::Details)
			&& firstColumn.type != ColumnType::Name)
		{
			lv.pszText = LPSTR_TEXTCALLBACK;
		}
		else
		{
			lv.pszText = filename.data();
		}

		lv.iImage = I_IMAGECALLBACK;
		lv.lParam = awaitingItem.iItemInternal;

		/* Insert the item into the list view control. */
		int iItemIndex = ListView_InsertItem(m_hListView, &lv);

		if (awaitingItem.bPosition && m_folderSettings.viewMode != +ViewMode::Details)
		{
			POINT ptItem;

			if (awaitingItem.iAfter != -1)
			{
				ListView_GetItemPosition(m_hListView, awaitingItem.iAfter, &ptItem);
			}
			else
			{
				ptItem.x = 0;
				ptItem.y = 0;
			}

			/* The item will end up in the position AFTER iAfter. */
			ListView_SetItemPosition32(m_hListView, iItemIndex, ptItem.x, ptItem.y);
		}

		if (m_folderSettings.viewMode == +ViewMode::Tiles)
		{
			SetTileViewItemInfo(iItemIndex, awaitingItem.iItemInternal);
		}

		if (m_queuedRenameItem
			&& ArePidlsEquivalent(itemInfo.pidlComplete.get(), m_queuedRenameItem.get()))
		{
			itemToRename = iItemIndex;
		}

		/* If the file is marked as hidden, ghost it out. */
		if (itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			ListView_SetItemState(m_hListView, iItemIndex, LVIS_CUT, LVIS_CUT);
		}

		/* Add the current file's size to the running size of the current directory. */
		/* A folder may or may not have 0 in its high file size member.
		It should either be zeroed, or never counted. */
		ULARGE_INTEGER ulFileSize;
		ulFileSize.LowPart = itemInfo.wfd.nFileSizeLow;
		ulFileSize.HighPart = itemInfo.wfd.nFileSizeHigh;

		m_directoryState.totalDirSize.QuadPart += ulFileSize.QuadPart;

		nAdded++;
	}

	if (m_folderSettings.autoArrange)
	{
		ListViewHelper::SetAutoArrange(m_hListView, TRUE);
	}

	m_directoryState.numItems = nPrevItems + nAdded;

	PositionDroppedItems();

	m_directoryState.awaitingAddList.clear();

	if (itemToRename)
	{
		m_queuedRenameItem.reset();
		ListView_EditLabel(m_hListView, *itemToRename);
	}
}

void ShellBrowser::ApplyFolderEmptyBackgroundImage(bool apply)
{
	if (apply)
	{
		ListViewHelper::SetBackgroundImage(m_hListView, IDB_FOLDEREMPTY);
	}
	else
	{
		ListViewHelper::SetBackgroundImage(m_hListView, NULL);
	}
}

BOOL ShellBrowser::IsFileFiltered(const ItemInfo_t &itemInfo) const
{
	BOOL bHideSystemFile = FALSE;
	BOOL bFilenameFiltered = FALSE;

	if (m_folderSettings.applyFilter
		&& ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
	{
		bFilenameFiltered = IsFilenameFiltered(itemInfo.displayName.c_str());
	}

	if (m_config->globalFolderSettings.hideSystemFiles)
	{
		bHideSystemFile =
			(itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == FILE_ATTRIBUTE_SYSTEM;
	}

	return bFilenameFiltered || bHideSystemFile;
}

void ShellBrowser::RemoveItem(int iItemInternal)
{
	ULARGE_INTEGER ulFileSize;
	LVFINDINFO lvfi;
	BOOL bFolder;
	int iItem;
	int nItems;

	if (iItemInternal == -1)
	{
		return;
	}

	/* Is this item a folder? */
	bFolder = (m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		== FILE_ATTRIBUTE_DIRECTORY;

	/* Take the file size of the removed file away from the total
	directory size. */
	ulFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
	ulFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

	m_directoryState.totalDirSize.QuadPart -= ulFileSize.QuadPart;

	/* Locate the item within the listview.
	Could use filename, providing removed
	items are always deleted before new
	items are inserted. */
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = iItemInternal;
	iItem = ListView_FindItem(m_hListView, -1, &lvfi);

	if (iItem != -1)
	{
		if (m_folderSettings.showInGroups)
		{
			auto groupId = GetItemGroupId(iItem);

			if (groupId)
			{
				OnItemRemovedFromGroup(*groupId);
			}
		}

		/* Remove the item from the listview. */
		ListView_DeleteItem(m_hListView, iItem);
	}

	m_itemInfoMap.erase(iItemInternal);

	nItems = ListView_GetItemCount(m_hListView);

	m_directoryState.numItems--;

	if (nItems == 0 && !m_folderSettings.applyFilter)
	{
		ApplyFolderEmptyBackgroundImage(true);
	}
}

ShellNavigationController *ShellBrowser::GetNavigationController() const
{
	return m_navigationController.get();
}

boost::signals2::connection ShellBrowser::AddNavigationStartedObserver(
	const NavigationStartedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return m_navigationStartedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowser::AddNavigationCommittedObserver(
	const NavigationCommittedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCommittedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowser::AddNavigationCompletedObserver(
	const NavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCompletedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowser::AddNavigationFailedObserver(
	const NavigationFailedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return m_navigationFailedSignal.connect(observer, position);
}