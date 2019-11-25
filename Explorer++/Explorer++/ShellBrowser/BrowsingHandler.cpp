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

HRESULT CShellBrowser::BrowseFolder(const TCHAR *szPath,UINT wFlags)
{
	unique_pidl_absolute pidlDirectory;
	HRESULT hr = SHParseDisplayName(szPath, nullptr, wil::out_param(pidlDirectory), 0, nullptr);

	if(SUCCEEDED(hr))
	{
		hr = BrowseFolder(pidlDirectory.get(),wFlags);
	}

	return hr;
}

HRESULT CShellBrowser::BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, UINT wFlags)
{
	SetCursor(LoadCursor(NULL,IDC_WAIT));

	PIDLIST_ABSOLUTE pidl = ILCloneFull(pidlDirectory);

	if(m_bFolderVisited)
	{
		SaveColumnWidths();
	}

	/* The path may not be absolute, in which case it will
	need to be completed. */
	BOOL StoreHistory = TRUE;
	HRESULT hr = ParsePath(&pidl,wFlags,&StoreHistory);

	if(hr != S_OK)
	{
		SetCursor(LoadCursor(NULL,IDC_ARROW));
		return E_FAIL;
	}

	/* TODO: Wait for any background threads to finish processing. */

	m_columnThreadPool.clear_queue();
	m_columnResults.clear();

	m_itemImageThreadPool.clear_queue();
	m_iconResults.clear();
	m_thumbnailResults.clear();

	m_infoTipsThreadPool.clear_queue();
	m_infoTipResults.clear();

	EnterCriticalSection(&m_csDirectoryAltered);
	m_FilesAdded.clear();
	m_FileSelectionList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	TCHAR szParsingPath[MAX_PATH];
	GetDisplayName(pidl,szParsingPath,SIZEOF_ARRAY(szParsingPath),SHGDN_FORPARSING);

	/* TODO: Method callback. */
	SendMessage(m_hOwner,WM_USER_STARTEDBROWSING,m_ID,reinterpret_cast<WPARAM>(szParsingPath));

	StringCchCopy(m_CurDir,SIZEOF_ARRAY(m_CurDir),szParsingPath);

	if(StoreHistory)
	{
		m_pathManager.StoreIdl(pidl);
	}

	/* Stop the list view from redrawing itself each time is inserted.
	Redrawing will be allowed once all items have being inserted.
	(reduces lag when a large number of items are going to be inserted). */
	SendMessage(m_hListView, WM_SETREDRAW, FALSE, NULL);

	ListView_DeleteAllItems(m_hListView);

	if(m_bFolderVisited)
	{
		ResetFolderMemoryAllocations();
	}

	m_iFolderIcon = GetDefaultFolderIconIndex();
	m_iFileIcon = GetDefaultFileIconIndex();

	m_nTotalItems = 0;

	BrowseVirtualFolder(pidl);

	CoTaskMemFree(pidl);

	/* Window updates needs these to be set. */
	m_NumFilesSelected		= 0;
	m_NumFoldersSelected	= 0;

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

	SetCursor(LoadCursor(NULL,IDC_ARROW));

	m_iUniqueFolderIndex++;

	return S_OK;
}

void CShellBrowser::ResetFolderMemoryAllocations(void)
{
	HIMAGELIST himl;
	HIMAGELIST himlOld;
	int nItems;

	m_directoryState = DirectoryState();

	/* If we're in thumbnails view, destroy the current
	imagelist, and create a new one. */
	if (m_folderSettings.viewMode == +ViewMode::Thumbnails)
	{
		himlOld = ListView_GetImageList(m_hListView, LVSIL_NORMAL);

		nItems = ListView_GetItemCount(m_hListView);

		/* Create and set the new imagelist. */
		himl = ImageList_Create(THUMBNAIL_ITEM_WIDTH, THUMBNAIL_ITEM_HEIGHT,
			ILC_COLOR32, nItems, nItems + 100);
		ListView_SetImageList(m_hListView, himl, LVSIL_NORMAL);

		ImageList_Destroy(himlOld);
	}

	EnterCriticalSection(&m_csDirectoryAltered);
	m_AlteredList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	m_itemInfoMap.clear();
	m_cachedFolderSizes.clear();
	m_FilteredItemsList.clear();
	m_AwaitingAddList.clear();
}

void CShellBrowser::BrowseVirtualFolder(PCIDLIST_ABSOLUTE pidlDirectory)
{
	DetermineFolderVirtual(pidlDirectory);

	wil::com_ptr<IShellFolder> pShellFolder;
	HRESULT hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&pShellFolder));

	if (SUCCEEDED(hr))
	{
		m_directoryState.pidlDirectory.reset(ILCloneFull(pidlDirectory));

		SHCONTF enumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

		if (m_folderSettings.showHidden)
		{
			enumFlags |= SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN;
		}

		wil::com_ptr<IEnumIDList> pEnumIDList;
		hr = pShellFolder->EnumObjects(m_hOwner, enumFlags, &pEnumIDList);

		if (SUCCEEDED(hr) && pEnumIDList)
		{
			ULONG uFetched = 1;
			PITEMID_CHILD rgelt;

			while (pEnumIDList->Next(1, &rgelt, &uFetched) == S_OK && (uFetched == 1))
			{
				ULONG uAttributes = SFGAO_FOLDER;

				pShellFolder->GetAttributesOf(1, const_cast<PCITEMID_CHILD *>(&rgelt), &uAttributes);

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
					hr = pShellFolder->GetDisplayNameOf(rgelt, SHGDN_INFOLDER, &str);
				}
				else
				{
					hr = pShellFolder->GetDisplayNameOf(rgelt, SHGDN_INFOLDER | SHGDN_FORPARSING, &str);
				}

				if (SUCCEEDED(hr))
				{
					TCHAR szFileName[MAX_PATH];
					StrRetToBuf(&str, rgelt, szFileName, SIZEOF_ARRAY(szFileName));

					AddItemInternal(pidlDirectory, rgelt, szFileName, -1, FALSE);
				}

				CoTaskMemFree(rgelt);
			}
		}
	}
}

HRESULT CShellBrowser::AddItemInternal(PCIDLIST_ABSOLUTE pidlDirectory,
	PCITEMID_CHILD pidlChild, const TCHAR *szFileName, int iItemIndex, BOOL bPosition)
{
	int uItemId = SetItemInformation(pidlDirectory, pidlChild, szFileName);
	return AddItemInternal(iItemIndex, uItemId, bPosition);
}

HRESULT CShellBrowser::AddItemInternal(int iItemIndex, int iItemId, BOOL bPosition)
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

int CShellBrowser::SetItemInformation(PCIDLIST_ABSOLUTE pidlDirectory,
	PCITEMID_CHILD pidlChild, const TCHAR *szFileName)
{
	HANDLE			hFirstFile;
	TCHAR			szPath[MAX_PATH];
	int				uItemId;

	uItemId = GenerateUniqueItemId();

	unique_pidl_absolute pidlItem(ILCombine(pidlDirectory, pidlChild));

	m_itemInfoMap[uItemId].pidlComplete.reset(ILCloneFull(pidlItem.get()));
	m_itemInfoMap[uItemId].pridl.reset(ILCloneChild(pidlChild));
	m_itemInfoMap[uItemId].bIconRetrieved = FALSE;
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

void CShellBrowser::InsertAwaitingItems(BOOL bInsertIntoGroup)
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

	for (const auto &item : m_AwaitingAddList)
	{
		if(!IsFileFiltered(item.iItemInternal))
		{
			BasicItemInfo_t basicItemInfo = getBasicItemInfo(item.iItemInternal);
			std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);

			TCHAR filenameCopy[MAX_PATH];
			StringCchCopy(filenameCopy, SIZEOF_ARRAY(filenameCopy), filename.c_str());

			LVITEM lv;
			lv.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

			if (bInsertIntoGroup)
			{
				lv.mask |= LVIF_GROUPID;
				lv.iGroupId = DetermineItemGroup(item.iItemInternal);
			}

			lv.iItem = item.iItem;
			lv.iSubItem = 0;
			lv.pszText = filenameCopy;
			lv.iImage = I_IMAGECALLBACK;
			lv.lParam = item.iItemInternal;

			/* Insert the item into the list view control. */
			int iItemIndex = ListView_InsertItem(m_hListView,&lv);

			if(item.bPosition && m_folderSettings.viewMode != +ViewMode::Details)
			{
				POINT ptItem;

				if(item.iAfter != -1)
				{
					ListView_GetItemPosition(m_hListView, item.iAfter,&ptItem);
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
				SetTileViewItemInfo(iItemIndex, item.iItemInternal);
			}

			if(m_bNewItemCreated)
			{
				if(CompareIdls(m_itemInfoMap.at((int)item.iItemInternal).pidlComplete.get(),m_pidlNewItem))
					m_bNewItemCreated = FALSE;

				m_iIndexNewItem = iItemIndex;
			}

			/* If the file is marked as hidden, ghost it out. */
			if(m_itemInfoMap.at(item.iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			{
				ListView_SetItemState(m_hListView,iItemIndex,LVIS_CUT,LVIS_CUT);
			}
			
			/* Add the current file's size to the running size of the current directory. */
			/* A folder may or may not have 0 in its high file size member.
			It should either be zeroed, or never counted. */
			ULARGE_INTEGER ulFileSize;
			ulFileSize.LowPart = m_itemInfoMap.at(item.iItemInternal).wfd.nFileSizeLow;
			ulFileSize.HighPart = m_itemInfoMap.at(item.iItemInternal).wfd.nFileSizeHigh;

			m_ulTotalDirSize.QuadPart += ulFileSize.QuadPart;

			nAdded++;
		}
		else
		{
			m_FilteredItemsList.push_back(item.iItemInternal);
		}
	}

	if (m_folderSettings.autoArrange)
	{
		NListView::ListView_SetAutoArrange(m_hListView, TRUE);
	}

	m_nTotalItems = nPrevItems + nAdded;

	PositionDroppedItems();

	m_AwaitingAddList.clear();
}

void CShellBrowser::ApplyFolderEmptyBackgroundImage(bool apply)
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

void CShellBrowser::ApplyFilteringBackgroundImage(bool apply)
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

BOOL CShellBrowser::IsFileFiltered(int iItemInternal) const
{
	BOOL bHideSystemFile	= FALSE;
	BOOL bFilenameFiltered	= FALSE;

	if(m_folderSettings.applyFilter &&
		((m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
	{
		bFilenameFiltered = IsFilenameFiltered(m_itemInfoMap.at(iItemInternal).szDisplayName);
	}

	if(m_config->globalFolderSettings.hideSystemFiles)
	{
		bHideSystemFile = (m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
			== FILE_ATTRIBUTE_SYSTEM;
	}

	return bFilenameFiltered || bHideSystemFile;
}

void CShellBrowser::RemoveItem(int iItemInternal)
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

HRESULT CShellBrowser::ParsePath(LPITEMIDLIST *pidlDirectory,UINT uFlags,
BOOL *bStoreHistory)
{
	if((uFlags & SBSP_RELATIVE) == SBSP_RELATIVE)
	{
		if(pidlDirectory == NULL)
			return E_INVALIDARG;

		/* This is a relative path. Add it on to the end of the current directory
		name to get a fully qualified path. */
		PIDLIST_ABSOLUTE pidlComplete = ILCombine(m_directoryState.pidlDirectory.get(),*pidlDirectory);

		*pidlDirectory = ILClone(pidlComplete);

		CoTaskMemFree(pidlComplete);
	}
	else if((uFlags & SBSP_PARENT) == SBSP_PARENT)
	{
		HRESULT hr;

		hr = GetVirtualParentPath(m_directoryState.pidlDirectory.get(),pidlDirectory);
	}
	else if((uFlags & SBSP_NAVIGATEBACK) == SBSP_NAVIGATEBACK)
	{
		if(m_pathManager.GetNumBackPathsStored() == 0)
		{
			SetFocus(m_hListView);
			return E_FAIL;
		}

		/*Gets the path of the folder that was last visited.
		Ignores the supplied Path argument.*/
		*bStoreHistory		= FALSE;

		*pidlDirectory = m_pathManager.RetrieveAndValidateIdl(-1);
	}
	else if((uFlags & SBSP_NAVIGATEFORWARD) == SBSP_NAVIGATEFORWARD)
	{
		if(m_pathManager.GetNumForwardPathsStored() == 0)
		{
			SetFocus(m_hListView);
			return E_FAIL;
		}

		/*Gets the path of the folder that is 'forward' of
		this one. Ignores the supplied Path argument.*/
		*bStoreHistory		= FALSE;

		*pidlDirectory = m_pathManager.RetrieveAndValidateIdl(1);
	}
	else
	{
		/* Assume that SBSP_ABSOLUTE was passed. */
		if(pidlDirectory == NULL)
			return E_INVALIDARG;
	}
	
	if((uFlags & SBSP_WRITENOHISTORY) == SBSP_WRITENOHISTORY)
	{
		/* Client has requested that the folder to be browsed to will have
		no history item associated with it. */
		*bStoreHistory		= FALSE;
	}

	if(!CheckIdl(*pidlDirectory))
		return E_FAIL;

	return S_OK;
}

HRESULT CShellBrowser::Refresh()
{
	return BrowseFolder(m_directoryState.pidlDirectory.get(), SBSP_ABSOLUTE | SBSP_WRITENOHISTORY);
}