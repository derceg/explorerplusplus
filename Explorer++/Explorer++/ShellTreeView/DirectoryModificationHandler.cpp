// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "ShellTreeNode.h"

// Starts monitoring for drive additions and removals, since they won't necessarily trigger updates
// in the parent folder.
void ShellTreeView::StartDirectoryMonitoringForDrives()
{
	// A pidl is needed in order to start monitoring, though in this case it's not relevant, since
	// only drive events are monitored. Therefore, the pidl for the root folder (which should always
	// be available) is used.
	unique_pidl_absolute pidl;
	[[maybe_unused]] HRESULT hr = GetRootPidl(wil::out_param(pidl));
	assert(SUCCEEDED(hr));

	m_shellChangeWatcher.StartWatching(pidl.get(), SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED);
}

void ShellTreeView::StartDirectoryMonitoringForNode(ShellTreeNode *node)
{
	auto pidl = node->GetFullPidl();
	node->SetChangeNotifyId(m_shellChangeWatcher.StartWatching(pidl.get(),
		SHCNE_ATTRIBUTES | SHCNE_MKDIR | SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_UPDATEDIR
			| SHCNE_UPDATEITEM));
}

void ShellTreeView::StopDirectoryMonitoringForNode(ShellTreeNode *node)
{
	if (node->GetChangeNotifyId() == 0)
	{
		return;
	}

	m_shellChangeWatcher.StopWatching(node->GetChangeNotifyId());
	node->ResetChangeNotifyId();
}

void ShellTreeView::StopDirectoryMonitoringForNodeAndChildren(ShellTreeNode *node)
{
	StopDirectoryMonitoringForNode(node);

	for (auto &child : node->GetChildren())
	{
		StopDirectoryMonitoringForNodeAndChildren(child.get());
	}
}

// When a directory is renamed, the directory monitoring which was originally set up will no longer
// work (since it's tied to a specific path). In that case, the directory monitoring for that item
// will need to be restarted. The directory monitoring for any children will also need to be
// restarted (when necessary), since their paths will have changed as well.
void ShellTreeView::RestartDirectoryMonitoringForNodeAndChildren(ShellTreeNode *node)
{
	RestartDirectoryMonitoringForNode(node);

	for (auto &child : node->GetChildren())
	{
		RestartDirectoryMonitoringForNodeAndChildren(child.get());
	}
}

void ShellTreeView::RestartDirectoryMonitoringForNode(ShellTreeNode *node)
{
	if (node->GetChangeNotifyId() == 0)
	{
		return;
	}

	StopDirectoryMonitoringForNode(node);
	StartDirectoryMonitoringForNode(node);
}

void ShellTreeView::ProcessShellChangeNotifications(
	const std::vector<ShellChangeNotification> &shellChangeNotifications)
{
	SendMessage(m_hTreeView, WM_SETREDRAW, FALSE, 0);

	for (const auto &change : shellChangeNotifications)
	{
		ProcessShellChangeNotification(change);
	}

	SendMessage(m_hTreeView, WM_SETREDRAW, TRUE, 0);
}

void ShellTreeView::ProcessShellChangeNotification(const ShellChangeNotification &change)
{
	switch (change.event)
	{
	case SHCNE_DRIVEADD:
	case SHCNE_MKDIR:
		OnItemAdded(change.pidl1.get());
		break;

	case SHCNE_RENAMEFOLDER:
		OnItemRenamed(change.pidl1.get(), change.pidl2.get());
		break;

	case SHCNE_UPDATEITEM:
		OnItemUpdated(change.pidl1.get());
		break;

	case SHCNE_DRIVEREMOVED:
	case SHCNE_RMDIR:
		OnItemRemoved(change.pidl1.get());
		break;

	case SHCNE_UPDATEDIR:
		OnDirectoryUpdated(change.pidl1.get());
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
	SortChildren(parentItem);
}

void ShellTreeView::OnItemRenamed(PCIDLIST_ABSOLUTE simplePidlOld, PCIDLIST_ABSOLUTE simplePidlNew)
{
	auto item = LocateExistingItem(simplePidlOld);

	if (!item)
	{
		return;
	}

	ShellTreeNode *node = GetNodeFromTreeViewItem(item);
	node->UpdateChildPidl(unique_pidl_child(ILCloneChild(ILFindLastID(simplePidlNew))));

	RestartDirectoryMonitoringForNodeAndChildren(node);

	std::wstring name;
	HRESULT hr = GetDisplayName(simplePidlNew, SHGDN_NORMAL, name);

	if (FAILED(hr))
	{
		return;
	}

	TVITEM tvItemUpdate = {};
	tvItemUpdate.mask = TVIF_TEXT;
	tvItemUpdate.hItem = item;
	tvItemUpdate.pszText = name.data();
	[[maybe_unused]] auto updated = TreeView_SetItem(m_hTreeView, &tvItemUpdate);
	assert(updated);

	auto parent = TreeView_GetParent(m_hTreeView, item);

	if (parent)
	{
		SortChildren(parent);
	}
}

void ShellTreeView::OnItemUpdated(PCIDLIST_ABSOLUTE simplePidl)
{
	auto item = LocateExistingItem(simplePidl);

	if (!item)
	{
		return;
	}

	// An SHCNE_UPDATEITEM notification will be sent to a folder when one of the items within it
	// changes. The image and child count for the item will need to be invalidated, as a sub-folder
	// may have been added/removed, or the item's icon may have changed.
	TVITEM tvItemUpdate = {};
	tvItemUpdate.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvItemUpdate.hItem = item;
	tvItemUpdate.iImage = I_IMAGECALLBACK;
	tvItemUpdate.iSelectedImage = I_IMAGECALLBACK;
	tvItemUpdate.cChildren = I_CHILDRENCALLBACK;
	[[maybe_unused]] auto updated = TreeView_SetItem(m_hTreeView, &tvItemUpdate);
	assert(updated);
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
	auto *node = GetNodeFromTreeViewItem(item);
	StopDirectoryMonitoringForNodeAndChildren(node);

	TVITEMEX tvItem = {};
	tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
	tvItem.hItem = item;
	[[maybe_unused]] bool itemRetrieved = TreeView_GetItem(m_hTreeView, &tvItem);
	assert(itemRetrieved);

	bool deleteIndividualItem;
	auto parent = TreeView_GetParent(m_hTreeView, item);

	// It's valid for the item that's being removed to have no parent, since root items may be
	// dynamically added and removed.
	if (parent)
	{
		deleteIndividualItem = ItemHasMultipleChildren(parent);
	}
	else
	{
		// This is a root item.
		deleteIndividualItem = true;
	}

	if (deleteIndividualItem)
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

		StopDirectoryMonitoringForNode(node->GetParent());
	}

	if (parent)
	{
		node->GetParent()->RemoveChild(node);
	}
	else
	{
		[[maybe_unused]] auto numErased =
			std::erase_if(m_nodes, [node](const auto &rootNode) { return rootNode.get() == node; });
		assert(numErased == 1);
	}
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

// This notification will also be generated for other directories. However, it's difficult to
// handle in the general case. For example, if the desktop root folder is expanded several layers
// deep and a change notification is generated for it, simply collapsing and re-expanding the
// folder will cause it to lose track of what the user expanded.
// It would also  be unusual to have an entire tree node collapse and re-expand for seemingly no
// reason.
// A better solution might be to update the node in-place. But that's potentially tricky to get
// right.
// Therefore, this notification is only handled for the quick access folder as of now. Collapsing
// and re-expanding that folder specifically isn't too bad, since it will only contain a small
// number of items at a single level.
void ShellTreeView::OnDirectoryUpdated(PCIDLIST_ABSOLUTE simplePidl)
{
	// Whether or not the quick access item is shown is user-configurable.
	if (!m_quickAccessRootItem)
	{
		return;
	}

	ShellTreeNode *quickAccessRootNode = GetNodeFromTreeViewItem(m_quickAccessRootItem);
	unique_pidl_absolute quickAccessPidl = quickAccessRootNode->GetFullPidl();

	if (!ArePidlsEquivalent(simplePidl, quickAccessPidl.get()))
	{
		return;
	}

	auto selectedItem = TreeView_GetSelection(m_hTreeView);
	unique_pidl_absolute selectedItemPidl;

	if (selectedItem)
	{
		ShellTreeNode *selectedNode = GetNodeFromTreeViewItem(selectedItem);
		selectedItemPidl = selectedNode->GetFullPidl();
	}

	StopDirectoryMonitoringForNodeAndChildren(quickAccessRootNode);
	quickAccessRootNode->RemoveAllChildren();

	SendMessage(m_hTreeView, TVM_EXPAND, TVE_COLLAPSE | TVE_COLLAPSERESET,
		reinterpret_cast<LPARAM>(m_quickAccessRootItem));

	SendMessage(m_hTreeView, TVM_EXPAND, TVE_EXPAND,
		reinterpret_cast<LPARAM>(m_quickAccessRootItem));

	if (selectedItemPidl)
	{
		auto previouslySelectedItem = LocateExistingItem(selectedItemPidl.get());

		// The previously selected item might not exist anymore (e.g. if the selection was a pinned
		// item that has been unpinned).
		if (previouslySelectedItem)
		{
			TreeView_SelectItem(m_hTreeView, previouslySelectedItem);
		}
	}
}
