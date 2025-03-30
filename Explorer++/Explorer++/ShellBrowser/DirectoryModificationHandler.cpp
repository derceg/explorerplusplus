// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "Config.h"
#include "ItemData.h"
#include "NavigateParams.h"
#include "Runtime.h"
#include "RuntimeHelper.h"
#include "ShellNavigationController.h"
#include "ViewModes.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/ShellHelper.h"
#include <list>

void ShellBrowserImpl::StartDirectoryMonitoring()
{
	if (m_config->changeNotifyMode == ChangeNotifyMode::Shell)
	{
		StartDirectoryMonitoringViaShellChangeWatcher();
	}
	else
	{
		StartDirectoryMonitoringViaFileSystemChangeWatcher();
	}
}

void ShellBrowserImpl::StartDirectoryMonitoringViaShellChangeWatcher()
{
	m_shellChangeWatcher.StartWatching(m_directoryState.pidlDirectory.Raw(),
		SHCNE_ATTRIBUTES | SHCNE_CREATE | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RENAMEFOLDER
			| SHCNE_RENAMEITEM | SHCNE_RMDIR | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | SHCNE_DRIVEADD
			| SHCNE_DRIVEREMOVED);

	unique_pidl_absolute rootPidl;
	HRESULT hr = GetRootPidl(wil::out_param(rootPidl));

	if (SUCCEEDED(hr))
	{
		// Monitoring a folder allows direct deletion of the folder to be detected. In that case, a
		// SHCNE_RMDIR notification will be sent.
		// It doesn't, however, allow indirect deletion to be detected. For example, if a parent
		// folder is deleted or renamed, no notification will be sent.
		// Therefore, it's necessary to globally monitor SHCNE_RMDIR notifications here, to detect
		// when a parent folder is deleted. It's also necessary to globally monitor SHCNE_UPDATEDIR
		// notifications, for at least two reasons:
		//
		// 1. SHCNE_RMDIR isn't sent consistently. It may or may not be sent when a parent folder is
		// deleted.
		// 2. When a parent folder is renamed, only a SHCNE_UPDATEDIR notification will be sent.
		//
		// This pair of notifications is also what Explorer uses to navigate away from a folder that
		// no longer exists.
		m_shellChangeWatcher.StartWatching(rootPidl.get(), SHCNE_RMDIR | SHCNE_UPDATEDIR, true);
	}
}

void ShellBrowserImpl::StartDirectoryMonitoringViaFileSystemChangeWatcher()
{
	m_directoryState.fileSystemChangeWatcher =
		FileSystemChangeWatcher::MaybeCreate(m_directoryState.pidlDirectory.Raw(),
			wil::FolderChangeEvents::All, m_app->GetRuntime()->GetUiThreadExecutor(),
			std::bind_front(&ShellBrowserImpl::ProcessFileSystemChangeNotification, this));
}

void ShellBrowserImpl::ProcessShellChangeNotifications(
	const std::vector<ShellChangeNotification> &shellChangeNotifications)
{
	ScopedRedrawDisabler redrawDisabler(m_hListView);

	for (const auto &change : shellChangeNotifications)
	{
		ProcessShellChangeNotification(change);
	}

	m_app->GetShellBrowserEvents()->NotifyItemsChanged(this);
}

void ShellBrowserImpl::ProcessShellChangeNotification(const ShellChangeNotification &change)
{
	switch (change.event)
	{
	case SHCNE_DRIVEADD:
	case SHCNE_MKDIR:
	case SHCNE_CREATE:
		if (ILIsParent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get(), TRUE))
		{
			OnItemAdded(change.pidl1.get());
		}
		break;

	case SHCNE_RENAMEFOLDER:
	case SHCNE_RENAMEITEM:
		if (ILIsParent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get(), TRUE)
			&& ILIsParent(m_directoryState.pidlDirectory.Raw(), change.pidl2.get(), TRUE))
		{
			OnItemRenamed(change.pidl1.get(), change.pidl2.get());
		}
		else if (ArePidlsEquivalent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get()))
		{
			OnCurrentDirectoryRenamed(m_weakPtrFactory.GetWeakPtr(), change.pidl2.get(),
				m_app->GetRuntime());
		}
		break;

	case SHCNE_UPDATEITEM:
		if (ILIsParent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get(), TRUE))
		{
			OnItemModified(change.pidl1.get());
		}
		else if (ArePidlsEquivalent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get()))
		{
			// This can be triggered in the following sorts of situations:
			//
			// - If the icon for the folder is changed.
			// - If the folder is virtual (e.g. the recycle bin) and the name is changed.
			OnDirectoryPropertiesChanged(m_weakPtrFactory.GetWeakPtr(),
				m_directoryState.pidlDirectory, m_app->GetRuntime());
		}
		break;

	case SHCNE_UPDATEDIR:
		if (ArePidlsEquivalent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get()))
		{
			// It's not safe to perform an immediate refresh here, since doing so would clear
			// ShellChangeWatcher::m_shellChangeNotifications, which is being actively iterated
			// through. Therefore, the function below will perform the refresh asynchronously.
			RefreshDirectoryAfterUpdate(m_weakPtrFactory.GetWeakPtr(), m_app->GetRuntime());
		}
		else if (ILIsParent(change.pidl1.get(), m_directoryState.pidlDirectory.Raw(), false))
		{
			// A parent folder has been updated. It's possible this folder may no longer exist (e.g.
			// because a parent was renamed or removed). A navigation to a parent item may be
			// required. It's also possible an unrelated item was updated, in which case no action
			// will be taken by the function below.
			NavigateUpToClosestExistingItemIfNecessary(m_weakPtrFactory.GetWeakPtr(),
				m_directoryState.pidlDirectory, m_app->GetRuntime());
		}
		break;

	case SHCNE_DRIVEREMOVED:
	case SHCNE_RMDIR:
	case SHCNE_DELETE:
		// Only the current directory is monitored, so notifications should only arrive for items in
		// that directory. However, if the user has just changed directories, a notification could
		// still come in for the previous directory. Therefore, it's important to verify that the
		// item is actually a child of the current directory.
		if (ILIsParent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get(), TRUE))
		{
			OnItemRemoved(change.pidl1.get());
		}
		else if (ArePidlsEquivalent(m_directoryState.pidlDirectory.Raw(), change.pidl1.get())
			|| ILIsParent(change.pidl1.get(), m_directoryState.pidlDirectory.Raw(), false))
		{
			// The current folder has been deleted, either directly, or by deleting one of its
			// parents. That makes it necessary to navigate to another folder. For similarity with
			// Explorer, a navigation to a parent will occur.
			NavigateUpToClosestExistingItemIfNecessary(m_weakPtrFactory.GetWeakPtr(),
				m_directoryState.pidlDirectory, m_app->GetRuntime());
		}
		break;
	}
}

void ShellBrowserImpl::ProcessFileSystemChangeNotification(FileSystemChangeWatcher::Event event,
	const PidlAbsolute &simplePidl1, const PidlAbsolute &simplePidl2)
{
	switch (event)
	{
	case FileSystemChangeWatcher::Event::Added:
		OnItemAdded(simplePidl1.Raw());
		break;

	case FileSystemChangeWatcher::Event::Modified:
		OnItemModified(simplePidl1.Raw());
		break;

	case FileSystemChangeWatcher::Event::Renamed:
		OnItemRenamed(simplePidl1.Raw(), simplePidl2.Raw());
		break;

	case FileSystemChangeWatcher::Event::Removed:
		OnItemRemoved(simplePidl1.Raw());
		break;
	}

	m_app->GetShellBrowserEvents()->NotifyItemsChanged(this);
}

void ShellBrowserImpl::OnItemAdded(PCIDLIST_ABSOLUTE simplePidl)
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

	PidlAbsolute pidlFull;
	HRESULT hr = UpdatePidl(simplePidl, pidlFull);

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
		pidl = pidlFull.Raw();
	}
	else
	{
		pidl = simplePidl;
	}

	AddItem(pidl);
}

void ShellBrowserImpl::AddItem(PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	PCITEMID_CHILD pidlChild = nullptr;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&shellFolder), &pidlChild);

	if (FAILED(hr))
	{
		return;
	}

	auto itemId = AddItemInternal(shellFolder.get(), m_directoryState.pidlDirectory.Raw(),
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
			m_directoryState.awaitingAddList.end(), [itemId](const AwaitingAdd_t &awaitingItem)
			{ return *itemId == awaitingItem.iItemInternal; });

		// The item was added successfully above, so should be in the list of awaiting
		// items.
		CHECK(itr != m_directoryState.awaitingAddList.end());

		itr->iItem = sortedPosition;
		itr->bPosition = TRUE;
		itr->iAfter = sortedPosition - 1;
	}

	InsertAwaitingItems();
}

void ShellBrowserImpl::OnItemRemoved(PCIDLIST_ABSOLUTE simplePidl)
{
	auto internalIndex = GetItemInternalIndexForPidl(simplePidl);

	if (internalIndex)
	{
		RemoveItem(*internalIndex);
	}
}

void ShellBrowserImpl::OnItemModified(PCIDLIST_ABSOLUTE simplePidl)
{
	PidlAbsolute pidlFull;
	HRESULT hr = UpdatePidl(simplePidl, pidlFull);

	// SimplePidlToFullPidl may fail if this item no longer exists. However, there's nothing that
	// can be done in that case. Leaving the previous details in place until the rename/deletion
	// notification is received is ok and unlikely to actually be noticed by the user (since the
	// rename/deletion notification is likely to be processed soon).
	if (SUCCEEDED(hr))
	{
		UpdateItem(pidlFull.Raw());
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
void ShellBrowserImpl::UpdateItem(PCIDLIST_ABSOLUTE pidl, PCIDLIST_ABSOLUTE updatedPidl)
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
		GetItemInformation(shellFolder.get(), m_directoryState.pidlDirectory.Raw(), pidlChild);

	if (!itemInfo)
	{
		return;
	}

	ULARGE_INTEGER oldFileSize = { m_itemInfoMap[*internalIndex].wfd.nFileSizeLow,
		m_itemInfoMap[*internalIndex].wfd.nFileSizeHigh };
	ULARGE_INTEGER newFileSize = { itemInfo->wfd.nFileSizeLow, itemInfo->wfd.nFileSizeHigh };

	m_directoryState.totalDirSize += newFileSize.QuadPart - oldFileSize.QuadPart;

	m_itemInfoMap[*internalIndex] = *itemInfo;
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

void ShellBrowserImpl::OnItemRenamed(PCIDLIST_ABSOLUTE simplePidlOld,
	PCIDLIST_ABSOLUTE simplePidlNew)
{
	// When an item is updated, the WIN32_FIND_DATA information cached in the pidl will be
	// retrieved. As the simple pidl won't contain this information, it's important to convert the
	// pidl to a full pidl here.
	PidlAbsolute pidlNewFull;
	HRESULT hr = UpdatePidl(simplePidlNew, pidlNewFull);

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
		pidlNew = pidlNewFull.Raw();
	}
	else
	{
		pidlNew = simplePidlNew;
	}

	UpdateItem(simplePidlOld, pidlNew);
}

void ShellBrowserImpl::InvalidateAllColumnsForItem(int itemIndex)
{
	if (m_folderSettings.viewMode != +ViewMode::Details)
	{
		return;
	}

	auto numColumns = std::count_if(m_pActiveColumns->begin(), m_pActiveColumns->end(),
		[](const Column_t &column) { return column.checked; });

	for (int i = 0; i < numColumns; i++)
	{
		ListView_SetItemText(m_hListView, itemIndex, i, LPSTR_TEXTCALLBACK);
	}
}

void ShellBrowserImpl::InvalidateIconForItem(int itemIndex)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE;
	lvItem.iItem = itemIndex;
	lvItem.iSubItem = 0;
	lvItem.iImage = I_IMAGECALLBACK;
	ListView_SetItem(m_hListView, &lvItem);
}

concurrencpp::null_result ShellBrowserImpl::OnCurrentDirectoryRenamed(
	WeakPtr<ShellBrowserImpl> weakSelf, PidlAbsolute simplePidlUpdated, Runtime *runtime)
{
	co_await ResumeOnComStaThread(runtime);

	PidlAbsolute fullPidlUpdated;
	HRESULT hr = UpdatePidl(simplePidlUpdated.Raw(), fullPidlUpdated);

	if (FAILED(hr))
	{
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	if (!weakSelf)
	{
		// The folder has changed, or the tab has been closed.
		co_return;
	}

	NavigateParams params =
		NavigateParams::Normal(fullPidlUpdated.Raw(), HistoryEntryType::ReplaceCurrentEntry);
	params.overrideNavigationTargetMode = true;
	weakSelf->m_navigationController->Navigate(params);
}

concurrencpp::null_result ShellBrowserImpl::OnDirectoryPropertiesChanged(
	WeakPtr<ShellBrowserImpl> weakSelf, PidlAbsolute currentDirectory, Runtime *runtime)
{
	co_await ResumeOnComStaThread(runtime);

	PidlAbsolute updatedPidl;
	HRESULT hr = UpdatePidl(currentDirectory.Raw(), updatedPidl);

	if (FAILED(hr))
	{
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	if (!weakSelf)
	{
		co_return;
	}

	// TODO: Should possibly also update the current navigation entry.

	// The parsing path isn't updated, since it should remain the same. Item renames can result in
	// this function being triggered, but only if the item is virtual. In that case, the parsing
	// path isn't going to change.
	weakSelf->m_directoryState.pidlDirectory = updatedPidl;
	weakSelf->m_app->GetShellBrowserEvents()->NotifyDirectoryPropertiesChanged(weakSelf.Get());
}

concurrencpp::null_result ShellBrowserImpl::RefreshDirectoryAfterUpdate(
	WeakPtr<ShellBrowserImpl> weakSelf, Runtime *runtime)
{
	co_await concurrencpp::resume_on(runtime->GetUiThreadExecutor());

	if (!weakSelf)
	{
		co_return;
	}

	weakSelf->m_navigationController->Refresh();
}

// Navigates to the closest ancestor of this item that exists. If this item itself exists, no
// navigation will occur.
concurrencpp::null_result ShellBrowserImpl::NavigateUpToClosestExistingItemIfNecessary(
	WeakPtr<ShellBrowserImpl> weakSelf, PidlAbsolute currentDirectory, Runtime *runtime)
{
	co_await ResumeOnComStaThread(runtime);

	auto closestExistingItemPidl = GetClosestExistingItem(currentDirectory.Raw());

	if (!closestExistingItemPidl.HasValue())
	{
		DCHECK(false);
		co_return;
	}

	if (ArePidlsEquivalent(closestExistingItemPidl.Raw(), currentDirectory.Raw()))
	{
		// The current directory still exists, so there's no need to do anything.
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	if (!weakSelf)
	{
		co_return;
	}

	// The current directory no longer exists, so the navigation here needs to proceed in this tab,
	// regardless of whether or not the tab is locked.
	NavigateParams params = NavigateParams::Normal(closestExistingItemPidl.Raw());
	params.overrideNavigationTargetMode = true;
	weakSelf->m_navigationController->Navigate(params);
}
