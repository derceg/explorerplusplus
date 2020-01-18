// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "MainResource.h"
#include "ViewModes.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <list>

HRESULT ShellBrowser::BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)
{
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	auto resetCursor = wil::scope_exit([] {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	});

	if(m_bFolderVisited)
	{
		SaveColumnWidths();
	}

	ClearPendingResults();

	EnterCriticalSection(&m_csDirectoryAltered);
	m_FilesAdded.clear();
	m_FileSelectionList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	TCHAR szParsingPath[MAX_PATH];
	GetDisplayName(pidlDirectory,szParsingPath,SIZEOF_ARRAY(szParsingPath),SHGDN_FORPARSING);

	/* TODO: Method callback. */
	SendMessage(m_hOwner,WM_USER_STARTEDBROWSING,m_ID,reinterpret_cast<WPARAM>(szParsingPath));

	StringCchCopy(m_CurDir,SIZEOF_ARRAY(m_CurDir),szParsingPath);

	/* Stop the list view from redrawing itself each time is inserted.
	Redrawing will be allowed once all items have being inserted.
	(reduces lag when a large number of items are going to be inserted). */
	SendMessage(m_hListView, WM_SETREDRAW, FALSE, NULL);

	ListView_DeleteAllItems(m_hListView);

	if(m_bFolderVisited)
	{
		ResetFolderState();
	}

	m_nTotalItems = 0;

	EnumerateFolder(pidlDirectory);

	/* Window updates needs these to be set. */
	m_NumFilesSelected = 0;
	m_NumFoldersSelected = 0;

	m_ulTotalDirSize.QuadPart = 0;
	m_ulFileSelectionSize.QuadPart = 0;

	SetActiveColumnSet();
	SetViewModeInternal(m_folderSettings.viewMode);

	InsertAwaitingItems(FALSE);

	VerifySortMode();
	SortFolder(m_folderSettings.sortMode);

	ListView_EnsureVisible(m_hListView,0,FALSE);

	/* Allow the listview to redraw itself once again. */
	SendMessage(m_hListView,WM_SETREDRAW,TRUE,NULL);

	/* Set the focus back to the first item. */
	ListView_SetItemState(m_hListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);

	m_bFolderVisited = TRUE;

	PlayNavigationSound();

	m_uniqueFolderId++;

	m_navigationCompletedSignal(pidlDirectory, addHistoryEntry);

	return S_OK;
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
		HIMAGELIST himlOld = ListView_GetImageList(m_hListView, LVSIL_NORMAL);

		int nItems = ListView_GetItemCount(m_hListView);

		/* Create and set the new imagelist. */
		HIMAGELIST himl = ImageList_Create(THUMBNAIL_ITEM_WIDTH, THUMBNAIL_ITEM_HEIGHT,
			ILC_COLOR32, nItems, nItems + 100);
		ListView_SetImageList(m_hListView, himl, LVSIL_NORMAL);

		ImageList_Destroy(himlOld);
	}

	m_directoryState = DirectoryState();

	EnterCriticalSection(&m_csDirectoryAltered);
	m_AlteredList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	m_itemInfoMap.clear();
	m_cachedFolderSizes.clear();
	m_FilteredItemsList.clear();
	m_AwaitingAddList.clear();
}

HRESULT ShellBrowser::EnumerateFolder(PCIDLIST_ABSOLUTE pidlDirectory)
{
	DetermineFolderVirtual(pidlDirectory);

	wil::com_ptr<IShellFolder> pShellFolder;
	HRESULT hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&pShellFolder));

	if (FAILED(hr))
	{
		return hr;
	}

	m_directoryState.pidlDirectory.reset(ILCloneFull(pidlDirectory));

	SHCONTF enumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

	if (m_folderSettings.showHidden)
	{
		enumFlags |= SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN;
	}

	wil::com_ptr<IEnumIDList> pEnumIDList;
	hr = pShellFolder->EnumObjects(m_hOwner, enumFlags, &pEnumIDList);

	if (FAILED(hr) || !pEnumIDList)
	{
		return hr;
	}

	ULONG uFetched = 1;
	unique_pidl_child pidlItem;

	while (pEnumIDList->Next(1, wil::out_param(pidlItem), &uFetched) == S_OK && (uFetched == 1))
	{
		ULONG uAttributes = SFGAO_FOLDER;
		PCITEMID_CHILD items[] = { pidlItem.get() };
		pShellFolder->GetAttributesOf(1, items, &uAttributes);

		STRRET str;

		/* If this is a virtual folder, only use SHGDN_INFOLDER. If this is
		a real folder, combine SHGDN_INFOLDER with SHGDN_FORPARSING. This is
		so that items in real folders can still be shown with extensions, even
		if the global, Explorer option is disabled.
		Also use only SHGDN_INFOLDER if this item is a folder. This is to ensure
		that specific folders in Windows 7 (those under C:\Users\Username) appear
		correctly. */
		if (m_bVirtualFolder || (uAttributes & SFGAO_FOLDER))
		{
			hr = pShellFolder->GetDisplayNameOf(pidlItem.get(), SHGDN_INFOLDER, &str);
		}
		else
		{
			hr = pShellFolder->GetDisplayNameOf(pidlItem.get(), SHGDN_INFOLDER | SHGDN_FORPARSING, &str);
		}

		if (SUCCEEDED(hr))
		{
			TCHAR szFileName[MAX_PATH];
			StrRetToBuf(&str, pidlItem.get(), szFileName, SIZEOF_ARRAY(szFileName));

			AddItemInternal(pidlDirectory, pidlItem.get(), szFileName, -1, FALSE);
		}
	}

	return hr;
}

HRESULT ShellBrowser::AddItemInternal(PCIDLIST_ABSOLUTE pidlDirectory,
	PCITEMID_CHILD pidlChild, const TCHAR *szFileName, int iItemIndex, BOOL bPosition)
{
	int uItemId = SetItemInformation(pidlDirectory, pidlChild, szFileName);
	return AddItemInternal(iItemIndex, uItemId, bPosition);
}

HRESULT ShellBrowser::AddItemInternal(int iItemIndex, int iItemId, BOOL bPosition)
{
	AwaitingAdd_t AwaitingAdd;

	if (iItemIndex == -1)
	{
		AwaitingAdd.iItem = m_nTotalItems + static_cast<int>(m_AwaitingAddList.size());
	}
	else
	{
		AwaitingAdd.iItem = iItemIndex;
	}

	AwaitingAdd.iItemInternal = iItemId;
	AwaitingAdd.bPosition = bPosition;
	AwaitingAdd.iAfter = iItemIndex - 1;

	m_AwaitingAddList.push_back(AwaitingAdd);

	return S_OK;
}

int ShellBrowser::SetItemInformation(PCIDLIST_ABSOLUTE pidlDirectory,
	PCITEMID_CHILD pidlChild, const TCHAR *szFileName)
{
	HANDLE			hFirstFile;
	TCHAR			szPath[MAX_PATH];
	int				uItemId;

	uItemId = GenerateUniqueItemId();

	unique_pidl_absolute pidlItem(ILCombine(pidlDirectory, pidlChild));

	m_itemInfoMap[uItemId].pidlComplete.reset(ILCloneFull(pidlItem.get()));
	m_itemInfoMap[uItemId].pridl.reset(ILCloneChild(pidlChild));
	StringCchCopy(m_itemInfoMap[uItemId].szDisplayName,
		SIZEOF_ARRAY(m_itemInfoMap[uItemId].szDisplayName), szFileName);

	SHGetPathFromIDList(pidlItem.get(), szPath);

	/* DO NOT call FindFirstFile() on root drives (especially
	floppy drives). Doing so may cause a delay of up to a
	few seconds. */
	if (!PathIsRoot(szPath))
	{
		m_itemInfoMap[uItemId].bDrive = FALSE;

		WIN32_FIND_DATA wfd;
		hFirstFile = FindFirstFile(szPath, &wfd);

		m_itemInfoMap[uItemId].wfd = wfd;
	}
	else
	{
		m_itemInfoMap[uItemId].bDrive = TRUE;
		StringCchCopy(m_itemInfoMap[uItemId].szDrive,
			SIZEOF_ARRAY(m_itemInfoMap[uItemId].szDrive),
			szPath);

		hFirstFile = INVALID_HANDLE_VALUE;
	}

	/* Need to use this, since may be in a virtual folder
	(such as the recycle bin), but items still exist. */
	if (hFirstFile != INVALID_HANDLE_VALUE)
	{
		FindClose(hFirstFile);
	}
	else
	{
		WIN32_FIND_DATA wfd;

		StringCchCopy(wfd.cFileName, SIZEOF_ARRAY(wfd.cFileName), szFileName);
		wfd.nFileSizeLow = 0;
		wfd.nFileSizeHigh = 0;
		wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		m_itemInfoMap[uItemId].wfd = wfd;
	}

	return uItemId;
}

void ShellBrowser::InsertAwaitingItems(BOOL bInsertIntoGroup)
{
	int nPrevItems = ListView_GetItemCount(m_hListView);

	if(nPrevItems == 0 && m_AwaitingAddList.empty())
	{
		if(m_folderSettings.applyFilter)
			ApplyFilteringBackgroundImage(true);
		else
			ApplyFolderEmptyBackgroundImage(true);

		m_nTotalItems = 0;

		return;
	}
	else if(!m_folderSettings.applyFilter)
	{
		ApplyFolderEmptyBackgroundImage(false);
	}

	/* Make the listview allocate space (for internal data structures)
	for all the items at once, rather than individually.
	Acts as a speed optimization. */
	ListView_SetItemCount(m_hListView,m_AwaitingAddList.size() + nPrevItems);

	if (m_folderSettings.autoArrange)
	{
		NListView::ListView_SetAutoArrange(m_hListView, FALSE);
	}

	int nAdded = 0;

	for (const auto &awaitingItem : m_AwaitingAddList)
	{
		const auto &itemInfo = m_itemInfoMap.at(awaitingItem.iItemInternal);

		if (IsFileFiltered(itemInfo))
		{
			m_FilteredItemsList.push_back(awaitingItem.iItemInternal);
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
		lv.pszText = filename.data();
		lv.iImage = I_IMAGECALLBACK;
		lv.lParam = awaitingItem.iItemInternal;

		/* Insert the item into the list view control. */
		int iItemIndex = ListView_InsertItem(m_hListView,&lv);

		if(awaitingItem.bPosition && m_folderSettings.viewMode != +ViewMode::Details)
		{
			POINT ptItem;

			if(awaitingItem.iAfter != -1)
			{
				ListView_GetItemPosition(m_hListView, awaitingItem.iAfter,&ptItem);
			}
			else
			{
				ptItem.x = 0;
				ptItem.y = 0;
			}

			/* The item will end up in the position AFTER iAfter. */
			ListView_SetItemPosition32(m_hListView,iItemIndex,ptItem.x,ptItem.y);
		}

		if(m_folderSettings.viewMode == +ViewMode::Tiles)
		{
			SetTileViewItemInfo(iItemIndex, awaitingItem.iItemInternal);
		}

		if(m_bNewItemCreated)
		{
			if(CompareIdls(itemInfo.pidlComplete.get(),m_pidlNewItem))
				m_bNewItemCreated = FALSE;

			m_iIndexNewItem = iItemIndex;
		}

		/* If the file is marked as hidden, ghost it out. */
		if(itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			ListView_SetItemState(m_hListView,iItemIndex,LVIS_CUT,LVIS_CUT);
		}
			
		/* Add the current file's size to the running size of the current directory. */
		/* A folder may or may not have 0 in its high file size member.
		It should either be zeroed, or never counted. */
		ULARGE_INTEGER ulFileSize;
		ulFileSize.LowPart = itemInfo.wfd.nFileSizeLow;
		ulFileSize.HighPart = itemInfo.wfd.nFileSizeHigh;

		m_ulTotalDirSize.QuadPart += ulFileSize.QuadPart;

		nAdded++;
	}

	if (m_folderSettings.autoArrange)
	{
		NListView::ListView_SetAutoArrange(m_hListView, TRUE);
	}

	m_nTotalItems = nPrevItems + nAdded;

	PositionDroppedItems();

	m_AwaitingAddList.clear();
}

void ShellBrowser::ApplyFolderEmptyBackgroundImage(bool apply)
{
	if (apply)
	{
		NListView::ListView_SetBackgroundImage(m_hListView, IDB_FOLDEREMPTY);
	}
	else
	{
		NListView::ListView_SetBackgroundImage(m_hListView, NULL);
	}
}

void ShellBrowser::ApplyFilteringBackgroundImage(bool apply)
{
	if (apply)
	{
		NListView::ListView_SetBackgroundImage(m_hListView, IDB_FILTERINGAPPLIED);
	}
	else
	{
		NListView::ListView_SetBackgroundImage(m_hListView, NULL);
	}
}

BOOL ShellBrowser::IsFileFiltered(const ItemInfo_t &itemInfo) const
{
	BOOL bHideSystemFile	= FALSE;
	BOOL bFilenameFiltered	= FALSE;

	if(m_folderSettings.applyFilter &&
		((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
	{
		bFilenameFiltered = IsFilenameFiltered(itemInfo.szDisplayName);
	}

	if(m_config->globalFolderSettings.hideSystemFiles)
	{
		bHideSystemFile = (itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
			== FILE_ATTRIBUTE_SYSTEM;
	}

	return bFilenameFiltered || bHideSystemFile;
}

void ShellBrowser::RemoveItem(int iItemInternal)
{
	ULARGE_INTEGER	ulFileSize;
	LVFINDINFO		lvfi;
	BOOL			bFolder;
	int				iItem;
	int				nItems;

	if(iItemInternal == -1)
		return;

	/* Is this item a folder? */
	bFolder = (m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
	FILE_ATTRIBUTE_DIRECTORY;

	/* Take the file size of the removed file away from the total
	directory size. */
	ulFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
	ulFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

	m_ulTotalDirSize.QuadPart -= ulFileSize.QuadPart;

	/* Locate the item within the listview.
	Could use filename, providing removed
	items are always deleted before new
	items are inserted. */
	lvfi.flags	= LVFI_PARAM;
	lvfi.lParam	= iItemInternal;
	iItem = ListView_FindItem(m_hListView,-1,&lvfi);
	
	if(iItem != -1)
	{
		/* Remove the item from the listview. */
		ListView_DeleteItem(m_hListView,iItem);
	}

	m_itemInfoMap.erase(iItemInternal);

	nItems = ListView_GetItemCount(m_hListView);

	m_nTotalItems--;

	if(nItems == 0 && !m_folderSettings.applyFilter)
	{
		ApplyFolderEmptyBackgroundImage(true);
	}
}

void ShellBrowser::PlayNavigationSound() const
{
	if (m_config->playNavigationSound)
	{
		PlaySound(MAKEINTRESOURCE(IDR_WAVE_NAVIGATIONSTART), NULL,
			SND_RESOURCE | SND_ASYNC);
	}
}

NavigationController *ShellBrowser::GetNavigationController() const
{
	return m_navigationController.get();
}

boost::signals2::connection ShellBrowser::AddNavigationCompletedObserver(const NavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCompletedSignal.connect(observer, position);
}