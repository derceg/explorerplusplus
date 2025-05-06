// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserCommandTarget.h"
#include "ClipboardOperations.h"
#include "ColumnDataRetrieval.h"
#include "Columns.h"
#include "FileSystemChangeWatcher.h"
#include "FolderSettings.h"
#include "MainFontSetter.h"
#include "NavigationManager.h"
#include "ScopedBrowserCommandTarget.h"
#include "ServiceProvider.h"
#include "ShellBrowser.h"
#include "ShellChangeWatcher.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/ScopedStopSource.h"
#include "../Helper/ShellDropTargetWindow.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include "../Helper/WinRTBaseWrapper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/core/noncopyable.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <thumbcache.h>
#include <future>
#include <list>
#include <memory>
#include <optional>
#include <stop_token>
#include <unordered_map>
#include <unordered_set>

class AcceleratorManager;
class App;
struct BasicItemInfo_t;
class BrowserCommandTargetManager;
class CachedIcons;
struct Config;
class FileActionHandler;
class IconFetcher;
class NavigationRequest;
struct PreservedFolderState;
class PreservedHistoryEntry;
class Runtime;
class ShellEnumeratorImpl;
class ShellNavigationController;
class TabNavigationInterface;
class WindowSubclass;

typedef struct
{
	ULARGE_INTEGER TotalFolderSize;
	ULARGE_INTEGER TotalSelectionSize;
} FolderInfo_t;

class ShellBrowserImpl :
	public ShellBrowser,
	public ShellDropTargetWindow<int>,
	public BrowserCommandTarget,
	private boost::noncopyable
{
public:
	ShellBrowserImpl(HWND hOwner, App *app, TabNavigationInterface *tabNavigation,
		BrowserCommandTargetManager *commandTargetManager, FileActionHandler *fileActionHandler,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState);
	ShellBrowserImpl(HWND hOwner, App *app, TabNavigationInterface *tabNavigation,
		BrowserCommandTargetManager *commandTargetManager, FileActionHandler *fileActionHandler,
		const PidlAbsolute &initialPidl, const FolderSettings &folderSettings,
		const FolderColumns *initialColumns);
	~ShellBrowserImpl();

	HWND GetListView() const;

	// ShellBrowser
	FolderSettings GetFolderSettings() const override;
	ShellNavigationController *GetNavigationController() const override;
	bool CanCreateNewFolder() const override;
	void CreateNewFolder() override;

	WeakPtr<ShellBrowserImpl> GetWeakPtr();

	/* Get/Set current state. */
	unique_pidl_absolute GetDirectoryIdl() const;
	std::wstring GetDirectory() const;
	int GetUniqueFolderId() const;
	bool GetAutoArrange() const;
	void SetAutoArrange(bool autoArrange);
	ViewMode GetViewMode() const;
	void SetViewMode(ViewMode viewMode);
	void CycleViewMode(bool cycleForward);
	SortMode GetSortMode() const;
	void SetSortMode(SortMode sortMode);
	SortMode GetGroupMode() const;
	void SetGroupMode(SortMode sortMode);
	SortDirection GetSortDirection() const;
	void SetSortDirection(SortDirection direction);
	SortDirection GetGroupSortDirection() const;
	void SetGroupSortDirection(SortDirection direction);
	bool GetShowHidden() const;
	void SetShowHidden(bool showHidden);
	int GetNumItems() const;
	int GetNumSelectedFiles() const;
	int GetNumSelectedFolders() const;
	int GetNumSelected() const;

	/* Item information. */
	WIN32_FIND_DATA GetItemFileFindData(int index) const;
	unique_pidl_absolute GetItemCompleteIdl(int index) const;
	unique_pidl_child GetItemChildIdl(int index) const;
	std::wstring GetItemName(int index) const;
	std::wstring GetItemDisplayName(int index) const;
	std::wstring GetItemFullName(int index) const;

	void ShowPropertiesForSelectedItems() const;

	/* Column support. */
	std::vector<Column_t> GetCurrentColumns();
	void SetCurrentColumns(const std::vector<Column_t> &columns);
	static SortMode DetermineColumnSortMode(ColumnType columnType);

	/* Filtering. */
	std::wstring GetFilterText() const;
	void SetFilterText(std::wstring_view filter);
	bool IsFilterApplied() const;
	void SetFilterApplied(bool filter);
	bool GetFilterCaseSensitive() const;
	void SetFilterCaseSensitive(bool filterCaseSensitive);

	bool TestListViewItemAttributes(int item, SFGAOF attributes) const;
	HRESULT GetListViewSelectionAttributes(SFGAOF *attributes) const;

	void SetFileAttributesForSelection();

	void SelectItems(const std::vector<PidlAbsolute> &pidls);
	uint64_t GetTotalDirectorySize();
	uint64_t GetSelectionSize();
	int LocateFileItemIndex(const TCHAR *szFileName) const;
	bool InVirtualFolder() const;
	BOOL CanCreate() const;
	HRESULT CopySelectedItemsToClipboard(bool copy);
	void PasteShortcut();
	void PasteHardLinks();
	void PasteSymLinks();
	void OnInternalPaste(const ClipboardOperations::PastedItems &pastedItems);
	void StartRenamingSelectedItems();

	bool GetShowInGroups() const;
	void SetShowInGroups(bool showInGroups);

	int CALLBACK SortTemporary(LPARAM lParam1, LPARAM lParam2);

	std::vector<SortMode> GetAvailableSortModes() const;
	void ImportAllColumns(const FolderColumns &folderColumns);
	FolderColumns ExportAllColumns();
	void QueueRename(PCIDLIST_ABSOLUTE pidlItem);
	void AutoSizeColumns();

	// BrowserCommandTarget
	bool IsCommandEnabled(int command) const override;
	void ExecuteCommand(int command) override;

protected:
	NavigationManager *GetNavigationManager() override;
	const NavigationManager *GetNavigationManager() const override;

private:
	struct ItemInfo_t
	{
		PidlAbsolute pidlComplete;
		PidlChild pridl;
		WIN32_FIND_DATA wfd;
		bool isFindDataValid;
		std::wstring parsingName;
		std::wstring displayName;
		std::wstring editingName;

		/* These are only used for drives. They are
		needed for when a drive is removed from the
		system, in which case the drive name is needed
		so that the removed drive can be found. */
		BOOL bDrive;
		TCHAR szDrive[4];

		/* Used for temporary sorting in details mode (i.e.
		when items need to be rearranged). */
		int iRelativeSort;

		ItemInfo_t() : wfd({}), isFindDataValid(false), bDrive(FALSE)
		{
		}
	};

	struct AwaitingAdd_t
	{
		int iItem;
		int iItemInternal;

		BOOL bPosition;
		int iAfter;
	};

	struct Added_t
	{
		TCHAR szFileName[MAX_PATH];
	};

	struct DroppedFile_t
	{
		TCHAR szFileName[MAX_PATH];
		POINT DropPoint;
	};

	struct ColumnResult_t
	{
		int itemInternalIndex;
		ColumnType columnType;
		std::wstring columnText;
	};

	struct ThumbnailResult_t
	{
		int itemInternalIndex;
		wil::unique_hbitmap bitmap;
	};

	struct InfoTipResult
	{
		int itemInternalIndex;
		std::wstring infoTip;
	};

	struct GroupInfo
	{
		std::wstring name;
		int relativeSortPosition;

		explicit GroupInfo(const std::wstring &name) : name(name), relativeSortPosition(0)
		{
		}

		GroupInfo(const std::wstring &name, int relativeSortPosition) :
			name(name),
			relativeSortPosition(relativeSortPosition)
		{
		}
	};

	struct ListViewGroup
	{
	public:
		int id;
		std::wstring name;
		int relativeSortPosition;
		int numItems;

		ListViewGroup(const GroupInfo &groupInfo) :
			id(idCounter++),
			name(groupInfo.name),
			relativeSortPosition(groupInfo.relativeSortPosition),
			numItems(0)
		{
		}

	private:
		static inline int idCounter = 0;
	};

	enum class GroupByDateType
	{
		Created,
		Modified,
		Accessed
	};

	// clang-format off
	using ListViewGroupSet = boost::multi_index_container<ListViewGroup,
		boost::multi_index::indexed_by<
			boost::multi_index::hashed_unique<
				boost::multi_index::member<ListViewGroup, int, &ListViewGroup::id>
			>,
			boost::multi_index::hashed_unique<
				boost::multi_index::member<ListViewGroup, std::wstring, &ListViewGroup::name>
			>
		>
	>;
	// clang-format on

	struct DirectoryState
	{
		PidlAbsolute pidlDirectory;
		std::wstring directory;
		bool virtualFolder;
		int itemIDCounter;

		/* Stores information on files that have
		been created and are awaiting insertion
		into the listview. */
		std::vector<AwaitingAdd_t> awaitingAddList;

		std::unique_ptr<FileSystemChangeWatcher> fileSystemChangeWatcher;

		std::unordered_set<int> filteredItemsList;

		// When an item is pasted or dropped, it will be selected. However, the item may not exist
		// at the time the call is made to select the file. This field keeps track of items in the
		// current directory which need to be selected, once added.
		std::vector<PidlAbsolute> filesToSelect;

		// When an item is created, it may need to be placed into rename mode (e.g. when created via
		// the "New" menu). However, it can take time for the directory change notification to be
		// processed. This field keeps track of an item that needs to be put into rename mode, once
		// it has been added.
		PidlAbsolute queuedRenameItem;

		int numItems;
		int numFilesSelected;
		int numFoldersSelected;
		uint64_t totalDirSize;
		uint64_t fileSelectionSize;

		/* Cached folder size data. */
		mutable std::unordered_map<int, ULONGLONG> cachedFolderSizes;

		// Thumbnails
		// The first imagelist will be used to retrieve item icons in thumbnails mode.
		HIMAGELIST thumbnailsShellImageList = nullptr;
		wil::unique_himagelist thumbnailsImageList;

		ListViewGroupSet groups;

		DirectoryState() :
			virtualFolder(false),
			itemIDCounter(0),
			numItems(0),
			numFilesSelected(0),
			numFoldersSelected(0),
			totalDirSize(0),
			fileSelectionSize(0)
		{
		}
	};

	enum class NavigationState
	{
		NoFolderShown,
		WillCommit,
		Committed
	};

	static const UINT WM_APP_COLUMN_RESULT_READY = WM_APP + 150;
	static const UINT WM_APP_THUMBNAIL_RESULT_READY = WM_APP + 151;
	static const UINT WM_APP_INFO_TIP_READY = WM_APP + 152;

	ShellBrowserImpl(HWND hOwner, App *app, TabNavigationInterface *tabNavigation,
		BrowserCommandTargetManager *commandTargetManager, FileActionHandler *fileActionHandler,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns);

	static HWND CreateListView(HWND parent);
	void InitializeListView();
	void ChangeToInitialFolder();
	int GenerateUniqueItemId();
	void MarkItemAsCut(int item, bool cut);
	void VerifySortMode();

	/* Browsing support. */
	void OnNavigationStarted(const NavigationRequest *request);
	static std::optional<ItemInfo_t> GetItemInformation(IShellFolder *shellFolder,
		PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild);
	void ChangeFolders(const PidlAbsolute &directory);
	void PrepareToChangeFolders();
	void ClearPendingResults();
	void StoreCurrentlySelectedItems();
	void ResetFolderState();
	void OnNavigationWillCommit(const NavigationRequest *request);
	void OnNavigationComitted(const NavigationRequest *request);
	void AddNavigationItems(const NavigationRequest *request,
		const std::vector<PidlChild> &itemPidls);
	std::vector<ItemInfo_t> GetItemInformationFromPidls(const NavigationRequest *request,
		const std::vector<PidlChild> &itemPidls);
	void InsertAwaitingItems();
	BOOL IsFileFiltered(const ItemInfo_t &itemInfo) const;
	std::optional<int> AddItemInternal(IShellFolder *shellFolder, PCIDLIST_ABSOLUTE pidlDirectory,
		PCITEMID_CHILD pidlChild, int itemIndex, BOOL setPosition);
	int AddItemInternal(int itemIndex, const ItemInfo_t &itemInfo, BOOL setPosition);
	static HRESULT ExtractFindDataUsingPropertyStore(IShellFolder *shellFolder,
		PCITEMID_CHILD pidlChild, WIN32_FIND_DATA &output);
	void SetViewModeInternal(ViewMode viewMode);
	void SetFirstColumnTextToCallback();
	void SetFirstColumnTextToFilename();
	void SetNavigationState(NavigationState navigationState);

	// Shell window integration
	void NotifyShellOfNavigation(PCIDLIST_ABSOLUTE pidl);
	HRESULT RegisterShellWindowIfNecessary(PCIDLIST_ABSOLUTE pidl);
	HRESULT RegisterShellWindow(PCIDLIST_ABSOLUTE pidl);

	LRESULT ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK SortStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	/* Message handlers. */
	void ColumnClicked(int iClickedColumn);

	/* Listview. */
	bool OnListViewLeftButtonDoubleClick(const POINT *pt);
	void OnListViewMButtonDown(const POINT *pt);
	void OnListViewMButtonUp(const POINT *pt, UINT keysDown);
	void OnRButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags);
	bool OnMouseWheel(int xPos, int yPos, int delta, UINT keys);
	bool OnSetCursor(HWND target);
	void OnListViewGetDisplayInfo(LPARAM lParam);
	LRESULT OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip);
	BOOL OnListViewGetEmptyMarkup(NMLVEMPTYMARKUP *emptyMarkup);
	void QueueInfoTipTask(int internalIndex, const std::wstring &existingInfoTip);
	static std::optional<InfoTipResult> GetInfoTipAsync(HWND listView, int infoTipResultId,
		int internalIndex, const BasicItemInfo_t &basicItemInfo, const Config &config,
		HINSTANCE resourceInstance, bool virtualFolder);
	void ProcessInfoTipResult(int infoTipResultId);
	void OnListViewItemInserted(const NMLISTVIEW *itemData);
	void OnListViewItemChanged(const NMLISTVIEW *changeData);
	void UpdateFileSelectionInfo(int internalIndex, BOOL selected);
	void OnListViewKeyDown(const NMLVKEYDOWN *lvKeyDown);
	std::vector<PidlAbsolute> GetSelectedItemPidls() const;
	void OnListViewBeginDrag(const NMLISTVIEW *info);
	void OnListViewBeginRightClickDrag(const NMLISTVIEW *info);
	HRESULT StartDrag(int draggedItem, const POINT &startPoint);
	BOOL OnListViewBeginLabelEdit(const NMLVDISPINFO *dispInfo);
	BOOL OnListViewEndLabelEdit(const NMLVDISPINFO *dispInfo);
	LRESULT OnListViewCustomDraw(NMLVCUSTOMDRAW *listViewCustomDraw);
	void OnColorRulesUpdated();
	void OnFullRowSelectUpdated(BOOL newValue);
	void OnCheckBoxSelectionUpdated(BOOL newValue);
	void OnShowGridlinesUpdated(BOOL newValue);
	void OnOneClickActivateUpdated(BOOL newValue);
	void OnOneClickActivateHoverTimeUpdated(UINT newValue);

	bool DoAllSelectedItemsHaveAttributes(SFGAOF attributes) const;
	HRESULT GetListViewItemAttributes(int item, SFGAOF *attributes) const;

	void DeleteSelectedItems(bool permanent);
	void StartRenamingSingleFile();
	void StartRenamingMultipleFiles();

	// Listview header context menu
	void OnListViewHeaderRightClick(const POINTS &cursorPos);
	void OnListViewHeaderMenuItemSelected(int menuItemId,
		const std::unordered_map<int, ColumnType> &menuItemMappings);
	void OnShowMoreColumnsSelected();
	void OnColumnMenuItemSelected(int menuItemId,
		const std::unordered_map<int, ColumnType> &menuItemMappings);

	const ItemInfo_t &GetItemByIndex(int index) const;
	ItemInfo_t &GetItemByIndex(int index);
	int GetItemInternalIndex(int item) const;

	BasicItemInfo_t getBasicItemInfo(int internalIndex) const;

	/* Sorting. */
	void SortFolder();
	int CALLBACK Sort(int InternalIndex1, int InternalIndex2) const;

	/* Listview column support. */
	void AddFirstColumn();
	void SetUpListViewColumns();
	void DeleteAllColumns();
	void QueueColumnTask(int itemInternalIndex, ColumnType columnType);
	static ColumnResult_t GetColumnTextAsync(HWND listView, int columnResultId,
		ColumnType columnType, int internalIndex, const BasicItemInfo_t &basicItemInfo,
		const GlobalFolderSettings &globalFolderSettings);
	void InsertColumn(ColumnType columnType, int columnIndex, int width);
	void SetActiveColumnSet();
	void GetColumnInternal(ColumnType columnType, Column_t *pci) const;
	Column_t GetFirstCheckedColumn();
	void SaveColumnWidths();
	void ProcessColumnResult(int columnResultId);
	std::optional<int> GetColumnIndexByType(ColumnType columnType) const;
	std::optional<ColumnType> GetColumnTypeByIndex(int index) const;

	// Directory change handling
	void StartDirectoryMonitoring();
	void StartDirectoryMonitoringViaShellChangeWatcher();
	void StartDirectoryMonitoringViaFileSystemChangeWatcher();
	void ProcessShellChangeNotifications(
		const std::vector<ShellChangeNotification> &shellChangeNotifications);
	void ProcessShellChangeNotification(const ShellChangeNotification &change);
	void ProcessFileSystemChangeNotification(FileSystemChangeWatcher::Event event,
		const PidlAbsolute &simplePidl1, const PidlAbsolute &simplePidl2);
	void OnItemAdded(PCIDLIST_ABSOLUTE simplePidl);
	void AddItem(PCIDLIST_ABSOLUTE pidl);
	void RemoveItem(int iItemInternal);
	void OnItemRemoved(PCIDLIST_ABSOLUTE simplePidl);
	void OnItemModified(PCIDLIST_ABSOLUTE simplePidl);
	void UpdateItem(PCIDLIST_ABSOLUTE pidl, PCIDLIST_ABSOLUTE updatedPidl = nullptr);
	void OnItemRenamed(PCIDLIST_ABSOLUTE simplePidlOld, PCIDLIST_ABSOLUTE simplePidlNew);
	void InvalidateAllColumnsForItem(int itemIndex);
	void InvalidateIconForItem(int itemIndex);
	int DetermineItemSortedPosition(LPARAM lParam) const;
	static concurrencpp::null_result OnCurrentDirectoryRenamed(WeakPtr<ShellBrowserImpl> weakSelf,
		PidlAbsolute simplePidlUpdated, Runtime *runtime);
	static concurrencpp::null_result OnDirectoryPropertiesChanged(
		WeakPtr<ShellBrowserImpl> weakSelf, PidlAbsolute currentDirectory, Runtime *runtime);
	static concurrencpp::null_result RefreshDirectoryAfterUpdate(WeakPtr<ShellBrowserImpl> weakSelf,
		Runtime *runtime);
	static concurrencpp::null_result NavigateUpToClosestExistingItemIfNecessary(
		WeakPtr<ShellBrowserImpl> weakSelf, PidlAbsolute currentDirectory, Runtime *runtime);

	/* Filtering support. */
	void UpdateFiltering();
	void RemoveFilteredItems();
	void RemoveFilteredItem(int iItem, int iItemInternal);
	BOOL IsFilenameFiltered(const TCHAR *FileName) const;
	void UnfilterAllItems();
	void UnfilterItem(int internalIndex);
	void RestoreFilteredItem(int internalIndex);

	/* Listview group support. */
	static int CALLBACK GroupComparisonStub(int id1, int id2, void *data);
	int GroupComparison(int id1, int id2);
	int GroupNameComparison(const ListViewGroup &group1, const ListViewGroup &group2);
	int GroupRelativePositionComparison(const ListViewGroup &group1, const ListViewGroup &group2);
	const ListViewGroup GetListViewGroupById(int groupId);
	int DetermineItemGroup(int iItemInternal);
	std::optional<GroupInfo> DetermineItemNameGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemSizeGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemTotalSizeGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemTypeGroupVirtual(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemDateGroup(const BasicItemInfo_t &itemInfo,
		GroupByDateType dateType) const;
	std::optional<GroupInfo> DetermineItemSummaryGroup(const BasicItemInfo_t &itemInfo,
		const SHCOLUMNID *pscid, const GlobalFolderSettings &globalFolderSettings) const;
	std::optional<GroupInfo> DetermineItemFreeSpaceGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemAttributeGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemOwnerGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemVersionGroup(const BasicItemInfo_t &itemInfo,
		const TCHAR *szVersionType) const;
	std::optional<GroupInfo> DetermineItemCameraPropertyGroup(const BasicItemInfo_t &itemInfo,
		PROPID PropertyId) const;
	std::optional<GroupInfo> DetermineItemExtensionGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemFileSystemGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemNetworkStatus(const BasicItemInfo_t &itemInfo) const;

	/* Other grouping support. */
	int GetOrCreateListViewGroup(const GroupInfo &groupInfo);
	void MoveItemsIntoGroups();
	void InsertItemIntoGroup(int index, int groupId);
	void EnsureGroupExistsInListView(int groupId);
	void InsertGroupIntoListView(const ListViewGroup &listViewGroup);
	void RemoveGroupFromListView(const ListViewGroup &listViewGroup);
	void UpdateGroupHeader(const ListViewGroup &listViewGroup);
	std::wstring GenerateGroupHeader(const ListViewGroup &listViewGroup);
	void OnItemRemovedFromGroup(int groupId);
	void OnItemAddedToGroup(int groupId);
	std::optional<int> GetItemGroupId(int index);

	/* Listview icons. */
	void ProcessIconResult(int internalIndex, int iconIndex, int overlayIndex);

	/* Thumbnails view. */
	void QueueThumbnailTask(int internalIndex);
	std::optional<int> GetCachedThumbnailIndex(const ItemInfo_t &itemInfo);
	static wil::unique_hbitmap GetThumbnail(PCIDLIST_ABSOLUTE pidl, UINT thumbnailSize,
		WTS_FLAGS flags);
	void ProcessThumbnailResult(int thumbnailResultId);
	void SetupThumbnailsView(int shellImageListType);
	void RemoveThumbnailsView();
	void InvalidateAllItemImages();
	int GetIconThumbnail(int iInternalIndex) const;
	int GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const;
	int GetThumbnailInternal(int iType, int iInternalIndex, HBITMAP hThumbnailBitmap) const;
	void DrawIconThumbnailInternal(HDC hdcBacking, int iInternalIndex) const;
	void DrawThumbnailInternal(HDC hdcBacking, HBITMAP hThumbnailBitmap) const;

	/* Tiles view. */
	void InsertTileViewColumns();
	void SetTileViewInfo();
	void SetTileViewItemInfo(int iItem, int iItemInternal);

	void UpdateCurrentClipboardObject(wil::com_ptr_nothrow<IDataObject> clipboardDataObject);
	void OnClipboardUpdate();
	void RestoreStateOfCutItems();

	// ShellDropTargetWindow
	int GetDropTargetItem(const POINT &pt) override;
	unique_pidl_absolute GetPidlForTargetItem(int targetItem) override;
	IUnknown *GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl) override;
	bool IsTargetSourceOfDrop(int targetItem, IDataObject *dataObject) override;
	void UpdateUiForDrop(int targetItem, const POINT &pt) override;
	void ResetDropUiState() override;

	/* Drag and Drop support. */
	void RepositionLocalFiles(const POINT *ppt);
	void ScrollListViewForDrop(const POINT &pt);

	/* Miscellaneous. */
	BOOL CompareVirtualFolders(UINT uFolderCSIDL) const;
	int LocateFileItemInternalIndex(const TCHAR *szFileName) const;
	std::optional<int> GetItemIndexForPidl(PCIDLIST_ABSOLUTE pidl) const;
	std::optional<int> GetItemInternalIndexForPidl(PCIDLIST_ABSOLUTE pidl) const;
	std::optional<int> LocateItemByInternalIndex(int internalIndex) const;
	void ApplyHeaderSortArrow();

	HWND m_hListView;
	HWND m_hOwner;

	App *const m_app;

	std::shared_ptr<ShellEnumeratorImpl> m_shellEnumerator;
	NavigationManager m_navigationManager;
	std::unique_ptr<ShellNavigationController> m_navigationController;
	NavigationState m_navigationState = NavigationState::NoFolderShown;
	const HCURSOR m_progressCursor;

	TabNavigationInterface *m_tabNavigation;
	BrowserCommandTargetManager *const m_commandTargetManager;
	ScopedBrowserCommandTarget m_commandTarget;
	FileActionHandler *m_fileActionHandler;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	// When the listview is assigned a font, it will also set the font for the tooltip control.
	// However, that font will be reset whenever the theme for the tooltip control changes. Managing
	// the font for the tooltips control using MainFontSetter is a simple way of ensuring the font
	// is set consistently.
	MainFontSetter m_fontSetter;
	MainFontSetter m_tooltipFontSetter;

	DirectoryState m_directoryState;

	/* Stores various extra information on files, such
	as display name. */
	std::unordered_map<int, ItemInfo_t> m_itemInfoMap;

	ctpl::thread_pool m_columnThreadPool;
	std::unordered_map<int, std::future<ColumnResult_t>> m_columnResults;
	int m_columnResultIDCounter;

	std::unique_ptr<IconFetcher> m_iconFetcher;
	CachedIcons *m_cachedIcons;

	ctpl::thread_pool m_thumbnailThreadPool;
	std::unordered_map<int, std::future<std::optional<ThumbnailResult_t>>> m_thumbnailResults;
	int m_thumbnailResultIDCounter;

	ctpl::thread_pool m_infoTipsThreadPool;
	std::unordered_map<int, std::future<std::optional<InfoTipResult>>> m_infoTipResults;
	int m_infoTipResultIDCounter;

	/* Internal state. */
	const HINSTANCE m_resourceInstance;
	AcceleratorManager *const m_acceleratorManager;
	bool m_folderVisited = false;
	int m_iFolderIcon;
	int m_iFileIcon;

	/* Stores a unique index for each folder.
	This may be needed so that folders can be
	told apart when adding files from directory
	modification. */
	int m_uniqueFolderId;

	const Config *m_config;
	FolderSettings m_folderSettings;

	// Directory monitoring
	ShellChangeWatcher m_shellChangeWatcher;

	int m_middleButtonItem;

	// Shell window integration
	winrt::com_ptr<IShellWindows> m_shellWindows;
	bool m_shellWindowRegistered;
	unique_shell_window_cookie m_shellWindowCookie;

	/* Thumbnails. */
	int m_thumbnailItemWidth;
	int m_thumbnailItemHeight;

	/* Column related data. */
	std::vector<Column_t> *m_pActiveColumns;
	FolderColumns m_folderColumns;
	int m_nCurrentColumns;
	int m_nActiveColumns;
	bool m_PreviousSortColumnExists;
	ColumnType m_previousSortColumn;

	wil::com_ptr_nothrow<IDataObject> m_clipboardDataObject;
	std::vector<std::wstring> m_cutFileNames;

	/* Drag and drop related data. */
	UINT m_getDragImageMessage;
	winrt::com_ptr<ServiceProvider> m_dropServiceProvider;
	std::vector<PidlAbsolute> m_draggedItems;
	POINT m_ptDraggedOffset;
	bool m_performingDrag;
	IDataObject *m_draggedDataObject;

	WeakPtrFactory<ShellBrowserImpl> m_weakPtrFactory;
};
