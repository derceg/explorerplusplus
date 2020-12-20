// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ViewModes.h"
#include "../Helper/Helper.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <wil/com.h>
#include <commctrl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <list>

HRESULT ShellBrowser::BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)
{
	if (m_bFolderVisited)
	{
		SaveColumnWidths();
	}

	ClearPendingResults();

	EnterCriticalSection(&m_csDirectoryAltered);
	m_FilesAdded.clear();
	m_FileSelectionList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	ListView_DeleteAllItems(m_hListView);

	if (m_bFolderVisited)
	{
		ResetFolderState();
	}

	// When a folder is entered, the PIDL is roughly the only thing that's immediately stored. That
	// way, the tab will act as if it's in the specified directory, even if the enumeration is still
	// in progress.
	m_directoryState.pidlDirectory.reset(ILCloneFull(pidlDirectory));

	std::wstring parsingPath;
	HRESULT hr = GetDisplayName(pidlDirectory, SHGDN_FORPARSING, parsingPath);

	if (FAILED(hr))
	{
		m_status = Status::Failed;
		m_navigationFailedSignal();
		return hr;
	}

	m_directoryState.directory = parsingPath;

	SetActiveColumnSet();
	VerifySortMode();
	SetViewModeInternal(m_folderSettings.viewMode);

	m_uniqueFolderId++;

	m_status = Status::Loading;

	hr = StartEnumeration(pidlDirectory);

	if (FAILED(hr))
	{
		m_status = Status::Failed;
		m_navigationFailedSignal();
		return hr;
	}

	m_bFolderVisited = TRUE;

	m_navigationStartedSignal(pidlDirectory, addHistoryEntry);

	return hr;
}

void ShellBrowser::ClearPendingResults()
{
	m_enumerationThreadPool.clear_queue();
	m_enumerationResults.clear();

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

HRESULT ShellBrowser::StartEnumeration(PCIDLIST_ABSOLUTE pidlDirectory)
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

	m_directoryState.virtualFolder = WI_IsFlagClear(attr, SFGAO_FILESYSTEM);

	unique_pidl_absolute_holder copiedPidlDirectory;
	copiedPidlDirectory.pidl.reset(ILCloneFull(pidlDirectory));
	bool showHidden = m_folderSettings.showHidden;
	int resultId = m_uniqueFolderId;

	auto result =
		m_enumerationThreadPool.push([this, copiedPidlDirectory, showHidden, resultId](int id) {
			UNREFERENCED_PARAMETER(id);

			// This will force a message loop to be created for this thread. See:
			//
			// https://stackoverflow.com/a/2850320
			MSG msg;
			PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

			auto result = EnumerateFolderAsync(m_hListView, resultId, m_closing, m_uniqueFolderId,
				copiedPidlDirectory.pidl.get(), m_hOwner, showHidden);

			// A core requirement of a single-threaded apartment is that the associated thread pumps
			// messages:
			//
			// https://docs.microsoft.com/en-us/windows/win32/learnwin32/initializing-the-com-library
			//
			// That's something that's required in order to be able to marshal objects between
			// threads. Although EnumerateFolderAsync() will only ever invoke objects from a single
			// thread (the current thread), it's possible that the implementation might internally
			// share objects between threads.
			// As far as I'm aware, that's not the case, but it's still at least possible, meaning
			// it's important to pump messages.
			// Additionally, pumping messages may be important if there are any hidden windows
			// created that rely on those messages. Again, I'm not sure that that's the case.
			// Part of the requirement around pumping messages appears to be that the messages are
			// pumped in a timely manner (i.e. the calling thread doesn't block for a significant
			// period of time). However, I don't see how that's achievable in this case.
			// Shell objects generally can't be invoked in multithreaded apartments:
			//
			// https://docs.microsoft.com/en-us/troubleshoot/windows/win32/shell-functions-multithreaded-apartment
			//
			// That means that to use the shell interfaces, you need a single-threaded apartment.
			// Which then means you need to pump messages in a timely manner. However, a call to
			// IEnumIDList::Next() can take an arbitrary amount of time to return. During that time,
			// it's not possible to pump messages on the calling thread and there's no way around
			// that.
			// I'm not aware of a specific situation in which pumping messages is required when
			// using IEnumIDList. That is, where not pumping messages would cause a deadlock or one
			// of the object invocations to fail. But, since pumping messages is documented as a
			// requirement, emptying the message queue here should fulfill that.
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			return result;
		});

	m_enumerationResults.insert({ resultId, std::move(result) });

	// This matches the behavior of the WM_SETCURSOR handler. HTCLIENT will only be set when the
	// cursor is over the listview, not the listview header.
	if (CursorInWindowClientArea(m_hListView)
		&& !CursorInWindowClientArea(ListView_GetHeader(m_hListView)))
	{
		SetCursor(LoadCursor(nullptr, IDC_WAIT));
	}

	return hr;
}

ShellBrowser::EnumerationResult ShellBrowser::EnumerateFolderAsync(HWND listView, int resultId,
	const std::atomic_bool &closing, const std::atomic_int &currentResultId,
	PCIDLIST_ABSOLUTE pidlDirectory, HWND owner, bool showHidden)
{
	std::vector<ItemInfo_t> items;
	HRESULT hr = EnumerateFolder(
		pidlDirectory, owner, showHidden, closing, currentResultId, resultId, items);

	PostMessage(listView, WM_APP_ENUMERATION_RESULTS_READY, resultId, 0);

	EnumerationResult result;
	result.hr = hr;
	result.items = std::move(items);

	return result;
}

HRESULT ShellBrowser::EnumerateFolder(PCIDLIST_ABSOLUTE pidlDirectory, HWND owner, bool showHidden,
	const std::atomic_bool &closing, const std::atomic_int &currentResultId, int resultId,
	std::vector<ShellBrowser::ItemInfo_t> &items)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return hr;
	}

	SHCONTF enumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

	if (showHidden)
	{
		WI_SetAllFlags(enumFlags, SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN);
	}

	wil::com_ptr_nothrow<IEnumIDList> enumerator;
	hr = shellFolder->EnumObjects(owner, enumFlags, &enumerator);

	if (FAILED(hr) || !enumerator)
	{
		return hr;
	}

	ULONG numFetched = 1;
	unique_pidl_child pidlItem;

	while (enumerator->Next(1, wil::out_param(pidlItem), &numFetched) == S_OK && (numFetched == 1))
	{
		auto item = GetItemInformation(shellFolder.get(), pidlDirectory, pidlItem.get());

		if (item)
		{
			items.push_back(std::move(*item));
		}

		if (closing || currentResultId != resultId)
		{
			hr = E_ABORT;
			break;
		}
	}

	return hr;
}

std::optional<ShellBrowser::ItemInfo_t> ShellBrowser::GetItemInformation(
	IShellFolder *shellFolder, PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild)
{
	ItemInfo_t newItem;

	unique_pidl_absolute pidlItem(ILCombine(pidlDirectory, pidlChild));

	newItem.pidlComplete.reset(ILCloneFull(pidlItem.get()));
	newItem.pridl.reset(ILCloneChild(pidlChild));

	std::wstring parsingName;
	HRESULT hr = GetDisplayName(shellFolder, pidlChild, SHGDN_FORPARSING, parsingName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	newItem.parsingName = parsingName;

	ULONG attributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
	PCITEMID_CHILD items[] = { pidlChild };
	hr = shellFolder->GetAttributesOf(1, items, &attributes);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	SHGDNF displayNameFlags = SHGDN_INFOLDER;

	wil::com_ptr_nothrow<IShellFolder> desktopFolder;
	SHGetDesktopFolder(&desktopFolder);

	unique_pidl_absolute recycleBinPidl;
	SHGetKnownFolderIDList(
		FOLDERID_RecycleBinFolder, KF_FLAG_DEFAULT, nullptr, wil::out_param(recycleBinPidl));

	bool isRecycleBin = desktopFolder && recycleBinPidl
		&& desktopFolder->CompareIDs(SHCIDS_CANONICALONLY, pidlDirectory, recycleBinPidl.get())
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

	newItem.displayName = displayName;
	StringCchCopy(newItem.wfd.cFileName, SIZEOF_ARRAY(newItem.wfd.cFileName), displayName.c_str());

	std::wstring editingName;
	hr = GetDisplayName(shellFolder, pidlChild, SHGDN_INFOLDER | SHGDN_FOREDITING, editingName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	newItem.editingName = editingName;

	if (PathIsRoot(parsingName.c_str()))
	{
		newItem.bDrive = TRUE;
		StringCchCopy(newItem.szDrive, SIZEOF_ARRAY(newItem.szDrive), parsingName.c_str());
	}
	else
	{
		newItem.bDrive = FALSE;
	}

	wil::com_ptr_nothrow<IShellFolder2> shellFolder2;
	hr = shellFolder->QueryInterface(IID_PPV_ARGS(&shellFolder2));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	VARIANT sizeVariant;
	hr = shellFolder2->GetDetailsEx(pidlChild, &PKEY_Size, &sizeVariant);

	// Size retrieval may fail for items in virtual folders. Note that it won't always fail in such
	// cases. For example, library folders in Windows are backed by the filesystem. Therefore, a
	// file in a library folder will still have a size, even though the library folder itself is
	// virtual.
	if (SUCCEEDED(hr))
	{
		ULONGLONG size;
		hr = VariantToUInt64(sizeVariant, &size);

		if (SUCCEEDED(hr))
		{
			ULARGE_INTEGER largeFileSize;
			largeFileSize.QuadPart = size;
			newItem.wfd.nFileSizeLow = largeFileSize.LowPart;
			newItem.wfd.nFileSizeHigh = largeFileSize.HighPart;
		}
	}

	FILETIME dateAccessed;
	hr = GetDateDetailsEx(shellFolder2.get(), pidlChild, &PKEY_DateAccessed, dateAccessed);

	// If the date set on the individual item is invalid, the date retrieval will fail, even though
	// there are no actual issues with the item. In other words, it's valid for the date retrieval
	// to fail, even for filesystem files.
	if (SUCCEEDED(hr))
	{
		newItem.wfd.ftLastAccessTime = dateAccessed;
	}

	FILETIME dateCreated;
	hr = GetDateDetailsEx(shellFolder2.get(), pidlChild, &PKEY_DateCreated, dateCreated);

	if (SUCCEEDED(hr))
	{
		newItem.wfd.ftCreationTime = dateCreated;
	}

	FILETIME dateModified;
	hr = GetDateDetailsEx(shellFolder2.get(), pidlChild, &PKEY_DateModified, dateModified);

	if (SUCCEEDED(hr))
	{
		newItem.wfd.ftLastWriteTime = dateModified;
	}

	// The attribute retrieval below may fail, but, at the very least, it's important to know
	// whether or not an item is a folder. The attributes set here will be used if the attribute
	// retrieval fails.
	if (WI_IsFlagSet(attributes, SFGAO_FOLDER))
	{
		WI_SetFlag(newItem.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
	}

	// Although it's possible to retrieve attributes for root folders, the attributes aren't useful.
	// For example, the attributes returned for the C:\ folder will be something like:
	//
	// FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN
	//
	// Accepting those attributes would result in the C:\ item (as shown in the "This PC" folder)
	// being grayed out (because it would be considered hidden).
	if (!PathIsRoot(parsingName.c_str()))
	{
		VARIANT attributesVariant;
		hr = shellFolder2->GetDetailsEx(pidlChild, &PKEY_FileAttributes, &attributesVariant);

		// Attribute retrieval may fail for items in virtual folders.
		if (SUCCEEDED(hr))
		{
			UINT fileAttributes;
			hr = VariantToUInt32(attributesVariant, &fileAttributes);

			if (SUCCEEDED(hr))
			{
				newItem.wfd.dwFileAttributes = fileAttributes;
			}
		}
	}

	return std::move(newItem);
}

void ShellBrowser::ProcessEnumerationResults(int resultId)
{
	auto itr = m_enumerationResults.find(resultId);

	if (itr == m_enumerationResults.end())
	{
		// This result is for a previous folder. It can be ignored. Note that the status isn't
		// updated here. Since this result is for a previous folder, it makes no difference whether
		// the enumeration succeeded or failed.
		return;
	}

	// The WM_SETCURSOR handler sets the appropriate wait cursor when loading and the default
	// handler will restore the normal cursor at other times. However, that message may only be sent
	// when the mouse moves. That means that if the mouse doesn't move when a navigation finishes,
	// the cursor may not be reset. To ensure that the normal cursor is restored, the cursor will be
	// set here.
	auto resetCursor = wil::scope_exit([this] {
		if (CursorInWindowClientArea(m_hListView)
			&& !CursorInWindowClientArea(ListView_GetHeader(m_hListView)))
		{
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
		}
	});

	auto result = itr->second.get();

	if (FAILED(result.hr))
	{
		m_status = Status::Failed;
		m_navigationFailedSignal();
		return;
	}

	for (auto &item : result.items)
	{
		AddItemInternal(-1, std::move(item), FALSE);
	}

	/* Stop the list view from redrawing itself each time is inserted.
	Redrawing will be allowed once all items have being inserted.
	(reduces lag when a large number of items are going to be inserted). */
	SendMessage(m_hListView, WM_SETREDRAW, FALSE, NULL);

	InsertAwaitingItems(FALSE);

	SortFolder(m_folderSettings.sortMode);

	ListView_EnsureVisible(m_hListView, 0, FALSE);

	/* Allow the listview to redraw itself once again. */
	SendMessage(m_hListView, WM_SETREDRAW, TRUE, NULL);

	/* Set the focus back to the first item. */
	ListView_SetItemState(m_hListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);

	m_status = Status::Completed;

	m_navigationCompletedSignal();
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

	for (const auto &awaitingItem : m_directoryState.awaitingAddList)
	{
		const auto &itemInfo = m_itemInfoMap.at(awaitingItem.iItemInternal);

		if (IsFileFiltered(itemInfo))
		{
			m_directoryState.filteredItemsList.push_back(awaitingItem.iItemInternal);
			continue;
		}

		BasicItemInfo_t basicItemInfo = getBasicItemInfo(awaitingItem.iItemInternal);
		std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);

		LVITEM lv;
		lv.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

		if (bInsertIntoGroup)
		{
			lv.mask |= LVIF_GROUPID;
			lv.iGroupId = DetermineItemGroup(awaitingItem.iItemInternal);
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

		if (m_bNewItemCreated)
		{
			if (CompareIdls(itemInfo.pidlComplete.get(), m_pidlNewItem))
			{
				m_bNewItemCreated = FALSE;
			}

			m_iIndexNewItem = iItemIndex;
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

void ShellBrowser::ApplyFilteringBackgroundImage(bool apply)
{
	if (apply)
	{
		ListViewHelper::SetBackgroundImage(m_hListView, IDB_FILTERINGAPPLIED);
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