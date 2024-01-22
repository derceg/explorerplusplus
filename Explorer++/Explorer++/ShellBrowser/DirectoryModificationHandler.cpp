// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "ItemData.h"
#include "ShellNavigationController.h"
#include "ViewModes.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <list>

void ShellBrowser::StartDirectoryMonitoring(PCIDLIST_ABSOLUTE pidl)
{
	// Shouldn't be monitoring the same directory with both directory modification notifications and
	// shell change notifications.
	assert(!m_dirMonitorId);

	m_shellChangeWatcher.StartWatching(pidl,
		SHCNE_ATTRIBUTES | SHCNE_CREATE | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RENAMEFOLDER
			| SHCNE_RENAMEITEM | SHCNE_RMDIR | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | SHCNE_DRIVEADD
			| SHCNE_DRIVEREMOVED);
}

void ShellBrowser::ProcessShellChangeNotifications(
	const std::vector<ShellChangeNotification> &shellChangeNotifications)
{
	SendMessage(m_hListView, WM_SETREDRAW, FALSE, NULL);

	for (const auto &change : shellChangeNotifications)
	{
		ProcessShellChangeNotification(change);
	}

	SendMessage(m_hListView, WM_SETREDRAW, TRUE, NULL);

	directoryModified.m_signal();
}

void ShellBrowser::ProcessShellChangeNotification(const ShellChangeNotification &change)
{
	switch (change.event)
	{
	case SHCNE_DRIVEADD:
	case SHCNE_MKDIR:
	case SHCNE_CREATE:
		if (ILIsParent(m_directoryState.pidlDirectory.get(), change.pidl1.get(), TRUE))
		{
			OnItemAdded(change.pidl1.get());
		}
		break;

	case SHCNE_RENAMEFOLDER:
	case SHCNE_RENAMEITEM:
		if (ILIsParent(m_directoryState.pidlDirectory.get(), change.pidl1.get(), TRUE)
			&& ILIsParent(m_directoryState.pidlDirectory.get(), change.pidl2.get(), TRUE))
		{
			OnItemRenamed(change.pidl1.get(), change.pidl2.get());
		}
		break;

	case SHCNE_UPDATEITEM:
		if (ILIsParent(m_directoryState.pidlDirectory.get(), change.pidl1.get(), TRUE))
		{
			OnItemModified(change.pidl1.get());
		}
		break;

	case SHCNE_UPDATEDIR:
		if (ArePidlsEquivalent(m_directoryState.pidlDirectory.get(), change.pidl1.get()))
		{
			// It's not safe to perform an immediate refresh here, since doing so would clear
			// ShellChangeWatcher::m_shellChangeNotifications, which is being actively iterated
			// through. A pending task is created instead, which will be processed via the message
			// loop.
			AddTaskToPendingWorkQueue(
				std::bind_front(&ShellBrowser::RefreshDirectoryAfterUpdate, this));
		}
		break;

	case SHCNE_DRIVEREMOVED:
	case SHCNE_RMDIR:
	case SHCNE_DELETE:
		// Only the current directory is monitored, so notifications should only arrive for items in
		// that directory. However, if the user has just changed directories, a notification could
		// still come in for the previous directory. Therefore, it's important to verify that the
		// item is actually a child of the current directory.
		if (ILIsParent(m_directoryState.pidlDirectory.get(), change.pidl1.get(), TRUE))
		{
			OnItemRemoved(change.pidl1.get());
		}
		else if (ArePidlsEquivalent(m_directoryState.pidlDirectory.get(), change.pidl1.get()))
		{
			// The current folder has been deleted. That makes it necessary to navigate to another
			// folder. For similarity with Explorer, a navigation to the parent will occur.
			AddTaskToPendingWorkQueue(
				std::bind_front(&ShellBrowser::GoUpAfterCurrentDirectoryDeleted, this));
		}
		break;
	}
}

void ShellBrowser::DirectoryAltered()
{
	EnterCriticalSection(&m_csDirectoryAltered);

	SendMessage(m_hListView, WM_SETREDRAW, FALSE, NULL);

	// Note that directory change notifications are received asynchronously. That means that, in
	// each of the cases below, it's not reasonable to assume that the file being referenced
	// actually exists (since it may have been renamed or deleted since the original notification
	// was sent).
	for (const auto &af : m_AlteredList)
	{
		// Only undertake the modification if the unique folder index on the modified item and
		// current folder match up (i.e. ensure the directory has not changed since these files were
		// modified).
		if (af.iFolderIndex != m_uniqueFolderId)
		{
			continue;
		}

		wil::com_ptr_nothrow<IShellFolder> parent;
		HRESULT hr = SHBindToObject(nullptr, m_directoryState.pidlDirectory.get(), nullptr,
			IID_PPV_ARGS(&parent));

		if (FAILED(hr))
		{
			continue;
		}

		unique_pidl_absolute simplePidl;
		hr = CreateSimplePidl(af.szFileName, wil::out_param(simplePidl), parent.get());

		if (FAILED(hr))
		{
			continue;
		}

		switch (af.dwAction)
		{
		case FILE_ACTION_ADDED:
			OnItemAdded(simplePidl.get());
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
			assert(!m_renamedItemOldPidl);
			m_renamedItemOldPidl.reset(ILCloneFull(simplePidl.get()));
			break;

		case FILE_ACTION_RENAMED_NEW_NAME:
			OnItemRenamed(m_renamedItemOldPidl.get(), simplePidl.get());

			m_renamedItemOldPidl.reset();
			break;

		case FILE_ACTION_MODIFIED:
			OnItemModified(simplePidl.get());
			break;

		case FILE_ACTION_REMOVED:
			OnItemRemoved(simplePidl.get());
			break;
		}
	}

	SendMessage(m_hListView, WM_SETREDRAW, TRUE, NULL);

	directoryModified.m_signal();

	m_AlteredList.clear();

	LeaveCriticalSection(&m_csDirectoryAltered);
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(dwTime);

	KillTimer(hwnd, idEvent);

	SendMessage(hwnd, WM_USER_FILESADDED, idEvent, 0);
}

void ShellBrowser::FilesModified(DWORD Action, const TCHAR *FileName, int EventId, int iFolderIndex)
{
	EnterCriticalSection(&m_csDirectoryAltered);

	SetTimer(m_hOwner, EventId, 200, TimerProc);

	AlteredFile_t af;

	StringCchCopy(af.szFileName, SIZEOF_ARRAY(af.szFileName), FileName);
	af.dwAction = Action;
	af.iFolderIndex = iFolderIndex;

	m_AlteredList.push_back(af);

	LeaveCriticalSection(&m_csDirectoryAltered);
}

void ShellBrowser::OnItemAdded(PCIDLIST_ABSOLUTE simplePidl)
{
	auto existingItemInternalIndex = GetItemInternalIndexForPidl(simplePidl);

	// When adding an item, it makes no sense to add it if it already exists. If the item does
	// exist, it's an indication of a programming error. That is, it's not expected that this would
	// happen at all during normal use.
	// Silently returning here is about the only thing that can be reasonably done and at least
	// prevents duplicate items from being added.
	if (existingItemInternalIndex)
	{
		assert(false);
		return;
	}

	unique_pidl_absolute pidlFull;
	HRESULT hr = SimplePidlToFullPidl(simplePidl, wil::out_param(pidlFull));

	PCIDLIST_ABSOLUTE pidl;

	// The item being referenced may not exist at this point, so it's valid for SimplePidlToFullPidl
	// to fail. In that case, the simple PIDL will be used instead. The issue with that is that the
	// WIN32_FIND_DATA information cached in the simple PIDL won't be valid. However, that's likely
	// ok.
	// The reason for that is that if this item has been renamed or deleted, the notification for
	// the rename/deletion is likely to be processed soon, so that there would be no practical
	// chance for the user to notice that the item details are wrong.
	if (SUCCEEDED(hr))
	{
		pidl = pidlFull.get();
	}
	else
	{
		pidl = simplePidl;
	}

	AddItem(pidl);
}

void ShellBrowser::AddItem(PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	PCITEMID_CHILD pidlChild = nullptr;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&shellFolder), &pidlChild);

	if (FAILED(hr))
	{
		return;
	}

	auto itemId = AddItemInternal(shellFolder.get(), m_directoryState.pidlDirectory.get(),
		pidlChild, -1, FALSE);

	if (!itemId)
	{
		return;
	}

	if (m_config->globalFolderSettings.insertSorted)
	{
		// TODO: It would be better to pass the items details to this function directly
		// instead (before the item is added to the awaiting list).
		int sortedPosition = DetermineItemSortedPosition(*itemId);

		auto itr = std::find_if(m_directoryState.awaitingAddList.begin(),
			m_directoryState.awaitingAddList.end(),
			[itemId](const AwaitingAdd_t &awaitingItem)
			{ return *itemId == awaitingItem.iItemInternal; });

		// The item was added successfully above, so should be in the list of awaiting
		// items.
		assert(itr != m_directoryState.awaitingAddList.end());

		itr->iItem = sortedPosition;
		itr->bPosition = TRUE;
		itr->iAfter = sortedPosition - 1;
	}

	InsertAwaitingItems();
}

void ShellBrowser::OnItemRemoved(PCIDLIST_ABSOLUTE simplePidl)
{
	auto internalIndex = GetItemInternalIndexForPidl(simplePidl);

	if (internalIndex)
	{
		RemoveItem(*internalIndex);
	}
}

void ShellBrowser::OnItemModified(PCIDLIST_ABSOLUTE simplePidl)
{
	unique_pidl_absolute pidlFull;
	HRESULT hr = SimplePidlToFullPidl(simplePidl, wil::out_param(pidlFull));

	// SimplePidlToFullPidl may fail if this item no longer exists. However, there's nothing that
	// can be done in that case. Leaving the previous details in place until the rename/deletion
	// notification is received is ok and unlikely to actually be noticed by the user (since the
	// rename/deletion notification is likely to be processed soon).
	if (SUCCEEDED(hr))
	{
		UpdateItem(pidlFull.get());
	}
}

// Handles both renames and modifications, since they're effectively the same thing. When a rename
// notification is processed and the updated item details are retrieved, other changes to the item
// could have been made as well. For example, the item's size could have changed since the rename
// notification was dispatched. So it makes sense to treat the item as if it has been updated.
// Additionally, if an item has been updated, then immediately renamed, the update notification
// might get ignored (since the item no longer exists with its original name). In that case, the
// item's details would have to be updated when the rename notification was processed.
// When an item is modified, the name shouldn't change, so that does mean that there is at least one
// difference between the two update types. However, handling both updates in a single method is
// better than having two very similar methods.
void ShellBrowser::UpdateItem(PCIDLIST_ABSOLUTE pidl, PCIDLIST_ABSOLUTE updatedPidl)
{
	auto internalIndex = GetItemInternalIndexForPidl(pidl);

	if (!internalIndex)
	{
		// When the user renames an item in the listview, the item details will be updated
		// immediately. That means that when the rename notification is received, the old item will
		// no longer exist and won't be found. In that case, though, the new item should exist.
		assert(!updatedPidl || GetItemInternalIndexForPidl(updatedPidl));
		return;
	}

	PCIDLIST_ABSOLUTE currentPidl = pidl;

	if (updatedPidl)
	{
		currentPidl = updatedPidl;
	}

	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	PCITEMID_CHILD pidlChild = nullptr;
	HRESULT hr = SHBindToParent(currentPidl, IID_PPV_ARGS(&shellFolder), &pidlChild);

	if (FAILED(hr))
	{
		return;
	}

	auto itemInfo =
		GetItemInformation(shellFolder.get(), m_directoryState.pidlDirectory.get(), pidlChild);

	if (!itemInfo)
	{
		return;
	}

	ULARGE_INTEGER oldFileSize = { m_itemInfoMap[*internalIndex].wfd.nFileSizeLow,
		m_itemInfoMap[*internalIndex].wfd.nFileSizeHigh };
	ULARGE_INTEGER newFileSize = { itemInfo->wfd.nFileSizeLow, itemInfo->wfd.nFileSizeHigh };

	m_directoryState.totalDirSize += newFileSize.QuadPart - oldFileSize.QuadPart;

	m_itemInfoMap[*internalIndex] = std::move(*itemInfo);
	const ItemInfo_t &updatedItemInfo = m_itemInfoMap[*internalIndex];

	auto itemIndex = LocateItemByInternalIndex(*internalIndex);

	// Items may be filtered out of the listview, so it's valid for an item not to be found.
	if (!itemIndex)
	{
		if (!IsFileFiltered(updatedItemInfo))
		{
			UnfilterItem(*internalIndex);
		}

		return;
	}

	UINT state = ListView_GetItemState(m_hListView, *itemIndex, LVIS_SELECTED);

	if (WI_IsFlagSet(state, LVIS_SELECTED))
	{
		m_directoryState.fileSelectionSize += newFileSize.QuadPart - oldFileSize.QuadPart;
	}

	if (IsFileFiltered(updatedItemInfo))
	{
		RemoveFilteredItem(*itemIndex, *internalIndex);
		return;
	}

	InvalidateIconForItem(*itemIndex);

	if (m_folderSettings.viewMode == +ViewMode::Details)
	{
		InvalidateAllColumnsForItem(*itemIndex);
	}
	else
	{
		// The display name can change, even if the parsing name is the same. For example, when the
		// recycle bin is renamed, the parsing name remains the same.
		BasicItemInfo_t basicItemInfo = getBasicItemInfo(*internalIndex);
		std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);
		ListView_SetItemText(m_hListView, *itemIndex, 0, filename.data());
	}

	if (WI_IsFlagSet(updatedItemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
	{
		ListView_SetItemState(m_hListView, *itemIndex, LVIS_CUT, LVIS_CUT);
	}
	else
	{
		ListView_SetItemState(m_hListView, *itemIndex, 0, LVIS_CUT);
	}

	if (m_folderSettings.showInGroups)
	{
		int groupId = DetermineItemGroup(*internalIndex);
		InsertItemIntoGroup(*itemIndex, groupId);
	}

	// It's not safe to use itemIndex past this point.
	ListView_SortItems(m_hListView, SortStub, this);
	itemIndex.reset();
}

void ShellBrowser::OnItemRenamed(PCIDLIST_ABSOLUTE simplePidlOld, PCIDLIST_ABSOLUTE simplePidlNew)
{
	// When an item is updated, the WIN32_FIND_DATA information cached in the pidl will be
	// retrieved. As the simple pidl won't contain this information, it's important to convert the
	// pidl to a full pidl here.
	unique_pidl_absolute pidlNewFull;
	HRESULT hr = SimplePidlToFullPidl(simplePidlNew, wil::out_param(pidlNewFull));

	PCIDLIST_ABSOLUTE pidlNew;

	// As with the above cases, it's valid for SimplePidlToFullPidl to fail, since the item may no
	// longer exist with the new name. Updating the item details anyway should be ok, for two
	// reasons.
	// The first is that, as when adding an item, the rename/deletion notification is likely to be
	// received soon, so there's not much practical chance for the user to notice that the item has
	// invalid details.
	// The second is that even if there was enough time for the user to notice the details were
	// invalid, they would also be invalid if the original details were left in place. That is,
	// attempting to show information on the item, based on its previous name, is going to fail as
	// well.
	if (SUCCEEDED(hr))
	{
		pidlNew = pidlNewFull.get();
	}
	else
	{
		pidlNew = simplePidlNew;
	}

	UpdateItem(simplePidlOld, pidlNew);
}

void ShellBrowser::InvalidateAllColumnsForItem(int itemIndex)
{
	if (m_folderSettings.viewMode != +ViewMode::Details)
	{
		return;
	}

	auto numColumns = std::count_if(m_pActiveColumns->begin(), m_pActiveColumns->end(),
		[](const Column_t &column) { return column.bChecked; });

	for (int i = 0; i < numColumns; i++)
	{
		ListView_SetItemText(m_hListView, itemIndex, i, LPSTR_TEXTCALLBACK);
	}
}

void ShellBrowser::InvalidateIconForItem(int itemIndex)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE;
	lvItem.iItem = itemIndex;
	lvItem.iSubItem = 0;
	lvItem.iImage = I_IMAGECALLBACK;
	ListView_SetItem(m_hListView, &lvItem);
}

void ShellBrowser::RefreshDirectoryAfterUpdate()
{
	m_navigationController->Refresh();
}

void ShellBrowser::GoUpAfterCurrentDirectoryDeleted()
{
	unique_pidl_absolute pidlParent;
	HRESULT hr =
		GetVirtualParentPath(m_directoryState.pidlDirectory.get(), wil::out_param(pidlParent));

	if (FAILED(hr))
	{
		// The only item that doesn't have a parent is the root folder. However, it's not possible
		// to delete that folder, so this function should never be called in a situation where
		// there's no parent folder.
		return;
	}

	// The navigation here doesn't use ShellNavigationController::GoUp(), since the navigation
	// needs to occur in this tab, regardless of whether not the tab is locked.
	NavigateParams params = NavigateParams::Normal(pidlParent.get());
	Navigate(params);
}
