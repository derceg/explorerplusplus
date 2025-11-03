// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "App.h"
#include "FeatureList.h"
#include "ShellTreeNode.h"
#include "../Helper/ScopedRedrawDisabler.h"

void ShellTreeView::StartDirectoryMonitoringForNode(ShellTreeNode *node)
{
	// Note that both folders and files are monitored for changes. Although this class displays
	// folders, certain types of container files (e.g. .7z, .cab, .rar, .zip) act like folders as
	// well. Those items will be displayed and need to be updated when necessary.
	node->SetDirectoryWatcher(m_app->GetDirectoryWatcherFactory()->MaybeCreate(
		node->GetFullPidl().get(), DirectoryWatcher::Filters::All,
		std::bind_front(&ShellTreeView::ProcessDirectoryChangeNotification, this),
		DirectoryWatcher::Behavior::NonRecursive));
}

void ShellTreeView::StopDirectoryMonitoringForNode(ShellTreeNode *node)
{
	node->SetDirectoryWatcher(nullptr);
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
	if (!node->GetDirectoryWatcher())
	{
		// Directory monitoring should only be restarted if it's already in place. If it's not in
		// place (e.g. because the node isn't expanded), there's no need to try to restart it.
		return;
	}

	StopDirectoryMonitoringForNode(node);
	StartDirectoryMonitoringForNode(node);
}

void ShellTreeView::ProcessDirectoryChangeNotification(DirectoryWatcher::Event event,
	const PidlAbsolute &simplePidl1, const PidlAbsolute &simplePidl2)
{
	switch (event)
	{
	case DirectoryWatcher::Event::Added:
		OnItemAdded(simplePidl1.Raw());
		break;

	case DirectoryWatcher::Event::Renamed:
		OnItemUpdated(simplePidl1.Raw(), simplePidl2.Raw());
		break;

	case DirectoryWatcher::Event::Modified:
		OnItemUpdated(simplePidl1.Raw(), nullptr);
		break;

	case DirectoryWatcher::Event::DirectoryContentsChanged:
		OnDirectoryUpdated(simplePidl1.Raw());
		break;

	case DirectoryWatcher::Event::Removed:
		OnItemRemoved(simplePidl1.Raw());
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

	PidlAbsolute pidlFull;
	HRESULT hr = UpdatePidl(simplePidl, pidlFull);

	PCIDLIST_ABSOLUTE pidl;

	if (SUCCEEDED(hr))
	{
		pidl = pidlFull.Raw();
	}
	else
	{
		pidl = simplePidl;
	}

	AddItem(parentItem, pidl);
	SortChildren(parentItem);
}

// Item renames and updates are both handled by this function. That makes more sense than handling
// them separately, since:
//
// - For some shell items (e.g. the recycle bin), renaming the item generates a SHCNE_UPDATEITEM
// notification only. That's likely because the parsing name hasn't changed, only the display name.
// - The set of properties that need to be updated in response to a rename/update are very similar.
//
// simpleUpdatedPidl will be null if this function was called in response to an update (as opposed
// to a rename).
void ShellTreeView::OnItemUpdated(PCIDLIST_ABSOLUTE simplePidl, PCIDLIST_ABSOLUTE simpleUpdatedPidl)
{
	auto item = LocateExistingItem(simplePidl);

	if (!item)
	{
		return;
	}

	PCIDLIST_ABSOLUTE currentPidl = simplePidl;

	if (simpleUpdatedPidl)
	{
		currentPidl = simpleUpdatedPidl;
	}

	ShellTreeNode *node = GetNodeFromTreeViewItem(item);
	node->UpdateItemDetails(currentPidl);

	// Directory monitoring only needs to be restarted if the parsing name changed. Updates to the
	// display name aren't relevant.
	if (simpleUpdatedPidl)
	{
		RestartDirectoryMonitoringForNodeAndChildren(node);
	}

	// The display name might have changed, even if the item wasn't renamed, so the updated display
	// name should always be retrieved.
	wil::unique_cotaskmem_string displayName;
	HRESULT hr = node->GetShellItem()->GetDisplayName(DISPLAY_NAME_TYPE, &displayName);

	if (FAILED(hr))
	{
		assert(false);

		displayName = wil::make_cotaskmem_string_nothrow(L"");
	}

	// The image and child count for the item will need to be invalidated, as a sub-folder may have
	// been added/removed, or the item's icon may have changed.
	TVITEM tvItemUpdate = {};
	tvItemUpdate.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_TEXT | TVIF_STATE;
	tvItemUpdate.hItem = item;
	tvItemUpdate.iImage = I_IMAGECALLBACK;
	tvItemUpdate.iSelectedImage = I_IMAGECALLBACK;
	tvItemUpdate.cChildren = I_CHILDRENCALLBACK;
	tvItemUpdate.pszText = displayName.get();
	tvItemUpdate.stateMask = TVIS_CUT;
	tvItemUpdate.state = ShouldGhostItem(item) ? TVIS_CUT : 0;
	[[maybe_unused]] auto updated = TreeView_SetItem(m_hTreeView, &tvItemUpdate);
	assert(updated);

	auto parent = TreeView_GetParent(m_hTreeView, item);

	// Even if the parsing name hasn't changed, the display name might have changed, in which case
	// the items should be sorted.
	if (parent)
	{
		SortChildren(parent);
	}
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

	auto parent = TreeView_GetParent(m_hTreeView, item);

	[[maybe_unused]] bool deleted = TreeView_DeleteItem(m_hTreeView, item);
	assert(deleted);

	if (parent)
	{
		auto *parentNode = node->GetParent();
		parentNode->RemoveChild(node);

		if (parentNode->GetChildren().empty())
		{
			TVITEM tvParentItem = {};
			tvParentItem.mask = TVIF_CHILDREN;
			tvParentItem.hItem = parent;
			tvParentItem.cChildren = 0;
			[[maybe_unused]] auto parentUpdated = TreeView_SetItem(m_hTreeView, &tvParentItem);
			assert(parentUpdated);

			StopDirectoryMonitoringForNode(parentNode);
		}
	}
	else
	{
		// If the item has no parent, it's a root node.
		[[maybe_unused]] auto numErased =
			std::erase_if(m_nodes, [node](const auto &rootNode) { return rootNode.get() == node; });
		assert(numErased == 1);
	}
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

	if (!ArePidlsEquivalent(simplePidl, quickAccessRootNode->GetFullPidl().get()))
	{
		return;
	}

	if (!m_app->GetFeatureList()->IsEnabled(Feature::AutomaticQuickAccessUpdates))
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
