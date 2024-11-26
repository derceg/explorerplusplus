// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include "ShellChangeWatcher.h"
#include "SignalWrapper.h"
#include "../Helper/DropHandler.h"
#include "../Helper/ShellContextMenu.h"
#include "../Helper/ShellDropTargetWindow.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <optional>

class App;
class BrowserWindow;
class CachedIcons;
struct Config;
class CoreInterface;
class FileActionHandler;
class ShellBrowserImpl;
class ShellTreeNode;

class ShellTreeView : public ShellDropTargetWindow<HTREEITEM>, public ShellContextMenuHandler
{
public:
	static ShellTreeView *Create(HWND hParent, App *app, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, FileActionHandler *fileActionHandler,
		CachedIcons *cachedIcons);

	/* User functions. */
	unique_pidl_absolute GetSelectedNodePidl() const;
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

	// FileContextMenuHandler
	void UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu) override;
	std::wstring GetHelpTextForItem(UINT menuItemId) override;
	bool HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		UINT menuItemId) override;

private:
	static const UINT WM_APP_ICON_RESULT_READY = WM_APP + 1;
	static const UINT WM_APP_SUBFOLDERS_RESULT_READY = WM_APP + 2;

	static const UINT DROP_EXPAND_TIMER_ID = 1;
	static const UINT DROP_EXPAND_TIMER_TIMEOUT = 800;

	static const UINT SELECTION_CHANGED_TIMER_ID = 2;
	static const UINT SELECTION_CHANGED_TIMEOUT = 500;

	static const LONG DROP_SCROLL_MARGIN_X_96DPI = 10;
	static const LONG DROP_SCROLL_MARGIN_Y_96DPI = 10;

	static const SIGDN DISPLAY_NAME_TYPE = SIGDN_NORMALDISPLAY;

	static const int OPEN_IN_NEW_TAB_MENU_ITEM_ID = ShellContextMenu::MAX_SHELL_MENU_ID + 1;

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
		int nodeId;
		HTREEITEM treeItem;
		int iconIndex;
	};

	struct SubfoldersResult
	{
		HTREEITEM item;
		bool hasSubfolder;
	};

	// Maintains information about an item that was cut or copied within the treeview.
	class CutCopiedItemManager
	{
	public:
		void SetCopiedItem(IDataObject *clipboardDataObject);
		void SetCutItem(HTREEITEM cutItem, IDataObject *clipboardDataObject);

		void ClearCutCopiedItem();

		HTREEITEM GetCutItem() const;
		IDataObject *GetCutCopiedClipboardDataObject() const;

		// Signals
		SignalWrapper<CutCopiedItemManager, void(HTREEITEM previousCutItem, HTREEITEM newCutItem)>
			cutItemChangedSignal;

	private:
		void SetDataInternal(HTREEITEM cutItem, IDataObject *clipboardDataObject);

		HTREEITEM m_cutItem = nullptr;
		wil::com_ptr_nothrow<IDataObject> m_clipboardDataObject;
	};

	ShellTreeView(HWND hParent, App *app, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, FileActionHandler *fileActionHandler,
		CachedIcons *cachedIcons);
	~ShellTreeView();

	static HWND CreateTreeView(HWND parent);

	LRESULT TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK CompareItemsStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	void AddRootItems();
	void AddQuickAccessRootItem();
	static HRESULT ParseShellFolderNameAndCheckExistence(const std::wstring &shellFolderPath,
		PIDLIST_ABSOLUTE *pidl);
	void AddShellNamespaceRootItem();
	HTREEITEM AddRootItem(PCIDLIST_ABSOLUTE pidl, HTREEITEM insertAfter = TVI_LAST);
	void OnShowQuickAccessUpdated(bool newValue);
	HRESULT ExpandDirectory(HTREEITEM hParent);
	HTREEITEM AddItem(HTREEITEM parent, PCIDLIST_ABSOLUTE pidl, HTREEITEM insertAfter = TVI_LAST);
	void SortChildren(HTREEITEM parent);
	void OnGetDisplayInfo(NMTVDISPINFO *pnmtvdi);
	void OnSelectionChanged(const NMTREEVIEW *eventInfo);
	void OnSelectionChangedTimer();
	void HandleSelectionChanged(const NMTREEVIEW *eventInfo);
	void OnItemExpanding(const NMTREEVIEW *nmtv);
	LRESULT OnKeyDown(const NMTVKEYDOWN *keyDown);
	void OnMiddleButtonDown(const POINT *pt);
	void OnMiddleButtonUp(const POINT *pt, UINT keysDown);
	bool OnBeginLabelEdit(const NMTVDISPINFO *dispInfo);
	bool OnEndLabelEdit(const NMTVDISPINFO *dispInfo);
	void OnShowContextMenu(const POINT &ptScreen);
	void UpdateSelection();

	void CopyItemToClipboard(HTREEITEM treeItem, bool copy);
	void OnClipboardUpdate();

	unique_pidl_absolute GetNodePidl(HTREEITEM hTreeItem) const;

	// Directory monitoring
	void StartDirectoryMonitoringForDrives();
	void StartDirectoryMonitoringForNode(ShellTreeNode *node);
	void StopDirectoryMonitoringForNode(ShellTreeNode *node);
	void StopDirectoryMonitoringForNodeAndChildren(ShellTreeNode *node);
	void RestartDirectoryMonitoringForNodeAndChildren(ShellTreeNode *node);
	void RestartDirectoryMonitoringForNode(ShellTreeNode *node);
	void ProcessShellChangeNotifications(
		const std::vector<ShellChangeNotification> &shellChangeNotifications);
	void ProcessShellChangeNotification(const ShellChangeNotification &change);
	void OnItemAdded(PCIDLIST_ABSOLUTE simplePidl);
	void OnItemUpdated(PCIDLIST_ABSOLUTE simplePidl, PCIDLIST_ABSOLUTE simpleUpdatedPidl);
	void OnItemRemoved(PCIDLIST_ABSOLUTE simplePidl);
	void RemoveItem(HTREEITEM item);
	void OnDirectoryUpdated(PCIDLIST_ABSOLUTE simplePidl);

	/* Icons. */
	void QueueIconTask(HTREEITEM treeItem);
	static std::optional<IconResult> FindIconAsync(HWND treeView, int iconResultId, int nodeId,
		HTREEITEM treeItem, PCIDLIST_ABSOLUTE pidl);
	void ProcessIconResult(int iconResultId);
	std::optional<int> GetCachedIconIndex(const ShellTreeNode *node);

	void QueueSubfoldersTask(HTREEITEM item);
	static std::optional<SubfoldersResult> CheckSubfoldersAsync(HWND treeView,
		int subfoldersResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl);
	void ProcessSubfoldersResult(int subfoldersResultId);

	ShellTreeNode *GetNodeFromTreeViewItem(HTREEITEM item) const;
	ShellTreeNode *GetNodeById(int id) const;
	ShellTreeNode *GetNodeByIdRecursive(ShellTreeNode *node, int id) const;

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
	HRESULT OnBeginDrag(const ShellTreeNode *node);

	/* Icon refresh. */
	void RefreshAllIconsInternal(HTREEITEM hFirstSibling);

	HTREEITEM LocateItem(PCIDLIST_ABSOLUTE pidlDirectory);
	HTREEITEM LocateExistingItem(PCIDLIST_ABSOLUTE pidlDirectory);
	HTREEITEM LocateItemInternal(PCIDLIST_ABSOLUTE pidlDirectory, BOOL bOnlyLocateExistingItem);

	void OnCutItemChanged(HTREEITEM previousCutItem, HTREEITEM newCutItem);
	bool ShouldGhostItem(HTREEITEM item);
	bool TestItemAttributes(ShellTreeNode *node, SFGAOF attributes);
	void UpdateItemState(HTREEITEM item, UINT stateMask, UINT state);

	ShellBrowserImpl *GetSelectedShellBrowser() const;

	HWND m_hTreeView;
	App *const m_app;
	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;
	HTREEITEM m_quickAccessRootItem = nullptr;
	BOOL m_bShowHidden;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
	const Config *m_config;
	FileActionHandler *m_fileActionHandler;
	bool m_browserInitialized = false;

	// Note that the treeview control sets the font on the tooltip control it creates each time the
	// tooltip is shown (which can be seen by logging WM_SETFONT calls made on the tooltip control).
	// That means there's no need to manually set the tooltip font; only the treeview font needs to
	// be set. Once the treeview font is set, the same font will be applied to the tooltip control.
	MainFontSetter m_fontSetter;

	ctpl::thread_pool m_iconThreadPool;
	std::unordered_map<int, std::future<std::optional<IconResult>>> m_iconResults;
	int m_iconResultIDCounter;

	ctpl::thread_pool m_subfoldersThreadPool;
	std::unordered_map<int, std::future<std::optional<SubfoldersResult>>> m_subfoldersResults;
	int m_subfoldersResultIDCounter;

	// Contains information about each node stored in the tree. Only root nodes are stored directly
	// in this vector; child nodes are stored underneath their parent node.
	std::vector<std::unique_ptr<ShellTreeNode>> m_nodes;

	CachedIcons *m_cachedIcons;

	int m_iFolderIcon;

	HTREEITEM m_middleButtonItem;

	std::optional<NMTREEVIEW> m_selectionChangedEventInfo;

	/* Drag and drop. */
	UINT m_getDragImageMessage;
	HTREEITEM m_dropExpandItem;
	BOOL m_bDragCancelled;
	BOOL m_bDragAllowed;
	bool m_performingDrag = false;
	PCIDLIST_ABSOLUTE m_draggedItemPidl = nullptr;

	CutCopiedItemManager m_cutCopiedItemManager;

	// Directory monitoring
	ShellChangeWatcher m_shellChangeWatcher;
};
