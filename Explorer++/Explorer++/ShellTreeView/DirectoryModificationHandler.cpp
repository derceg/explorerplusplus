// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "../Helper/Logging.h"

void ShellTreeView::StartDirectoryMonitoringForItem(ItemInfo &item)
{
	// There shouldn't be more than one call to monitor a directory.
	assert(item.shChangeNotifyId == 0);

	SHChangeNotifyEntry shcne;
	shcne.pidl = item.pidl.get();
	shcne.fRecursive = false;
	item.shChangeNotifyId = SHChangeNotifyRegister(m_hTreeView,
		SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
		SHCNE_ATTRIBUTES | SHCNE_MKDIR | SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_UPDATEDIR
			| SHCNE_UPDATEITEM,
		WM_APP_SHELL_NOTIFY, 1, &shcne);

	if (item.shChangeNotifyId == 0)
	{
		std::wstring path;
		HRESULT hr = GetDisplayName(item.pidl.get(), SHGDN_FORPARSING, path);

		if (SUCCEEDED(hr))
		{
			LOG(warning) << L"Couldn't monitor directory \"" << path << L"\" for changes.";
		}
	}
}

void ShellTreeView::StopDirectoryMonitoringForItem(ItemInfo &item)
{
	if (item.shChangeNotifyId == 0)
	{
		return;
	}

	[[maybe_unused]] auto res = SHChangeNotifyDeregister(item.shChangeNotifyId);
	assert(res);

	item.shChangeNotifyId = 0;
}

void ShellTreeView::OnShellNotify(WPARAM wParam, LPARAM lParam)
{
	PIDLIST_ABSOLUTE *pidls;
	LONG event;
	HANDLE lock = SHChangeNotification_Lock(reinterpret_cast<HANDLE>(wParam),
		static_cast<DWORD>(lParam), &pidls, &event);

	m_shellChangeNotifications.emplace_back(event, pidls[0], pidls[1]);

	SHChangeNotification_Unlock(lock);

	SetTimer(m_hTreeView, PROCESS_SHELL_CHANGES_TIMER_ID, PROCESS_SHELL_CHANGES_TIMEOUT, nullptr);
}

void ShellTreeView::OnProcessShellChangeNotifications()
{
	KillTimer(m_hTreeView, PROCESS_SHELL_CHANGES_TIMER_ID);

	SendMessage(m_hTreeView, WM_SETREDRAW, FALSE, 0);

	for (const auto &change : m_shellChangeNotifications)
	{
		ProcessShellChangeNotification(change);
	}

	SendMessage(m_hTreeView, WM_SETREDRAW, TRUE, 0);

	m_shellChangeNotifications.clear();
}

void ShellTreeView::ProcessShellChangeNotification(const ShellChangeNotification &change)
{
	switch (change.event)
	{
	case SHCNE_MKDIR:
		OnItemAdded(change.pidl1.get());
		break;

	case SHCNE_RMDIR:
		OnItemRemoved(change.pidl1.get());
		break;
	}
}

void ShellTreeView::OnItemAdded(PCIDLIST_ABSOLUTE simplePidl)
{
	auto existingItem = LocateExistingItem(simplePidl);

	// Items shouldn't be added more than once.
	if (existingItem)
	{
		assert(false);
		return;
	}

	unique_pidl_absolute parent(ILCloneFull(simplePidl));
	BOOL res = ILRemoveLastID(parent.get());

	if (!res)
	{
		return;
	}

	auto parentItem = LocateExistingItem(parent.get());

	// If the parent item isn't being shown, there's no need to add this item.
	if (!parentItem)
	{
		return;
	}

	TVITEMEX tvParentItem = {};
	tvParentItem.mask = TVIF_HANDLE | TVIF_STATE;
	tvParentItem.hItem = parentItem;
	res = TreeView_GetItem(m_hTreeView, &tvParentItem);
	assert(res);

	// If the parent exists, but isn't expanded, there's also no need to add this item.
	if (WI_IsFlagClear(tvParentItem.state, TVIS_EXPANDED))
	{
		return;
	}

	unique_pidl_absolute pidlFull;
	HRESULT hr = SimplePidlToFullPidl(simplePidl, wil::out_param(pidlFull));

	PCIDLIST_ABSOLUTE pidl;

	if (SUCCEEDED(hr))
	{
		pidl = pidlFull.get();
	}
	else
	{
		pidl = simplePidl;
	}

	AddItem(parentItem, pidl);
}

void ShellTreeView::OnItemRemoved(PCIDLIST_ABSOLUTE simplePidl)
{
	auto item = LocateExistingItem(simplePidl);

	if (item)
	{
		RemoveItem(item);
	}
}

void ShellTreeView::RemoveItem(HTREEITEM item)
{
	StopDirectoryMonitoringForItem(GetItemByHandle(item));

	RemoveChildrenFromInternalMap(item);

	TVITEMEX tvItem = {};
	tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
	tvItem.hItem = item;
	[[maybe_unused]] bool itemRetrieved = TreeView_GetItem(m_hTreeView, &tvItem);
	assert(itemRetrieved);

	auto parent = TreeView_GetParent(m_hTreeView, item);
	assert(parent);

	bool multipleChildren = ItemHasMultipleChildren(parent);

	if (multipleChildren)
	{
		[[maybe_unused]] bool deleted = TreeView_DeleteItem(m_hTreeView, item);
		assert(deleted);
	}
	else
	{
		// There's no need to remove the item specifically, as this call will collapse the parent
		// and remove its children (i.e. the current item).
		[[maybe_unused]] auto expanded =
			TreeView_Expand(m_hTreeView, parent, TVE_COLLAPSE | TVE_COLLAPSERESET);
		assert(expanded);

		TVITEM tvParentItem = {};
		tvParentItem.mask = TVIF_CHILDREN;
		tvParentItem.hItem = parent;
		tvParentItem.cChildren = 0;
		[[maybe_unused]] auto updated = TreeView_SetItem(m_hTreeView, &tvParentItem);
		assert(updated);

		StopDirectoryMonitoringForItem(GetItemByHandle(parent));
	}

	[[maybe_unused]] auto numErased = m_itemInfoMap.erase(static_cast<int>(tvItem.lParam));
	assert(numErased == 1);
}

bool ShellTreeView::ItemHasMultipleChildren(HTREEITEM item)
{
	auto firstChild = TreeView_GetChild(m_hTreeView, item);

	if (!firstChild)
	{
		return false;
	}

	auto nextSibling = TreeView_GetNextSibling(m_hTreeView, firstChild);

	if (!nextSibling)
	{
		return false;
	}

	return true;
}
