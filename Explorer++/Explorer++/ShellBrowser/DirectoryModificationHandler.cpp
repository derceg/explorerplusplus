// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "ViewModes.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <list>

BOOL g_bNewFileRenamed = FALSE;
static int iRenamedItem;

void ShellBrowser::DirectoryAltered(void)
{
	BOOL bNewItemCreated;

	EnterCriticalSection(&m_csDirectoryAltered);

	bNewItemCreated = m_bNewItemCreated;

	SendMessage(m_hListView,WM_SETREDRAW,(WPARAM)FALSE,(LPARAM)NULL);

	LOG(debug) << _T("ShellBrowser - Starting directory change update for \"") << m_CurDir << _T("\"");

	/* Potential problem:
	After a file is created, it may be renamed shortly afterwards.
	If the rename occurs before the file is added here, the
	addition won't be registered (since technically, the file
	does not exist), and the rename operation will not take place.
	Adding an item that does not exist will corrupt the programs
	state.

	Solution:
	If a file does not exist when adding it, temporarily remember
	its filename. On the next rename operation, if the renamed
	file matches the name of the added file, add the file in-place
	with its new name.
	The operation should NOT be queued, as it is possible that
	other actions for the file will take place before the addition,
	which will again result in an incorrect state.
	*/
	for(const auto &af : m_AlteredList)
	{
		/* Only undertake the modification if the unique folder
		index on the modified item and current folder match up
		(i.e. ensure the directory has not changed since these
		files were modified). */
		if(af.iFolderIndex == m_uniqueFolderId)
		{
			switch(af.dwAction)
			{
			case FILE_ACTION_ADDED:
				LOG(debug) << _T("ShellBrowser - Adding \"") << af.szFileName << _T("\"");
				OnFileActionAdded(af.szFileName);
				break;

			case FILE_ACTION_MODIFIED:
				LOG(debug) << _T("ShellBrowser - Modifying \"") << af.szFileName << _T("\"");
				ModifyItemInternal(af.szFileName);
				break;

			case FILE_ACTION_REMOVED:
				LOG(debug) << _T("ShellBrowser - Removing \"") << af.szFileName << _T("\"");
				RemoveItemInternal(af.szFileName);
				break;

			case FILE_ACTION_RENAMED_OLD_NAME:
				LOG(debug) << _T("ShellBrowser - Old name received \"") << af.szFileName << _T("\"");
				OnFileActionRenamedOldName(af.szFileName);
				break;

			case FILE_ACTION_RENAMED_NEW_NAME:
				LOG(debug) << _T("ShellBrowser - New name received \"") << af.szFileName << _T("\"");
				OnFileActionRenamedNewName(af.szFileName);
				break;
			}
		}
	}

	LOG(debug) << _T("ShellBrowser - Finished directory change update for \"") << m_CurDir << _T("\"");

	SendMessage(m_hListView,WM_SETREDRAW,(WPARAM)TRUE,(LPARAM)NULL);

	/* Ensure the first dropped item is visible. */
	if(m_iDropped != -1)
	{
		if(!ListView_IsItemVisible(m_hListView,m_iDropped))
			ListView_EnsureVisible(m_hListView,m_iDropped,TRUE);

		m_iDropped = -1;
	}

	SendMessage(m_hOwner,WM_USER_DIRECTORYMODIFIED,m_ID,0);

	if(bNewItemCreated && !m_bNewItemCreated)
		SendMessage(m_hOwner,WM_USER_NEWITEMINSERTED,0,m_iIndexNewItem);

	m_AlteredList.clear();

	BOOL bFocusSet = FALSE;
	int iIndex;

	/* Select the specified items, and place the
	focus on the first item. */
	auto itr = m_FileSelectionList.begin();
	while(itr != m_FileSelectionList.end())
	{
		iIndex = LocateFileItemIndex(itr->c_str());

		if(iIndex != -1)
		{
			NListView::ListView_SelectItem(m_hListView,iIndex,TRUE);

			if(!bFocusSet)
			{
				NListView::ListView_FocusItem(m_hListView,iIndex,TRUE);
				ListView_EnsureVisible(m_hListView,iIndex,TRUE);

				bFocusSet = TRUE;
			}

			itr = m_FileSelectionList.erase(itr);
		}
		else
		{
			++itr;
		}
	}

	LeaveCriticalSection(&m_csDirectoryAltered);

	return;
}

void CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(dwTime);

	KillTimer(hwnd,idEvent);

	SendMessage(hwnd,WM_USER_FILESADDED,(WPARAM)idEvent,0);
}

void ShellBrowser::FilesModified(DWORD Action,const TCHAR *FileName,
int EventId,int iFolderIndex)
{
	EnterCriticalSection(&m_csDirectoryAltered);

	SetTimer(m_hOwner,EventId,200,TimerProc);

	AlteredFile_t af;

	StringCchCopy(af.szFileName,SIZEOF_ARRAY(af.szFileName),FileName);
	af.dwAction = Action;
	af.iFolderIndex = iFolderIndex;

	m_AlteredList.push_back(af);

	LeaveCriticalSection(&m_csDirectoryAltered);
}

void ShellBrowser::OnFileActionAdded(const TCHAR *szFileName)
{
	IShellFolder	*pShellFolder = NULL;
	PCITEMID_CHILD	pidlRelative = NULL;
	Added_t			Added;
	TCHAR			FullFileName[MAX_PATH];
	TCHAR			szDisplayName[MAX_PATH];
	STRRET			str;
	BOOL			bFileAdded = FALSE;
	HRESULT hr;

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,szFileName);

	unique_pidl_absolute pidlFull;
	hr = SHParseDisplayName(FullFileName, nullptr, wil::out_param(pidlFull), 0, nullptr);

	/* It is possible that by the time a file is registered here,
	it will have already been renamed. In this the following
	check will fail.
	If the file is not added, store its filename. */
	if(SUCCEEDED(hr))
	{
		hr = SHBindToParent(pidlFull.get(), IID_PPV_ARGS(&pShellFolder), &pidlRelative);

		if(SUCCEEDED(hr))
		{
			/* If this is a virtual folder, only use SHGDN_INFOLDER. If this is
			a real folder, combine SHGDN_INFOLDER with SHGDN_FORPARSING. This is
			so that items in real folders can still be shown with extensions, even
			if the global, Explorer option is disabled. */
			if(m_bVirtualFolder)
				hr = pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_INFOLDER,&str);
			else
				hr = pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_INFOLDER|SHGDN_FORPARSING,&str);

			if(SUCCEEDED(hr))
			{
				StrRetToBuf(&str,pidlRelative,szDisplayName,SIZEOF_ARRAY(szDisplayName));

				std::list<DroppedFile_t>::iterator itr;
				BOOL bDropped = FALSE;

				if(!m_DroppedFileNameList.empty())
				{
					for(itr = m_DroppedFileNameList.begin();itr != m_DroppedFileNameList.end();itr++)
					{
						if(lstrcmp(szDisplayName,itr->szFileName) == 0)
						{
							bDropped = TRUE;
							break;
						}
					}
				}

				/* Only insert the item in its sorted position if it
				wasn't dropped in. */
				if(m_config->globalFolderSettings.insertSorted && !bDropped)
				{
					int iItemId;
					int iSorted;

					iItemId = SetItemInformation(m_directoryState.pidlDirectory.get(),pidlRelative,szDisplayName);

					iSorted = DetermineItemSortedPosition(iItemId);

					AddItemInternal(iSorted,iItemId,TRUE);
				}
				else
				{
					/* Just add the item to the end of the list. */
					AddItemInternal(m_directoryState.pidlDirectory.get(),pidlRelative,szDisplayName,-1,FALSE);
				}
				
				InsertAwaitingItems(m_folderSettings.showInGroups);

				bFileAdded = TRUE;
			}

			pShellFolder->Release();
		}
	}
	
	if(!bFileAdded)
	{
		/* The file does not exist. However, it is possible
		that is was simply renamed shortly after been created.
		Record the filename temporarily (so that it can later
		be added). */
		StringCchCopy(Added.szFileName,SIZEOF_ARRAY(Added.szFileName),szFileName);
		m_FilesAdded.push_back(Added);
	}
}

void ShellBrowser::RemoveItemInternal(const TCHAR *szFileName)
{
	std::list<Added_t>::iterator itr;
	int iItemInternal;
	BOOL bFound = FALSE;

	/* First check if this item is in the queue of awaiting
	items. If it is, remove it. */
	for(itr = m_FilesAdded.begin();itr != m_FilesAdded.end();itr++)
	{
		if(lstrcmp(szFileName,itr->szFileName) == 0)
		{
			m_FilesAdded.erase(itr);
			bFound = TRUE;
			break;
		}
	}

	if(!bFound)
	{
		iItemInternal = LocateFileItemInternalIndex(szFileName);

		if(iItemInternal != -1)
			RemoveItem(iItemInternal);
	}
}

/*
 * Modifies the attributes of an item currently in the listview.
 */
void ShellBrowser::ModifyItemInternal(const TCHAR *FileName)
{
	HANDLE			hFirstFile;
	ULARGE_INTEGER	ulFileSize;
	LVITEM			lvItem;
	TCHAR			FullFileName[MAX_PATH];
	BOOL			bFolder;
	BOOL			res;
	int				iItem;
	int				iItemInternal = -1;

	iItem = LocateFileItemIndex(FileName);

	/* Although an item may not have been added to the listview
	yet, it is critical that its' size still be updated if
	necessary.
	It is possible (and quite likely) that the file add and
	modified messages will be sent in the same group, meaning
	that when the modification message is processed, the item
	is not in the listview, but it still needs to be updated.
	Therefore, instead of searching for items solely in the
	listview, also look through the list of pending file
	additions. */

	if(iItem == -1)
	{
		/* The item doesn't exist in the listview. This can
		happen when a file has been created with a non-zero
		size, but an item has not yet been inserted into
		the listview.
		Search through the list of items waiting to be
		inserted, so that files the have just been created
		can be updated without them residing within the
		listview. */
		std::list<AwaitingAdd_t>::iterator itr;

		for(itr = m_AwaitingAddList.begin();itr!= m_AwaitingAddList.end();itr++)
		{
			if(lstrcmp(m_itemInfoMap.at(itr->iItemInternal).wfd.cFileName,FileName) == 0)
			{
				iItemInternal = itr->iItemInternal;
				break;
			}
		}
	}
	else
	{
		/* The item exists in the listview. Determine its
		internal index from its listview information. */
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		res = ListView_GetItem(m_hListView,&lvItem);

		if(res != FALSE)
			iItemInternal = (int)lvItem.lParam;

		TCHAR szFullFileName[MAX_PATH];
		StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_CurDir);
		PathAppend(szFullFileName,FileName);

		/* When a file is modified, its icon overlay may change.
		This is the case when modifying a file managed by
		TortoiseSVN, for example. */
		SHFILEINFO shfi;
		DWORD_PTR dwRes = SHGetFileInfo(szFullFileName,0,&shfi,sizeof(SHFILEINFO),SHGFI_ICON|SHGFI_OVERLAYINDEX);

		if(dwRes != 0)
		{
			lvItem.mask			= LVIF_STATE;
			lvItem.iItem		= iItem;
			lvItem.iSubItem		= 0;
			lvItem.stateMask	= LVIS_OVERLAYMASK;
			lvItem.state		= INDEXTOOVERLAYMASK(shfi.iIcon >> 24);
			ListView_SetItem(m_hListView,&lvItem);

			DestroyIcon(shfi.hIcon);
		}
	}

	if(iItemInternal != -1)
	{
		/* Is this item a folder? */
		bFolder = (m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
			FILE_ATTRIBUTE_DIRECTORY;

		ulFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
		ulFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

		m_ulTotalDirSize.QuadPart -= ulFileSize.QuadPart;

		if(ListView_GetItemState(m_hListView,iItem,LVIS_SELECTED)
		== LVIS_SELECTED)
		{
			ulFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
			ulFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

			m_ulFileSelectionSize.QuadPart -= ulFileSize.QuadPart;
		}

		StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
		PathAppend(FullFileName,FileName);

		hFirstFile = FindFirstFile(FullFileName,&m_itemInfoMap.at(iItemInternal).wfd);

		if(hFirstFile != INVALID_HANDLE_VALUE)
		{
			ulFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
			ulFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

			m_ulTotalDirSize.QuadPart += ulFileSize.QuadPart;

			if(ListView_GetItemState(m_hListView,iItem,LVIS_SELECTED)
				== LVIS_SELECTED)
			{
				ulFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
				ulFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

				m_ulFileSelectionSize.QuadPart += ulFileSize.QuadPart;
			}

			if((m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ==
				FILE_ATTRIBUTE_HIDDEN)
			{
				ListView_SetItemState(m_hListView,iItem,LVIS_CUT,LVIS_CUT);
			}
			else
				ListView_SetItemState(m_hListView,iItem,0,LVIS_CUT);

			if(m_folderSettings.viewMode == +ViewMode::Details)
			{
				if(m_pActiveColumns != NULL)
				{
					for(auto itrColumn = m_pActiveColumns->begin();itrColumn != m_pActiveColumns->end();itrColumn++)
					{
						if(itrColumn->bChecked)
						{
							QueueColumnTask(iItemInternal, itrColumn->id);
						}
					}
				}
			}

			FindClose(hFirstFile);
		}
		else
		{
			/* The file may not exist if, for example, it was
			renamed just after a file with the same name was
			deleted. If this does happen, a modification
			message will likely be sent out after the file
			has been renamed, indicating the new items properties.
			However, the files' size will be subtracted on
			modification. If the internal structures still hold
			the old size, the total directory size will become
			corrupted. */
			m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow = 0;
			m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh = 0;
		}
	}
}

void ShellBrowser::OnFileActionRenamedOldName(const TCHAR *szFileName)
{
	std::list<Added_t>::iterator itrAdded;
	BOOL bFileHandled = FALSE;

	/* Loop through each file that is awaiting add to check for the
	renamed file. */
	for(itrAdded = m_FilesAdded.begin();itrAdded != m_FilesAdded.end();itrAdded++)
	{
		if(lstrcmp(szFileName,itrAdded->szFileName) == 0)
		{
			bFileHandled = TRUE;

			g_bNewFileRenamed = TRUE;

			m_FilesAdded.erase(itrAdded);

			break;
		}
	}

	if(!bFileHandled)
	{
		/* Find the index of the item that was renamed...
		Store the index so that it is known which item needs
		renaming when the files new name is received. */
		iRenamedItem = LocateFileItemInternalIndex(szFileName);
	}
}

void ShellBrowser::OnFileActionRenamedNewName(const TCHAR *szFileName)
{
	if(g_bNewFileRenamed)
	{
		/* The file that was previously added was renamed before
		it could be added. Add the file now. */
		OnFileActionAdded(szFileName);

		g_bNewFileRenamed = FALSE;
	}
	else
	{
		RenameItem(iRenamedItem,szFileName);
	}
}

/* Renames an item currently in the listview.
 */
/* TODO: This code should be coalesced with the code that
adds items as well as the code that finds their icons.
ALL changes to an items name/internal properties/icon/overlay icon
should go through a central function. */
void ShellBrowser::RenameItem(int iItemInternal,const TCHAR *szNewFileName)
{
	IShellFolder	*pShellFolder = NULL;
	PCITEMID_CHILD	pidlRelative = NULL;
	SHFILEINFO		shfi;
	LVFINDINFO		lvfi;
	TCHAR			szDisplayName[MAX_PATH];
	LVITEM			lvItem;
	TCHAR			szFullFileName[MAX_PATH];
	DWORD_PTR		res;
	HRESULT			hr;
	int				iItem;

	if(iItemInternal == -1)
		return;

	auto &itemInfo = m_itemInfoMap.at(iItemInternal);

	StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_CurDir);
	PathAppend(szFullFileName,szNewFileName);

	unique_pidl_absolute pidlFull;
	hr = SHParseDisplayName(szFullFileName, nullptr, wil::out_param(pidlFull), 0, nullptr);

	if(SUCCEEDED(hr))
	{
		hr = SHBindToParent(pidlFull.get(), IID_PPV_ARGS(&pShellFolder), &pidlRelative);

		if(SUCCEEDED(hr))
		{
			hr = GetDisplayName(szFullFileName,szDisplayName,SIZEOF_ARRAY(szDisplayName),SHGDN_INFOLDER|SHGDN_FORPARSING);

			if(SUCCEEDED(hr))
			{
				itemInfo.pidlComplete.reset(ILCloneFull(pidlFull.get()));
				itemInfo.pridl.reset(ILCloneChild(pidlRelative));
				StringCchCopy(itemInfo.szDisplayName, SIZEOF_ARRAY(itemInfo.szDisplayName), szDisplayName);
				StringCchCopy(itemInfo.wfd.cFileName, SIZEOF_ARRAY(itemInfo.wfd.cFileName), szNewFileName);

				/* The files' type may have changed, so retrieve the files'
				icon again. */
				res = SHGetFileInfo((LPTSTR)pidlFull.get(),0,&shfi,
					sizeof(SHFILEINFO),SHGFI_PIDL|SHGFI_ICON|
					SHGFI_OVERLAYINDEX);

				if(res != 0)
				{
					/* Locate the item within the listview. */
					lvfi.flags	= LVFI_PARAM;
					lvfi.lParam	= iItemInternal;
					iItem = ListView_FindItem(m_hListView,-1,&lvfi);

					if(iItem != -1)
					{
						BasicItemInfo_t basicItemInfo = getBasicItemInfo(iItemInternal);
						std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);

						TCHAR filenameCopy[MAX_PATH];
						StringCchCopy(filenameCopy, SIZEOF_ARRAY(filenameCopy), filename.c_str());

						lvItem.mask			= LVIF_TEXT|LVIF_IMAGE|LVIF_STATE;
						lvItem.iItem		= iItem;
						lvItem.iSubItem		= 0;
						lvItem.iImage		= shfi.iIcon;
						lvItem.pszText		= filenameCopy;
						lvItem.stateMask	= LVIS_OVERLAYMASK;

						/* As well as resetting the items icon, we'll also set
						it's overlay again (the overlay could change, for example,
						if the file is changed to a shortcut). */
						lvItem.state		= INDEXTOOVERLAYMASK(shfi.iIcon >> 24);

						/* Update the item in the listview. */
						ListView_SetItem(m_hListView,&lvItem);

						/* TODO: Does the file need to be filtered out? */
						if(IsFileFiltered(itemInfo))
						{
							RemoveFilteredItem(iItem,iItemInternal);
						}
					}

					DestroyIcon(shfi.hIcon);
				}
			}

			pShellFolder->Release();
		}
	}
	else
	{
		StringCchCopy(itemInfo.szDisplayName, SIZEOF_ARRAY(itemInfo.szDisplayName), szNewFileName);
		StringCchCopy(itemInfo.wfd.cFileName, SIZEOF_ARRAY(itemInfo.wfd.cFileName), szNewFileName);
	}
}