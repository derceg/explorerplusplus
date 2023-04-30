// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellChangeWatcher.h"
#include "../Helper/DropHandler.h"
#include "../Helper/ShellDropTargetWindow.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <optional>

class CachedIcons;
struct Config;
class CoreInterface;
class FileActionHandler;
class TabContainer;

class ShellTreeView : public ShellDropTargetWindow<HTREEITEM>
{
public:
	static ShellTreeView *Create(HWND hParent, CoreInterface *coreInterface,
		TabContainer *tabContainer, FileActionHandler *fileActionHandler, CachedIcons *cachedIcons);

	/* User functions. */
	unique_pidl_absolute GetItemPidl(HTREEITEM hTreeItem) const;
	HTREEITEM LocateItem(PCIDLIST_ABSOLUTE pidlDirectory);
	void SetShowHidden(BOOL bShowHidden);
	void RefreshAllIcons();

	/* Sorting. */
	int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2);

	void StartRenamingSelectedItem();
	void StartRenamingItem(PCIDLIST_ABSOLUTE pidl);
	void ShowPropertiesOfSelectedItem() const;
	void DeleteSelectedItem(bool permanent);
	void CopySelectedItemToClipboard(bool copy);
	void CopyItemToClipboard(PCIDLIST_ABSOLUTE pidl, bool copy);
	void Paste();
	void PasteShortcut();

private:
	static const UINT WM_APP_ICON_RESULT_READY = WM_APP + 1;
	static const UINT WM_APP_SUBFOLDERS_RESULT_READY = WM_APP + 2;

	// This is the same background color as used in the Explorer treeview.
	static inline constexpr COLORREF TREE_VIEW_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	static const UINT DROP_EXPAND_TIMER_ID = 1;
	static const UINT DROP_EXPAND_TIMER_ELAPSE = 800;

	static const LONG DROP_SCROLL_MARGIN_X_96DPI = 10;
	static const LONG DROP_SCROLL_MARGIN_Y_96DPI = 10;

	// Represents an item in the treeview. There are two types of items:
	//
	// 1. Root items. These items appear at the top level of the treeview and have an absolute path.
	// 2. Child items. These appear at every other level. Each child only stores its relative path.
	//
	// Full item paths are then built dynamically.
	class ItemInfo
	{
	public:
		ItemInfo(unique_pidl_child childPidl, ItemInfo *parent) :
			m_childPidl(std::move(childPidl)),
			m_parent(parent)
		{
			assert(parent != nullptr);
		}

		ItemInfo(unique_pidl_absolute pidl) : m_rootPidl(std::move(pidl))
		{
		}

		unique_pidl_absolute GetFullPidl() const
		{
			std::vector<PCITEMID_CHILD> childPidls;
			const ItemInfo *currentItem = this;

			while (currentItem->m_parent != nullptr)
			{
				childPidls.push_back(currentItem->m_childPidl.get());

				currentItem = currentItem->m_parent;
			}

			assert(currentItem->m_rootPidl);
			unique_pidl_absolute fullPidl(ILCloneFull(currentItem->m_rootPidl.get()));

			for (PCITEMID_CHILD childPidl : childPidls | std::views::reverse)
			{
				fullPidl.reset(ILCombine(fullPidl.get(), childPidl));
			}

			return fullPidl;
		}

		void UpdateChildPidl(unique_pidl_child childPidl)
		{
			assert(m_parent != nullptr);

			m_childPidl = std::move(childPidl);
		}

		ULONG GetChangeNotifyId() const
		{
			return m_changeNotifyId;
		}

		void SetChangeNotifyId(ULONG changeNotifyId)
		{
			// The directory for an item should only be monitored once.
			assert(m_changeNotifyId == 0);

			m_changeNotifyId = changeNotifyId;
		}

		void ResetChangeNotifyId()
		{
			m_changeNotifyId = 0;
		}

		ItemInfo *GetParent()
		{
			return m_parent;
		}

	private:
		// This is only used if this item is a root item.
		unique_pidl_absolute m_rootPidl;

		// This is only used if this item is a child item.
		unique_pidl_child m_childPidl;

		// This will be non-zero if the directory associated with this item is being monitored for
		// changes.
		ULONG m_changeNotifyId = 0;

		ItemInfo *m_parent = nullptr;
	};

	struct BasicItemInfo
	{
		BasicItemInfo() = default;

		BasicItemInfo(const BasicItemInfo &other)
		{
			pidl.reset(ILCloneFull(other.pidl.get()));
		}

		unique_pidl_absolute pidl;
	};

	struct IconResult
	{
		HTREEITEM item;
		int internalIndex;
		int iconIndex;
	};

	struct SubfoldersResult
	{
		HTREEITEM item;
		bool hasSubfolder;
	};

	ShellTreeView(HWND hParent, CoreInterface *coreInterface, TabContainer *tabContainer,
		FileActionHandler *fileActionHandler, CachedIcons *cachedIcons);
	~ShellTreeView();

	static HWND CreateTreeView(HWND parent);

	static LRESULT CALLBACK TreeViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK CompareItemsStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	HTREEITEM AddRoot();
	HRESULT ExpandDirectory(HTREEITEM hParent);
	HTREEITEM AddItem(HTREEITEM parent, PCIDLIST_ABSOLUTE pidl);
	void SortChildren(HTREEITEM parent);
	void RemoveChildrenFromInternalMap(HTREEITEM hParent);
	void OnGetDisplayInfo(NMTVDISPINFO *pnmtvdi);
	void OnItemExpanding(const NMTREEVIEW *nmtv);
	LRESULT OnKeyDown(const NMTVKEYDOWN *keyDown);
	void OnMiddleButtonDown(const POINT *pt);
	void OnMiddleButtonUp(const POINT *pt, UINT keysDown);
	bool OnEndLabelEdit(const NMTVDISPINFO *dispInfo);

	void CopyItemToClipboard(HTREEITEM treeItem, bool copy);
	void UpdateCurrentClipboardObject(wil::com_ptr_nothrow<IDataObject> clipboardDataObject);
	void OnClipboardUpdate();

	unique_pidl_absolute GetSelectedItemPidl() const;

	// Directory monitoring
	void StartDirectoryMonitoringForDrives();
	void StartDirectoryMonitoringForItem(ItemInfo &item);
	void StopDirectoryMonitoringForItem(ItemInfo &item);
	void StopDirectoryMonitoringForItemAndChildren(ItemInfo &item);
	void RestartDirectoryMonitoringForItemAndChildren(ItemInfo &item);
	void RestartDirectoryMonitoringForItem(ItemInfo &item);
	void ProcessShellChangeNotifications(
		const std::vector<ShellChangeNotification> &shellChangeNotifications);
	void ProcessShellChangeNotification(const ShellChangeNotification &change);
	void OnItemAdded(PCIDLIST_ABSOLUTE simplePidl);
	void OnItemRenamed(PCIDLIST_ABSOLUTE simplePidlOld, PCIDLIST_ABSOLUTE simplePidlNew);
	void OnItemUpdated(PCIDLIST_ABSOLUTE simplePidl);
	void OnItemRemoved(PCIDLIST_ABSOLUTE simplePidl);
	void RemoveItem(HTREEITEM item);
	bool ItemHasMultipleChildren(HTREEITEM item);

	/* Icons. */
	void QueueIconTask(HTREEITEM item, int internalIndex);
	static std::optional<IconResult> FindIconAsync(HWND treeView, int iconResultId, HTREEITEM item,
		int internalIndex, PCIDLIST_ABSOLUTE pidl);
	void ProcessIconResult(int iconResultId);
	std::optional<int> GetCachedIconIndex(const ItemInfo &itemInfo);

	void QueueSubfoldersTask(HTREEITEM item);
	static std::optional<SubfoldersResult> CheckSubfoldersAsync(HWND treeView,
		int subfoldersResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl);
	void ProcessSubfoldersResult(int subfoldersResultId);

	/* Item id's. */
	int GenerateUniqueItemId();

	const ItemInfo &GetItemByHandle(HTREEITEM item) const;
	ItemInfo &GetItemByHandle(HTREEITEM item);
	int GetItemInternalIndex(HTREEITEM item) const;

	// ShellDropTargetWindow
	HTREEITEM GetDropTargetItem(const POINT &pt) override;
	unique_pidl_absolute GetPidlForTargetItem(HTREEITEM targetItem) override;
	IUnknown *GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl) override;
	bool IsTargetSourceOfDrop(HTREEITEM targetItem, IDataObject *dataObject) override;
	void UpdateUiForDrop(HTREEITEM targetItem, const POINT &pt) override;
	void ResetDropUiState() override;

	/* Drag and drop. */
	void UpdateUiForTargetItem(HTREEITEM targetItem);
	void ScrollTreeViewForDrop(const POINT &pt);
	void OnDropExpandTimer();
	HRESULT OnBeginDrag(int iItemId);

	/* Icon refresh. */
	void RefreshAllIconsInternal(HTREEITEM hFirstSibling);

	HTREEITEM LocateExistingItem(PCIDLIST_ABSOLUTE pidlDirectory);
	HTREEITEM LocateItemInternal(PCIDLIST_ABSOLUTE pidlDirectory, BOOL bOnlyLocateExistingItem);
	void UpdateItemState(HTREEITEM item, UINT stateMask, UINT state);

	void OnApplicationShuttingDown();

	HWND m_hTreeView;
	BOOL m_bShowHidden;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
	const Config *m_config;
	TabContainer *m_tabContainer;
	FileActionHandler *m_fileActionHandler;

	ctpl::thread_pool m_iconThreadPool;
	std::unordered_map<int, std::future<std::optional<IconResult>>> m_iconResults;
	int m_iconResultIDCounter;

	ctpl::thread_pool m_subfoldersThreadPool;
	std::unordered_map<int, std::future<std::optional<SubfoldersResult>>> m_subfoldersResults;
	int m_subfoldersResultIDCounter;

	/* Item id's and info. */
	std::unordered_map<int, ItemInfo> m_itemInfoMap;
	int m_itemIDCounter;
	CachedIcons *m_cachedIcons;

	int m_iFolderIcon;

	HTREEITEM m_middleButtonItem;

	/* Drag and drop. */
	UINT m_getDragImageMessage;
	HTREEITEM m_dropExpandItem;
	BOOL m_bDragCancelled;
	BOOL m_bDragAllowed;

	HTREEITEM m_cutItem;
	wil::com_ptr_nothrow<IDataObject> m_clipboardDataObject;

	// Directory monitoring
	ShellChangeWatcher m_shellChangeWatcher;
};
