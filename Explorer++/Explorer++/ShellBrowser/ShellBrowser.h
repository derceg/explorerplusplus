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
#include "TabNavigationInterface.h"
#include "ViewModes.h"
#include "../Helper/DropHandler.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <wil/resource.h>
#include <future>
#include <list>
#include <optional>
#include <unordered_map>

#define WM_USER_UPDATEWINDOWS		(WM_APP + 17)
#define WM_USER_FILESADDED			(WM_APP + 51)
#define WM_USER_STARTEDBROWSING		(WM_APP + 55)
#define WM_USER_NEWITEMINSERTED		(WM_APP + 200)
#define WM_USER_DIRECTORYMODIFIED	(WM_APP + 204)

struct BasicItemInfo_t;
class CachedIcons;
struct Config;
class IconFetcher;
class IconResourceLoader;
struct PreservedFolderState;
struct PreservedHistoryEntry;
class ShellNavigationController;

typedef struct
{
	ULARGE_INTEGER TotalFolderSize;
	ULARGE_INTEGER TotalSelectionSize;
} FolderInfo_t;

typedef struct
{
	std::wstring header;
	int iGroupId;

	/* Used to record the number of items in this group.
	Mimics the feature available in Windows Vista and later. */
	int nItems;
} TypeGroup_t;

class ShellBrowser : public IDropTarget, public IDropFilesCallback, public NavigatorInterface
{
public:

	static ShellBrowser *CreateNew(int id, HINSTANCE resourceInstance, HWND hOwner,
		CachedIcons *cachedIcons, IconResourceLoader *iconResourceLoader, const Config *config,
		TabNavigationInterface *tabNavigation, const FolderSettings &folderSettings,
		std::optional<FolderColumns> initialColumns);

	static ShellBrowser *CreateFromPreserved(int id, HINSTANCE resourceInstance, HWND hOwner,
		CachedIcons *cachedIcons, IconResourceLoader *iconResourceLoader, const Config *config,
		TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState);

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject) override;
	ULONG __stdcall		AddRef() override;
	ULONG __stdcall		Release() override;

	HWND				GetListView() const;
	FolderSettings		GetFolderSettings() const;

	ShellNavigationController	*GetNavigationController() const;
	boost::signals2::connection	AddNavigationCompletedObserver(const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

	/* Drag and Drop. */
	void				DragStarted(int iFirstItem,POINT *ptCursor);
	void				DragStopped();
	HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect) override;
	HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect) override;
	HRESULT _stdcall	DragLeave() override;
	HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect) override;

	/* Get/Set current state. */
	unique_pidl_absolute	GetDirectoryIdl() const;
	std::wstring		GetDirectory() const;
	BOOL				GetAutoArrange() const;
	void				SetAutoArrange(BOOL autoArrange);
	ViewMode			GetViewMode() const;
	void				SetViewMode(ViewMode viewMode);
	void				CycleViewMode(bool cycleForward);
	SortMode			GetSortMode() const;
	void				SetSortMode(SortMode sortMode);
	void				SortFolder(SortMode sortMode);
	BOOL				GetSortAscending() const;
	BOOL				SetSortAscending(BOOL bAscending);
	BOOL				GetShowHidden() const;
	BOOL				SetShowHidden(BOOL bShowHidden);
	int					GetNumItems() const;
	int					GetNumSelectedFiles() const;
	int					GetNumSelectedFolders() const;
	int					GetNumSelected() const;

	/* ID. */
	int					GetId() const;

	/* Directory modification support. */
	void				FilesModified(DWORD Action, const TCHAR *FileName, int EventId, int iFolderIndex);
	void				DirectoryAltered();
	void				SetDirMonitorId(int iDirMonitorId);
	int					GetDirMonitorId() const;
	int					GetUniqueFolderId() const;

	/* Item information. */
	WIN32_FIND_DATA		GetItemFileFindData(int iItem) const;
	unique_pidl_absolute	GetItemCompleteIdl(int iItem) const;
	unique_pidl_child	GetItemChildIdl(int iItem) const;
	int					GetItemDisplayName(int iItem,UINT BufferSize,TCHAR *Buffer) const;
	HRESULT				GetItemFullName(int iIndex,TCHAR *FullItemPath,UINT cchMax) const;

	void				ShowPropertiesForSelectedFiles() const;
	
	/* Column support. */
	std::vector<Column_t>	ExportCurrentColumns();
	void				ImportColumns(const std::vector<Column_t> &columns);
	static SortMode		DetermineColumnSortMode(int iColumnId);
	static int			LookupColumnNameStringIndex(int iColumnId);
	static int			LookupColumnDescriptionStringIndex(int iColumnId);

	/* Filtering. */
	std::wstring		GetFilter() const;
	void				SetFilter(std::wstring_view filter);
	BOOL				GetFilterStatus() const;
	void				SetFilterStatus(BOOL bFilter);
	BOOL				GetFilterCaseSensitive() const;
	void				SetFilterCaseSensitive(BOOL filterCaseSensitive);

	void				SetFileAttributesForSelection();

	int					SelectFiles(const TCHAR *FileNamePattern);
	void				GetFolderInfo(FolderInfo_t *pFolderInfo);
	int					LocateFileItemIndex(const TCHAR *szFileName) const;
	BOOL				DeghostItem(int iItem);
	BOOL				GhostItem(int iItem);
	BOOL				InVirtualFolder() const;
	BOOL				CanCreate() const;

	BOOL				GetShowInGroups() const;
	void				SetShowInGroups(BOOL bShowInGroups);
	void				SetShowInGroupsFlag(BOOL bShowInGroups);

	int CALLBACK		SortTemporary(LPARAM lParam1,LPARAM lParam2);

	std::vector<SortMode>	GetAvailableSortModes() const;
	size_t				GetNumActiveColumns() const;
	void				ImportAllColumns(const FolderColumns &folderColumns);
	FolderColumns		ExportAllColumns();
	void				QueueRename(PCIDLIST_ABSOLUTE pidlItem);
	void				SelectItems(const std::list<std::wstring> &PastedFileList);
	void				OnDeviceChange(WPARAM wParam,LPARAM lParam);

	void				OnGridlinesSettingChanged();

	// Signals
	SignalWrapper<ShellBrowser, void()> listViewSelectionChanged;
	SignalWrapper<ShellBrowser, void()> columnsChanged;

private:

	DISALLOW_COPY_AND_ASSIGN(ShellBrowser);

	struct DirectoryState
	{
		unique_pidl_absolute pidlDirectory;
		int itemIDCounter;

		DirectoryState() :
			itemIDCounter(0)
		{

		}
	};

	struct ItemInfo_t
	{
		unique_pidl_absolute	pidlComplete;
		unique_pidl_child	pridl;
		WIN32_FIND_DATA	wfd;
		TCHAR			szDisplayName[MAX_PATH];
		int				iIcon;

		/* These are only used for drives. They are
		needed for when a drive is removed from the
		system, in which case the drive name is needed
		so that the removed drive can be found. */
		BOOL			bDrive;
		TCHAR			szDrive[4];

		/* Used for temporary sorting in details mode (i.e.
		when items need to be rearranged). */
		int				iRelativeSort;
	};

	struct AlteredFile_t
	{
		TCHAR	szFileName[MAX_PATH];
		DWORD	dwAction;
		int		iFolderIndex;
	};

	struct AwaitingAdd_t
	{
		int		iItem;
		int		iItemInternal;

		BOOL	bPosition;
		int		iAfter;
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
		int columnID;
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

	enum class GroupByDateType
	{
		Created,
		Modified,
		Accessed
	};

	static const int THUMBNAIL_ITEM_HORIZONTAL_SPACING = 20;
	static const int THUMBNAIL_ITEM_VERTICAL_SPACING = 20;

	static const UINT_PTR LISTVIEW_SUBCLASS_ID = 0;

	static const UINT WM_APP_COLUMN_RESULT_READY = WM_APP + 150;
	static const UINT WM_APP_THUMBNAIL_RESULT_READY = WM_APP + 151;
	static const UINT WM_APP_INFO_TIP_READY = WM_APP + 152;

	static const int THUMBNAIL_ITEM_WIDTH = 120;
	static const int THUMBNAIL_ITEM_HEIGHT = 120;

	ShellBrowser(int id, HINSTANCE resourceInstance, HWND hOwner, CachedIcons *cachedIcons,
		IconResourceLoader *iconResourceLoader, const Config *config,
		TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState);
	ShellBrowser(int id, HINSTANCE resourceInstance, HWND hOwner, CachedIcons *cachedIcons,
		IconResourceLoader *iconResourceLoader, const Config *config,
		TabNavigationInterface *tabNavigation, const FolderSettings &folderSettings,
		std::optional<FolderColumns> initialColumns);
	~ShellBrowser();

	HWND				SetUpListView(HWND parent);
	int					GenerateUniqueItemId();
	BOOL				GhostItemInternal(int iItem,BOOL bGhost);
	void				DetermineFolderVirtual(PCIDLIST_ABSOLUTE pidlDirectory);
	void				VerifySortMode();

	/* NavigatorInterface methods. */
	HRESULT				BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry = true) override;

	/* Browsing support. */
	HRESULT				EnumerateFolder(PCIDLIST_ABSOLUTE pidlDirectory);
	void				ClearPendingResults();
	void				ResetFolderState();
	void				InsertAwaitingItems(BOOL bInsertIntoGroup);
	BOOL				IsFileFiltered(const ItemInfo_t &itemInfo) const;
	HRESULT				AddItemInternal(PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild, const TCHAR *szFileName, int iItemIndex, BOOL bPosition);
	HRESULT				AddItemInternal(int iItemIndex,int iItemId,BOOL bPosition);
	int					SetItemInformation(PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild, const TCHAR *szFileName);
	void				SetViewModeInternal(ViewMode viewMode);
	void				ApplyFolderEmptyBackgroundImage(bool apply);
	void				ApplyFilteringBackgroundImage(bool apply);
	void				PlayNavigationSound() const;

	static LRESULT CALLBACK	ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK	ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK	ListViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK	ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK	SortStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	/* Message handlers. */
	void				ColumnClicked(int iClickedColumn);

	/* Listview. */
	void				OnListViewMButtonDown(const POINT *pt);
	void				OnListViewMButtonUp(const POINT *pt);
	void				OnListViewGetDisplayInfo(LPARAM lParam);
	LRESULT				OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip);
	void				QueueInfoTipTask(int internalIndex, const std::wstring &existingInfoTip);
	static std::optional<InfoTipResult>	GetInfoTipAsync(HWND listView, int infoTipResultId, int internalIndex, const BasicItemInfo_t &basicItemInfo, const Config &config, HINSTANCE instance, bool virtualFolder);
	void				ProcessInfoTipResult(int infoTipResultId);
	void				OnListViewItemChanged(const NMLISTVIEW *changeData);
	void				UpdateFileSelectionInfo(int internalIndex, BOOL selected);
	void				OnListViewKeyDown(const NMLVKEYDOWN *lvKeyDown);

	// Listview header context menu
	void				OnListViewHeaderRightClick(const POINTS &cursorPos);
	void				OnListViewHeaderMenuItemSelected(int menuItemId, const std::unordered_map<int, UINT> &menuItemMappings);
	void				OnShowMoreColumnsSelected();
	void				OnColumnMenuItemSelected(int menuItemId, const std::unordered_map<int, UINT> &menuItemMappings);

	ItemInfo_t			&GetItemByIndex(int index);
	int					GetItemInternalIndex(int item) const;

	BasicItemInfo_t		getBasicItemInfo(int internalIndex) const;

	/* Sorting. */
	int CALLBACK		Sort(int InternalIndex1,int InternalIndex2) const;

	/* Listview column support. */
	void				PlaceColumns();
	void				QueueColumnTask(int itemInternalIndex, int columnIndex);
	static ColumnResult_t	GetColumnTextAsync(HWND listView, int columnResultId, unsigned int columnID, int internalIndex, const BasicItemInfo_t &basicItemInfo, const GlobalFolderSettings &globalFolderSettings);
	void				InsertColumn(unsigned int columnId,int iColumnIndex,int iWidth);
	void				SetActiveColumnSet();
	void				GetColumnInternal(unsigned int id,Column_t *pci) const;
	void				SaveColumnWidths();
	void				ProcessColumnResult(int columnResultId);
	std::optional<int>	GetColumnIndexById(unsigned int id) const;
	std::optional<unsigned int>	GetColumnIdByIndex(int index) const;

	/* Device change support. */
	void				UpdateDriveIcon(const TCHAR *szDrive);
	void				RemoveDrive(const TCHAR *szDrive);
	
	/* Directory altered support. */
	void				OnFileActionAdded(const TCHAR *szFileName);
	void				RemoveItem(int iItemInternal);
	void				RemoveItemInternal(const TCHAR *szFileName);
	void				ModifyItemInternal(const TCHAR *FileName);
	void				OnFileActionRenamedOldName(const TCHAR *szFileName);
	void				OnFileActionRenamedNewName(const TCHAR *szFileName);
	void				RenameItem(int iItemInternal, const TCHAR *szNewFileName);
	int					DetermineItemSortedPosition(LPARAM lParam) const;

	/* Filtering support. */
	BOOL				IsFilenameFiltered(const TCHAR *FileName) const;
	void				RemoveFilteredItems();
	void				RemoveFilteredItem(int iItem,int iItemInternal);
	void				UpdateFiltering();
	void				UnfilterAllItems();

	/* Listview group support (real files). */
	static INT CALLBACK	GroupNameComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData);
	INT CALLBACK		GroupNameComparison(INT Group1_ID, INT Group2_ID);
	static INT CALLBACK	GroupFreeSpaceComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData);
	INT CALLBACK		GroupFreeSpaceComparison(INT Group1_ID, INT Group2_ID);
	std::wstring		RetrieveGroupHeader(int groupId);
	int					DetermineItemGroup(int iItemInternal);
	std::wstring		DetermineItemNameGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemSizeGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemTotalSizeGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemTypeGroupVirtual(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemDateGroup(const BasicItemInfo_t &itemInfo, GroupByDateType dateType) const;
	std::wstring		DetermineItemSummaryGroup(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid,
		const GlobalFolderSettings &globalFolderSettings) const;
	std::wstring		DetermineItemFreeSpaceGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemAttributeGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemOwnerGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemVersionGroup(const BasicItemInfo_t &itemInfo, const TCHAR *szVersionType) const;
	std::wstring		DetermineItemCameraPropertyGroup(const BasicItemInfo_t &itemInfo, PROPID PropertyId) const;
	std::wstring		DetermineItemExtensionGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemFileSystemGroup(const BasicItemInfo_t &itemInfo) const;
	std::wstring		DetermineItemNetworkStatus(const BasicItemInfo_t &itemInfo) const;

	/* Other grouping support. */
	int					CheckGroup(std::wstring_view groupHeader, PFNLVGROUPCOMPARE groupComparison);
	void				InsertItemIntoGroup(int iItem,int iGroupId);
	void				MoveItemsIntoGroups();

	/* Listview icons. */
	void				ProcessIconResult(int internalIndex, int iconIndex);
	std::optional<int>	GetCachedIconIndex(const ItemInfo_t &itemInfo);

	/* Thumbnails view. */
	void				QueueThumbnailTask(int internalIndex);
	static std::optional<ThumbnailResult_t>	FindThumbnailAsync(HWND listView, int thumbnailResultId, int internalIndex, const BasicItemInfo_t &basicItemInfo);
	void				ProcessThumbnailResult(int thumbnailResultId);
	void				SetupThumbnailsView();
	void				RemoveThumbnailsView();
	int					GetIconThumbnail(int iInternalIndex) const;
	int					GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const;
	int					GetThumbnailInternal(int iType, int iInternalIndex, HBITMAP hThumbnailBitmap) const;
	void				DrawIconThumbnailInternal(HDC hdcBacking, int iInternalIndex) const;
	void				DrawThumbnailInternal(HDC hdcBacking, HBITMAP hThumbnailBitmap) const;

	/* Tiles view. */
	void				InsertTileViewColumns();
	void				DeleteTileViewColumns();
	void				SetTileViewInfo();
	void				SetTileViewItemInfo(int iItem,int iItemInternal);

	/* Drag and Drop support. */
	HRESULT				InitializeDragDropHelpers();
	DWORD				CheckItemLocations(IDataObject *pDataObject,int iDroppedItem);
	void				HandleDragSelection(const POINT *ppt);
	void				RepositionLocalFiles(const POINT *ppt);
	void				ScrollListViewFromCursor(HWND hListView, const POINT *CursorPos);
	void				PositionDroppedItems();
	void				OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt) override;

	/* Miscellaneous. */
	BOOL				CompareVirtualFolders(UINT uFolderCSIDL) const;
	int					LocateFileItemInternalIndex(const TCHAR *szFileName) const;
	std::optional<int>	LocateItemByInternalIndex(int internalIndex) const;
	void				ApplyHeaderSortArrow();
	void				QueryFullItemNameInternal(int iItemInternal,TCHAR *szFullFileName,UINT cchMax) const;


	int					m_iRefCount;

	HWND				m_hListView;
	HWND				m_hOwner;

	NavigationCompletedSignal	m_navigationCompletedSignal;
	std::unique_ptr<ShellNavigationController>	m_navigationController;

	TabNavigationInterface	*m_tabNavigation;

	std::vector<WindowSubclassWrapper>	m_windowSubclasses;

	// Each instance of this class will subclass the parent window. As
	// they'll all use the same static procedure, it's important that
	// they use different subclass IDs (as the procedure and ID uniquely
	// identify a subclass).
	static int listViewParentSubclassIdCounter;

	BOOL				m_bPerformingDrag;
	HIMAGELIST			m_hListViewImageList;

	DirectoryState		m_directoryState;

	/* Stores various extra information on files, such
	as display name. */
	std::unordered_map<int, ItemInfo_t>	m_itemInfoMap;

	ctpl::thread_pool	m_columnThreadPool;
	std::unordered_map<int, std::future<ColumnResult_t>> m_columnResults;
	int					m_columnResultIDCounter;

	std::unique_ptr<IconFetcher> m_iconFetcher;
	CachedIcons			*m_cachedIcons;

	IconResourceLoader	*m_iconResourceLoader;

	ctpl::thread_pool	m_thumbnailThreadPool;
	std::unordered_map<int, std::future<std::optional<ThumbnailResult_t>>> m_thumbnailResults;
	int					m_thumbnailResultIDCounter;

	ctpl::thread_pool	m_infoTipsThreadPool;
	std::unordered_map<int, std::future<std::optional<InfoTipResult>>> m_infoTipResults;
	int					m_infoTipResultIDCounter;

	/* Cached folder size data. */
	mutable std::unordered_map<int, ULONGLONG>	m_cachedFolderSizes;

	/* Internal state. */
	const HINSTANCE		m_hResourceModule;
	TCHAR				m_CurDir[MAX_PATH];
	ULARGE_INTEGER		m_ulTotalDirSize;
	ULARGE_INTEGER		m_ulFileSelectionSize;
	BOOL				m_bVirtualFolder;
	BOOL				m_bFolderVisited;
	int					m_nTotalItems;
	int					m_NumFilesSelected;
	int					m_NumFoldersSelected;
	int					m_iDirMonitorId;
	int					m_iFolderIcon;
	int					m_iFileIcon;
	int					m_iDropped;

	/* Stores a unique index for each folder.
	This may be needed so that folders can be
	told apart when adding files from directory
	modification. */
	int					m_uniqueFolderId;

	const Config		*m_config;
	FolderSettings		m_folderSettings;

	/* ID. */
	const int			m_ID;

	/* Stores information on files that
	have been modified (i.e. created, deleted,
	renamed, etc). */
	CRITICAL_SECTION	m_csDirectoryAltered;
	std::list<AlteredFile_t>	m_AlteredList;
	std::list<Added_t>	m_FilesAdded;

	/* Stores information on files that have
	been created and are awaiting insertion
	into the listview. */
	std::list<AwaitingAdd_t>	m_AwaitingAddList;

	int					m_middleButtonItem;

	/* Shell new. */
	BOOL				m_bNewItemCreated;
	PCIDLIST_ABSOLUTE	m_pidlNewItem;
	int					m_iIndexNewItem;

	/* File selection. */
	std::list<std::wstring>	m_FileSelectionList;

	/* Thumbnails. */
	BOOL				m_bThumbnailsSetup;

	/* Column related data. */
	std::vector<Column_t>	*m_pActiveColumns;
	FolderColumns		m_folderColumns;
	BOOL				m_bColumnsPlaced;
	int					m_nCurrentColumns;
	int					m_nActiveColumns;
	bool				m_PreviousSortColumnExists;
	unsigned int		m_iPreviousSortedColumnId;

	/* Drag and drop related data. */
	IDragSourceHelper *	m_pDragSourceHelper;
	IDropTargetHelper *	m_pDropTargetHelper;
	std::list<DroppedFile_t>	m_DroppedFileNameList;
	std::list<DraggedFile_t>	m_DraggedFilesList;
	DragType			m_DragType;
	POINT				m_ptDraggedOffset;
	BOOL				m_bDataAccept;
	BOOL				m_bDragging;
	BOOL				m_bDeselectDropFolder;
	BOOL				m_bOnSameDrive;
	int					m_bOverFolder;
	int					m_iDropFolder;

	/* Listview groups. The group id is declared
	explicitly, rather than taken from the size
	of the group list, to avoid warnings concerning
	size_t and int. */
	std::list<TypeGroup_t>	m_GroupList;
	int					m_iGroupId;

	/* Filtering related data. */
	std::list<int>		m_FilteredItemsList;
};