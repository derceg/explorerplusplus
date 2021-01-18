// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ColumnDataRetrieval.h"
#include "Columns.h"
#include "FolderSettings.h"
#include "NavigatorInterface.h"
#include "SignalWrapper.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/DropHandler.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <thumbcache.h>
#include <future>
#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#define WM_USER_UPDATEWINDOWS (WM_APP + 17)
#define WM_USER_FILESADDED (WM_APP + 51)

struct BasicItemInfo_t;
class CachedIcons;
struct Config;
class FileActionHandler;
class IconFetcher;
class IconResourceLoader;
__interface IExplorerplusplus;
struct PreservedFolderState;
struct PreservedHistoryEntry;
class ShellNavigationController;
__interface TabNavigationInterface;
class WindowSubclassWrapper;

typedef struct
{
	ULARGE_INTEGER TotalFolderSize;
	ULARGE_INTEGER TotalSelectionSize;
} FolderInfo_t;

class ShellBrowser : public IDropTarget, public IDropFilesCallback, public NavigatorInterface
{
public:
	static ShellBrowser *CreateNew(int id, HWND hOwner, IExplorerplusplus *coreInterface,
		TabNavigationInterface *tabNavigation, FileActionHandler *fileActionHandler,
		const FolderSettings &folderSettings, std::optional<FolderColumns> initialColumns);

	static ShellBrowser *CreateFromPreserved(int id, HWND hOwner, IExplorerplusplus *coreInterface,
		TabNavigationInterface *tabNavigation, FileActionHandler *fileActionHandler,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState);

	/* IUnknown methods. */
	HRESULT __stdcall QueryInterface(REFIID iid, void **ppvObject) override;
	ULONG __stdcall AddRef() override;
	ULONG __stdcall Release() override;

	HWND GetListView() const;
	FolderSettings GetFolderSettings() const;

	ShellNavigationController *GetNavigationController() const;
	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

	/* Drag and Drop. */
	void DragStarted(int iFirstItem, POINT *ptCursor);
	void DragStopped();
	HRESULT _stdcall DragEnter(
		IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT _stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT _stdcall DragLeave() override;
	HRESULT _stdcall Drop(
		IDataObject *pDataObject, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) override;

	/* Get/Set current state. */
	unique_pidl_absolute GetDirectoryIdl() const;
	std::wstring GetDirectory() const;
	BOOL GetAutoArrange() const;
	void SetAutoArrange(BOOL autoArrange);
	ViewMode GetViewMode() const;
	void SetViewMode(ViewMode viewMode);
	void CycleViewMode(bool cycleForward);
	SortMode GetSortMode() const;
	void SetSortMode(SortMode sortMode);
	void SortFolder(SortMode sortMode);
	BOOL GetSortAscending() const;
	BOOL SetSortAscending(BOOL bAscending);
	BOOL GetShowHidden() const;
	BOOL SetShowHidden(BOOL bShowHidden);
	int GetNumItems() const;
	int GetNumSelectedFiles() const;
	int GetNumSelectedFolders() const;
	int GetNumSelected() const;

	/* ID. */
	int GetId() const;

	/* Directory modification support. */
	void FilesModified(DWORD Action, const TCHAR *FileName, int EventId, int iFolderIndex);
	void DirectoryAltered();
	void SetDirMonitorId(int iDirMonitorId);
	int GetDirMonitorId() const;
	int GetUniqueFolderId() const;

	/* Item information. */
	WIN32_FIND_DATA GetItemFileFindData(int index) const;
	unique_pidl_absolute GetItemCompleteIdl(int index) const;
	unique_pidl_child GetItemChildIdl(int index) const;
	std::wstring GetItemName(int index) const;
	std::wstring GetItemDisplayName(int index) const;
	std::wstring GetItemEditingName(int index) const;
	std::wstring GetItemFullName(int index) const;

	void ShowPropertiesForSelectedFiles() const;

	/* Column support. */
	std::vector<Column_t> GetCurrentColumns();
	void SetCurrentColumns(const std::vector<Column_t> &columns);
	static SortMode DetermineColumnSortMode(ColumnType columnType);
	static int LookupColumnNameStringIndex(ColumnType columnType);
	static int LookupColumnDescriptionStringIndex(ColumnType columnType);

	/* Filtering. */
	std::wstring GetFilter() const;
	void SetFilter(std::wstring_view filter);
	BOOL GetFilterStatus() const;
	void SetFilterStatus(BOOL bFilter);
	BOOL GetFilterCaseSensitive() const;
	void SetFilterCaseSensitive(BOOL filterCaseSensitive);

	bool TestListViewItemAttributes(int item, SFGAOF attributes) const;
	HRESULT GetListViewSelectionAttributes(SFGAOF *attributes) const;

	void SetFileAttributesForSelection();

	void SelectItems(const std::vector<PCIDLIST_ABSOLUTE> &pidls);
	void GetFolderInfo(FolderInfo_t *pFolderInfo);
	int LocateFileItemIndex(const TCHAR *szFileName) const;
	bool InVirtualFolder() const;
	BOOL CanCreate() const;
	HRESULT CopySelectedItemToClipboard(bool copy);
	void StartRenamingSelectedItems();
	void DeleteSelectedItems(bool permanent);

	BOOL GetShowInGroups() const;
	void SetShowInGroups(BOOL bShowInGroups);
	void SetShowInGroupsFlag(BOOL bShowInGroups);

	int CALLBACK SortTemporary(LPARAM lParam1, LPARAM lParam2);

	std::vector<SortMode> GetAvailableSortModes() const;
	size_t GetNumActiveColumns() const;
	void ImportAllColumns(const FolderColumns &folderColumns);
	FolderColumns ExportAllColumns();
	void QueueRename(PCIDLIST_ABSOLUTE pidlItem);
	void SelectItems(const std::list<std::wstring> &PastedFileList);
	void OnDeviceChange(WPARAM wParam, LPARAM lParam);

	void OnGridlinesSettingChanged();

	// Signals
	SignalWrapper<ShellBrowser, void()> directoryModified;
	SignalWrapper<ShellBrowser, void()> listViewSelectionChanged;
	SignalWrapper<ShellBrowser, void()> columnsChanged;

private:
	DISALLOW_COPY_AND_ASSIGN(ShellBrowser);

	struct ItemInfo_t
	{
		unique_pidl_absolute pidlComplete;
		unique_pidl_child pridl;
		WIN32_FIND_DATA wfd;
		bool isFindDataValid;
		std::wstring parsingName;
		std::wstring displayName;
		std::wstring editingName;
		int iIcon;

		/* These are only used for drives. They are
		needed for when a drive is removed from the
		system, in which case the drive name is needed
		so that the removed drive can be found. */
		BOOL bDrive;
		TCHAR szDrive[4];

		/* Used for temporary sorting in details mode (i.e.
		when items need to be rearranged). */
		int iRelativeSort;

		ItemInfo_t() : wfd({}), isFindDataValid(false), iIcon(0), bDrive(FALSE)
		{
		}
	};

	struct ShellChangeNotification
	{
		LONG event;
		unique_pidl_absolute pidl1;
		unique_pidl_absolute pidl2;

		ShellChangeNotification(LONG event, PCIDLIST_ABSOLUTE pidl1, PCIDLIST_ABSOLUTE pidl2) :
			event(event),
			pidl1(pidl1 ? ILCloneFull(pidl1) : nullptr),
			pidl2(pidl2 ? ILCloneFull(pidl2) : nullptr)
		{
		}
	};

	struct AlteredFile_t
	{
		TCHAR szFileName[MAX_PATH];
		DWORD dwAction;
		int iFolderIndex;
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

	struct DraggedFile_t
	{
		TCHAR szFileName[MAX_PATH];
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
		int id;
		std::wstring name;
		int relativeSortPosition;
		int numItems;

		ListViewGroup(int id, const GroupInfo &groupInfo) :
			id(id),
			name(groupInfo.name),
			relativeSortPosition(groupInfo.relativeSortPosition),
			numItems(0)
		{
		}
	};

	enum class GroupByDateType
	{
		Created,
		Modified,
		Accessed
	};

	struct DirectoryState
	{
		unique_pidl_absolute pidlDirectory;
		std::wstring directory;
		bool virtualFolder;
		int itemIDCounter;

		/* Stores information on files that have
		been created and are awaiting insertion
		into the listview. */
		std::vector<AwaitingAdd_t> awaitingAddList;

		std::unordered_set<int> filteredItemsList;

		int numItems;
		int numFilesSelected;
		int numFoldersSelected;
		ULARGE_INTEGER totalDirSize;
		ULARGE_INTEGER fileSelectionSize;

		/* Cached folder size data. */
		mutable std::unordered_map<int, ULONGLONG> cachedFolderSizes;

		std::vector<ShellChangeNotification> shellChangeNotifications;

		DirectoryState() :
			virtualFolder(false),
			itemIDCounter(0),
			numItems(0),
			numFilesSelected(0),
			numFoldersSelected(0),
			totalDirSize({}),
			fileSelectionSize({})
		{
		}
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

	static const UINT_PTR LISTVIEW_SUBCLASS_ID = 0;

	static const UINT WM_APP_COLUMN_RESULT_READY = WM_APP + 150;
	static const UINT WM_APP_THUMBNAIL_RESULT_READY = WM_APP + 151;
	static const UINT WM_APP_INFO_TIP_READY = WM_APP + 152;
	static const UINT WM_APP_SHELL_NOTIFY = WM_APP + 153;

	static const int THUMBNAIL_ITEM_WIDTH = 120;
	static const int THUMBNAIL_ITEM_HEIGHT = 120;

	static const UINT PROCESS_SHELL_CHANGES_TIMER_ID = 1;
	static const UINT PROCESS_SHELL_CHANGES_TIMEOUT = 100;

	ShellBrowser(int id, HWND hOwner, IExplorerplusplus *coreInterface,
		TabNavigationInterface *tabNavigation, FileActionHandler *fileActionHandler,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState);
	ShellBrowser(int id, HWND hOwner, IExplorerplusplus *coreInterface,
		TabNavigationInterface *tabNavigation, FileActionHandler *fileActionHandler,
		const FolderSettings &folderSettings, std::optional<FolderColumns> initialColumns);
	~ShellBrowser();

	HWND SetUpListView(HWND parent);
	int GenerateUniqueItemId();
	void MarkItemAsCut(int item, bool cut);
	void VerifySortMode();

	/* NavigatorInterface methods. */
	HRESULT BrowseFolder(const HistoryEntry &entry) override;
	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry = true) override;

	/* Browsing support. */
	HRESULT EnumerateFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry);
	void PrepareToChangeFolders();
	void ClearPendingResults();
	void ResetFolderState();
	void StoreCurrentlySelectedItems();
	void InsertAwaitingItems(BOOL bInsertIntoGroup);
	BOOL IsFileFiltered(const ItemInfo_t &itemInfo) const;
	std::optional<int> AddItemInternal(IShellFolder *shellFolder, PCIDLIST_ABSOLUTE pidlDirectory,
		PCITEMID_CHILD pidlChild, int itemIndex, BOOL setPosition);
	int AddItemInternal(int itemIndex, ItemInfo_t itemInfo, BOOL setPosition);
	std::optional<ItemInfo_t> GetItemInformation(
		IShellFolder *shellFolder, PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild);
	static HRESULT ExtractFindDataUsingPropertyStore(
		IShellFolder *shellFolder, PCITEMID_CHILD pidlChild, WIN32_FIND_DATA &output);
	void SetViewModeInternal(ViewMode viewMode);
	void SetFirstColumnTextToCallback();
	void SetFirstColumnTextToFilename();
	void ApplyFolderEmptyBackgroundImage(bool apply);

	static LRESULT CALLBACK ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ListViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK SortStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	/* Message handlers. */
	void ColumnClicked(int iClickedColumn);

	/* Listview. */
	void OnListViewMButtonDown(const POINT *pt);
	void OnListViewMButtonUp(const POINT *pt, UINT keysDown);
	void OnListViewGetDisplayInfo(LPARAM lParam);
	LRESULT OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip);
	void QueueInfoTipTask(int internalIndex, const std::wstring &existingInfoTip);
	static std::optional<InfoTipResult> GetInfoTipAsync(HWND listView, int infoTipResultId,
		int internalIndex, const BasicItemInfo_t &basicItemInfo, const Config &config,
		HINSTANCE instance, bool virtualFolder);
	void ProcessInfoTipResult(int infoTipResultId);
	void OnListViewItemInserted(const NMLISTVIEW *itemData);
	void OnListViewItemChanged(const NMLISTVIEW *changeData);
	void UpdateFileSelectionInfo(int internalIndex, BOOL selected);
	void OnListViewKeyDown(const NMLVKEYDOWN *lvKeyDown);
	std::vector<std::wstring> GetSelectedItems();

	HRESULT GetListViewItemAttributes(int item, SFGAOF *attributes) const;

	void StartRenamingSingleFile();
	void StartRenamingMultipleFiles();

	// Listview header context menu
	void OnListViewHeaderRightClick(const POINTS &cursorPos);
	void OnListViewHeaderMenuItemSelected(
		int menuItemId, const std::unordered_map<int, ColumnType> &menuItemMappings);
	void OnShowMoreColumnsSelected();
	void OnColumnMenuItemSelected(
		int menuItemId, const std::unordered_map<int, ColumnType> &menuItemMappings);

	const ItemInfo_t &GetItemByIndex(int index) const;
	ItemInfo_t &GetItemByIndex(int index);
	int GetItemInternalIndex(int item) const;

	BasicItemInfo_t getBasicItemInfo(int internalIndex) const;

	/* Sorting. */
	int CALLBACK Sort(int InternalIndex1, int InternalIndex2) const;

	/* Listview column support. */
	void SetUpListViewColumns();
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

	/* Device change support. */
	void UpdateDriveIcon(const TCHAR *szDrive);
	void RemoveDrive(const TCHAR *szDrive);

	/* Directory altered support. */
	void StartDirectoryMonitoring(PCIDLIST_ABSOLUTE pidl);
	void StopDirectoryMonitoring();
	void OnShellNotify(WPARAM wParam, LPARAM lParam);
	void OnProcessShellChangeNotifications();
	void ProcessShellChangeNotification(const ShellChangeNotification &change);
	void OnFileAdded(const TCHAR *szFileName);
	void AddItem(PCIDLIST_ABSOLUTE pidl);
	void RemoveItem(int iItemInternal);
	void OnItemRemoved(PCIDLIST_ABSOLUTE pidl);
	void OnFileRemoved(const TCHAR *szFileName);
	void OnFileModified(const TCHAR *fileName);
	void ModifyItem(PCIDLIST_ABSOLUTE pidl);
	void OnItemRenamed(PCIDLIST_ABSOLUTE pidlOld, PCIDLIST_ABSOLUTE pidlNew);
	void OnFileRenamedOldName(const TCHAR *szFileName);
	void OnFileRenamedNewName(const TCHAR *szFileName);
	void RenameItem(int internalIndex, const TCHAR *szNewFileName);
	void RenameItem(int internalIndex, PCIDLIST_ABSOLUTE pidlNew);
	void InvalidateAllColumnsForItem(int itemIndex);
	void InvalidateIconForItem(int itemIndex);
	int DetermineItemSortedPosition(LPARAM lParam) const;

	/* Filtering support. */
	void UpdateFiltering();
	void RemoveFilteredItems();
	void RemoveFilteredItem(int iItem, int iItemInternal);
	BOOL IsFilenameFiltered(const TCHAR *FileName) const;
	void UnfilterAllItems();
	void UnfilterItem(int internalIndex);
	void RestoreFilteredItem(int internalIndex);
	void ApplyFilteringBackgroundImage(bool apply);

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
	std::optional<GroupInfo> DetermineItemDateGroup(
		const BasicItemInfo_t &itemInfo, GroupByDateType dateType) const;
	std::optional<GroupInfo> DetermineItemSummaryGroup(const BasicItemInfo_t &itemInfo,
		const SHCOLUMNID *pscid, const GlobalFolderSettings &globalFolderSettings) const;
	std::optional<GroupInfo> DetermineItemFreeSpaceGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemAttributeGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemOwnerGroup(const BasicItemInfo_t &itemInfo) const;
	std::optional<GroupInfo> DetermineItemVersionGroup(
		const BasicItemInfo_t &itemInfo, const TCHAR *szVersionType) const;
	std::optional<GroupInfo> DetermineItemCameraPropertyGroup(
		const BasicItemInfo_t &itemInfo, PROPID PropertyId) const;
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
	void ProcessIconResult(int internalIndex, int iconIndex);
	std::optional<int> GetCachedIconIndex(const ItemInfo_t &itemInfo);

	/* Thumbnails view. */
	void QueueThumbnailTask(int internalIndex);
	std::optional<int> GetCachedThumbnailIndex(const ItemInfo_t &itemInfo);
	static wil::unique_hbitmap GetThumbnail(PIDLIST_ABSOLUTE pidl, WTS_FLAGS flags);
	void ProcessThumbnailResult(int thumbnailResultId);
	void SetupThumbnailsView();
	void RemoveThumbnailsView();
	int GetIconThumbnail(int iInternalIndex) const;
	int GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const;
	int GetThumbnailInternal(int iType, int iInternalIndex, HBITMAP hThumbnailBitmap) const;
	void DrawIconThumbnailInternal(HDC hdcBacking, int iInternalIndex) const;
	void DrawThumbnailInternal(HDC hdcBacking, HBITMAP hThumbnailBitmap) const;

	/* Tiles view. */
	void InsertTileViewColumns();
	void DeleteTileViewColumns();
	void SetTileViewInfo();
	void SetTileViewItemInfo(int iItem, int iItemInternal);

	void UpdateCurrentClipboardObject(wil::com_ptr_nothrow<IDataObject> clipboardDataObject);
	void OnClipboardUpdate();
	void RestoreStateOfCutItems();

	/* Drag and Drop support. */
	HRESULT InitializeDragDropHelpers();
	DWORD CheckItemLocations(IDataObject *pDataObject, int iDroppedItem);
	void HandleDragSelection(const POINT *ppt);
	void RepositionLocalFiles(const POINT *ppt);
	void ScrollListViewFromCursor(HWND hListView, const POINT *CursorPos);
	void PositionDroppedItems();
	void OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt) override;

	void OnApplicationShuttingDown();

	/* Miscellaneous. */
	BOOL CompareVirtualFolders(UINT uFolderCSIDL) const;
	int LocateFileItemInternalIndex(const TCHAR *szFileName) const;
	std::optional<int> GetItemInternalIndexForPidl(PCIDLIST_ABSOLUTE pidl) const;
	std::optional<int> LocateItemByInternalIndex(int internalIndex) const;
	void ApplyHeaderSortArrow();

	int m_iRefCount;

	HWND m_hListView;
	HWND m_hOwner;

	NavigationStartedSignal m_navigationStartedSignal;
	NavigationCommittedSignal m_navigationCommittedSignal;
	NavigationCompletedSignal m_navigationCompletedSignal;
	NavigationFailedSignal m_navigationFailedSignal;
	std::unique_ptr<ShellNavigationController> m_navigationController;

	TabNavigationInterface *m_tabNavigation;
	FileActionHandler *m_fileActionHandler;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	// Each instance of this class will subclass the parent window. As
	// they'll all use the same static procedure, it's important that
	// they use different subclass IDs (as the procedure and ID uniquely
	// identify a subclass).
	static int listViewParentSubclassIdCounter;

	BOOL m_bPerformingDrag;
	HIMAGELIST m_hListViewImageList;

	DirectoryState m_directoryState;

	/* Stores various extra information on files, such
	as display name. */
	std::unordered_map<int, ItemInfo_t> m_itemInfoMap;

	ctpl::thread_pool m_columnThreadPool;
	std::unordered_map<int, std::future<ColumnResult_t>> m_columnResults;
	int m_columnResultIDCounter;

	std::unique_ptr<IconFetcher> m_iconFetcher;
	CachedIcons *m_cachedIcons;

	IconResourceLoader *m_iconResourceLoader;

	ctpl::thread_pool m_thumbnailThreadPool;
	std::unordered_map<int, std::future<std::optional<ThumbnailResult_t>>> m_thumbnailResults;
	int m_thumbnailResultIDCounter;

	ctpl::thread_pool m_infoTipsThreadPool;
	std::unordered_map<int, std::future<std::optional<InfoTipResult>>> m_infoTipResults;
	int m_infoTipResultIDCounter;

	/* Internal state. */
	const HINSTANCE m_hResourceModule;
	BOOL m_bFolderVisited;
	int m_iDirMonitorId;
	int m_iFolderIcon;
	int m_iFileIcon;
	int m_iDropped;

	/* Stores a unique index for each folder.
	This may be needed so that folders can be
	told apart when adding files from directory
	modification. */
	int m_uniqueFolderId;

	const Config *m_config;
	FolderSettings m_folderSettings;

	/* ID. */
	const int m_ID;

	/* Directory monitoring. */
	ULONG m_shChangeNotifyId;

	wil::com_ptr_nothrow<IShellFolder> m_desktopFolder;
	unique_pidl_absolute m_recycleBinPidl;

	/* Stores information on files that
	have been modified (i.e. created, deleted,
	renamed, etc). */
	CRITICAL_SECTION m_csDirectoryAltered;
	std::list<AlteredFile_t> m_AlteredList;

	int m_middleButtonItem;

	/* Shell new. */
	unique_pidl_absolute m_queuedRenameItem;

	/* File selection. */
	std::list<std::wstring> m_FileSelectionList;

	/* Thumbnails. */
	BOOL m_bThumbnailsSetup;

	/* Column related data. */
	std::vector<Column_t> *m_pActiveColumns;
	FolderColumns m_folderColumns;
	bool m_listViewColumnsSetUp;
	int m_nCurrentColumns;
	int m_nActiveColumns;
	bool m_PreviousSortColumnExists;
	ColumnType m_previousSortColumn;

	wil::com_ptr_nothrow<IDataObject> m_clipboardDataObject;
	std::vector<std::wstring> m_cutFileNames;

	/* Drag and drop related data. */
	IDragSourceHelper *m_pDragSourceHelper;
	IDropTargetHelper *m_pDropTargetHelper;
	std::list<DroppedFile_t> m_droppedFileNameList;
	std::list<DraggedFile_t> m_DraggedFilesList;
	DragType m_DragType;
	POINT m_ptDraggedOffset;
	BOOL m_bDataAccept;
	BOOL m_bDragging;
	BOOL m_bDeselectDropFolder;
	BOOL m_bOnSameDrive;
	int m_bOverFolder;
	int m_iDropFolder;

	ListViewGroupSet m_listViewGroups;
	int m_groupIdCounter;
};