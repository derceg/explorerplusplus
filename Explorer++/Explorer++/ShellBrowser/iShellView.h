// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ColumnDataRetrieval.h"
#include "FolderSettings.h"
#include "iPathManager.h"
#include "ItemData.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/DropHandler.h"
#include "../Helper/Helper.h"
#include "../Helper/ImageWrappers.h"
#include "../Helper/Macros.h"
#include "../Helper/StringHelper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/optional.hpp>
#include <future>
#include <list>
#include <mutex>
#include <unordered_map>

#define WM_USER_UPDATEWINDOWS		(WM_APP + 17)
#define WM_USER_FILESADDED			(WM_APP + 51)
#define WM_USER_STARTEDBROWSING		(WM_APP + 55)
#define WM_USER_NEWITEMINSERTED		(WM_APP + 200)
#define WM_USER_DIRECTORYMODIFIED	(WM_APP + 204)

typedef struct
{
	unsigned int id;
	BOOL bChecked;
	int iWidth;
} Column_t;

typedef struct
{
	unsigned int id;
	BOOL bChecked;
} ColumnOld_t;

typedef struct
{
	std::list<Column_t>	RealFolderColumnList;
	std::list<Column_t>	MyComputerColumnList;
	std::list<Column_t>	ControlPanelColumnList;
	std::list<Column_t>	RecycleBinColumnList;
	std::list<Column_t>	PrintersColumnList;
	std::list<Column_t>	NetworkConnectionsColumnList;
	std::list<Column_t>	MyNetworkPlacesColumnList;
} ColumnExport_t;

struct InitialColumns
{
	std::list<Column_t> *pRealFolderColumnList;
	std::list<Column_t> *pMyComputerColumnList;
	std::list<Column_t> *pControlPanelColumnList;
	std::list<Column_t> *pRecycleBinColumnList;
	std::list<Column_t> *pPrintersColumnList;
	std::list<Column_t> *pNetworkConnectionsColumnList;
	std::list<Column_t> *pMyNetworkPlacesColumnList;
};

typedef struct
{
	ULARGE_INTEGER TotalFolderSize;
	ULARGE_INTEGER TotalSelectionSize;
} FolderInfo_t;

typedef struct
{
	TCHAR szHeader[512];
	int iGroupId;

	/* Used to record the number of items in this group.
	Mimics the feature available in Windows Vista and later. */
	int nItems;
} TypeGroup_t;

class CShellBrowser : public IDropTarget, public IDropFilesCallback
{
	friend int CALLBACK SortStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

public:

	static CShellBrowser	*CreateNew(HWND hOwner, HWND hListView, const GlobalFolderSettings *globalFolderSettings,
		const FolderSettings &folderSettings, const InitialColumns &initialColumns);

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	/* Navigation. */
	HRESULT				BrowseFolder(const TCHAR *szPath,UINT wFlags);
	HRESULT				BrowseFolder(LPCITEMIDLIST pidlDirectory,UINT wFlags);
	HRESULT				Refresh(void);

	/* Drag and Drop. */
	void				DragStarted(int iFirstItem,POINT *ptCursor);
	void				DragStopped(void);
	HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragLeave(void);
	HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect);

	/* Drag and Drop support. */
	BOOL				QueryDragging(void) const;

	/* Get/Set current state. */
	LPITEMIDLIST		QueryCurrentDirectoryIdl(void) const;
	UINT				QueryCurrentDirectory(int BufferSize,TCHAR *Buffer) const;
	BOOL				GetAutoArrange(void) const;
	void				SetAutoArrange(BOOL autoArrange);
	ViewMode			GetViewMode() const;
	void				SetViewMode(ViewMode viewMode);
	SortMode			GetSortMode() const;
	void				SetSortMode(SortMode sortMode);
	void				SortFolder(SortMode sortMode);
	BOOL				GetSortAscending(void) const;
	BOOL				SetSortAscending(BOOL bAscending);
	BOOL				ToggleSortAscending(void);
	BOOL				GetShowHidden(void) const;
	BOOL				SetShowHidden(BOOL bShowHidden);
	BOOL				ToggleShowHidden(void);
	BOOL				CanBrowseBack(void) const;
	BOOL				CanBrowseForward(void) const;
	std::list<LPITEMIDLIST>	GetBackHistory() const;
	std::list<LPITEMIDLIST>	GetForwardHistory() const;
	LPITEMIDLIST		RetrieveHistoryItemWithoutUpdate(int iItem);
	LPITEMIDLIST		RetrieveHistoryItem(int iItem);
	BOOL				CanBrowseUp(void) const;
	int					GetNumItems(void) const;
	int					GetNumSelectedFiles(void) const;
	int					GetNumSelectedFolders(void) const;
	int					GetNumSelected(void) const;

	/* ID. */
	int					GetId(void) const;
	void				SetId(int ID);

	/* Directory modification support. */
	void				FilesModified(DWORD Action, const TCHAR *FileName, int EventId, int iFolderIndex);
	void				DirectoryAltered(void);
	void				SetDirMonitorId(int iDirMonitorId);
	int					GetDirMonitorId(void) const;
	int					GetFolderIndex(void) const;

	/* Item information. */
	WIN32_FIND_DATA		QueryFileFindData(int iItem) const;
	LPITEMIDLIST		QueryItemCompleteIdl(int iItem) const;
	LPITEMIDLIST		QueryItemRelativeIdl(int iItem) const;
	DWORD				QueryFileAttributes(int iItem) const;
	int					QueryDisplayName(int iItem,UINT BufferSize,TCHAR *Buffer) const;
	HRESULT				QueryFullItemName(int iIndex,TCHAR *FullItemPath,UINT cchMax) const;
	
	/* Column support. */
	void				ExportCurrentColumns(std::list<Column_t> *pColumns);
	void				ImportColumns(std::list<Column_t> *pColumns);
	static SortMode		DetermineColumnSortMode(int iColumnId);
	static int			LookupColumnNameStringIndex(int iColumnId);
	static int			LookupColumnDescriptionStringIndex(int iColumnId);

	/* Filtering. */
	void				GetFilter(TCHAR *szFilter,int cchMax) const;
	void				SetFilter(const TCHAR *szFilter);
	BOOL				GetFilterStatus(void) const;
	void				SetFilterStatus(BOOL bFilter);
	BOOL				GetFilterCaseSensitive(void) const;
	void				SetFilterCaseSensitive(BOOL bCaseSensitive);

	void				UpdateFileSelectionInfo(int,BOOL);
	HRESULT				CreateHistoryPopup(IN HWND hParent,OUT LPITEMIDLIST *pidl,IN POINT *pt,IN BOOL bBackOrForward);
	int					SelectFiles(const TCHAR *FileNamePattern);
	void				GetFolderInfo(FolderInfo_t *pFolderInfo);
	int					LocateFileItemIndex(const TCHAR *szFileName) const;
	BOOL				DeghostItem(int iItem);
	BOOL				GhostItem(int iItem);
	BOOL				InVirtualFolder(void) const;
	BOOL				CanCreate(void) const;

	BOOL				GetShowInGroups(void) const;
	void				SetShowInGroups(BOOL bShowInGroups);
	void				SetShowInGroupsFlag(BOOL bShowInGroups);

	void				SetInsertSorted(BOOL bInsertSorted);

	int CALLBACK		SortTemporary(LPARAM lParam1,LPARAM lParam2);

	std::list<int>		GetAvailableSortModes() const;
	size_t				QueryNumActiveColumns(void) const;
	void				SetResourceModule(HINSTANCE hResourceModule);
	void				ImportAllColumns(const ColumnExport_t *pce);
	void				ExportAllColumns(ColumnExport_t *pce);
	void				QueueRename(LPCITEMIDLIST pidlItem);
	void				SelectItems(const std::list<std::wstring> &PastedFileList);
	void				OnDeviceChange(WPARAM wParam,LPARAM lParam);

	void				OnGridlinesSettingChanged();

private:

	DISALLOW_COPY_AND_ASSIGN(CShellBrowser);

	struct ItemInfo_t
	{
		PIDLPointer		pidlComplete;
		PIDLPointer		pridl;
		WIN32_FIND_DATA	wfd;
		TCHAR			szDisplayName[MAX_PATH];
		BOOL			bIconRetrieved;
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

	enum DateType_t
	{
		DATE_TYPE_CREATED,
		DATE_TYPE_MODIFIED,
		DATE_TYPE_ACCESSED
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

	struct IconResult_t
	{
		int itemInternalIndex;
		int iconIndex;
	};

	struct ThumbnailResult_t
	{
		int itemInternalIndex;
		HBitmapPtr bitmap;
	};

	static const int THUMBNAIL_ITEM_HORIZONTAL_SPACING = 20;
	static const int THUMBNAIL_ITEM_VERTICAL_SPACING = 20;

	static const UINT_PTR LISTVIEW_SUBCLASS_ID = 0;

	static const UINT WM_APP_COLUMN_RESULT_READY = WM_APP + 150;
	static const UINT WM_APP_THUMBNAIL_RESULT_READY = WM_APP + 151;
	static const UINT WM_APP_ICON_RESULT_READY = WM_APP + 152;

	static const int THUMBNAIL_ITEM_WIDTH = 120;
	static const int THUMBNAIL_ITEM_HEIGHT = 120;

	CShellBrowser(HWND hOwner, HWND hListView, const GlobalFolderSettings *globalFolderSettings,
		const FolderSettings &folderSettings, const InitialColumns &initialColumns);
	~CShellBrowser();

	int					GenerateUniqueItemId(void);
	BOOL				GhostItemInternal(int iItem,BOOL bGhost);
	void				DetermineFolderVirtual(LPITEMIDLIST pidlDirectory);
	void				VerifySortMode(void);

	/* Browsing support. */
	void				BrowseVirtualFolder(LPITEMIDLIST pidlDirectory);
	HRESULT				ParsePath(LPITEMIDLIST *pidlDirectory,UINT uFlags,BOOL *bWriteHistory);
	void inline			InsertAwaitingItems(BOOL bInsertIntoGroup);
	BOOL				IsFileFiltered(int iItemInternal) const;
	HRESULT inline		AddItemInternal(LPITEMIDLIST pidlDirectory, LPITEMIDLIST pidlRelative, const TCHAR *szFileName, int iItemIndex, BOOL bPosition);
	HRESULT inline		AddItemInternal(int iItemIndex,int iItemId,BOOL bPosition);
	int inline			SetItemInformation(LPITEMIDLIST pidlDirectory, LPITEMIDLIST pidlRelative, const TCHAR *szFileName);
	void				ResetFolderMemoryAllocations(void);
	void				SetViewModeInternal(ViewMode viewMode);
	void				ApplyFolderEmptyBackgroundImage(bool apply);
	void				ApplyFilteringBackgroundImage(bool apply);

	static LRESULT CALLBACK	ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK	ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK	ListViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK	ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/* Message handlers. */
	void				OnListViewGetDisplayInfo(LPARAM lParam);
	void				ColumnClicked(int iClickedColumn);

	BasicItemInfo_t		getBasicItemInfo(int internalIndex) const;

	/* Sorting. */
	int CALLBACK		Sort(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const GlobalFolderSettings &globalFolderSettings) const;
	int CALLBACK		SortBySize(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByType(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByDate(int InternalIndex1,int InternalIndex2,DateType_t DateType) const;
	int CALLBACK		SortByTotalSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, bool TotalSize) const;
	int CALLBACK		SortByAttributes(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByRealSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByShortName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByOwner(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByVersionInfo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, VersionInfoType_t VersioninfoType) const;
	int CALLBACK		SortByShortcutTo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByHardlinks(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByExtension(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByItemDetails(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const SHCOLUMNID *pscid) const;
	int CALLBACK		SortByImageProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PROPID PropertyId) const;
	int CALLBACK		SortByVirtualComments(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByFileSystem(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByPrinterProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PrinterInformationType_t PrinterInformationType) const;
	int CALLBACK		SortByNetworkAdapterStatus(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const;
	int CALLBACK		SortByMediaMetadata(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, MediaMetadataType_t MediaMetaDataType) const;

	/* Listview column support. */
	void				PlaceColumns(void);
	void				QueueColumnTask(int itemInternalIndex, int columnIndex);
	static ColumnResult_t	GetColumnTextAsync(HWND listView, int columnResultId, unsigned int ColumnID, int InternalIndex, const BasicItemInfo_t &basicItemInfo, const GlobalFolderSettings &globalFolderSettings);
	void				InsertColumn(unsigned int ColumnId,int iColumndIndex,int iWidth);
	void				SetActiveColumnSet(void);
	void				GetColumnInternal(unsigned int id,Column_t *pci) const;
	void				SaveColumnWidths(void);
	void				ProcessColumnResult(int columnResultId);
	boost::optional<int>	GetColumnIndexById(unsigned int id) const;
	boost::optional<unsigned int>	GetColumnIdByIndex(int index) const;

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
	void				RemoveFilteredItems(void);
	void				RemoveFilteredItem(int iItem,int iItemInternal);
	void				UpdateFiltering(void);
	void				UnfilterAllItems(void);

	/* Listview group support (real files). */
	static INT CALLBACK	GroupNameComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData);
	INT CALLBACK		GroupNameComparison(INT Group1_ID, INT Group2_ID);
	static INT CALLBACK	GroupFreeSpaceComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData);
	INT CALLBACK		GroupFreeSpaceComparison(INT Group1_ID, INT Group2_ID);
	TCHAR				*RetrieveGroupHeader(int iGroupId);
	int					DetermineItemGroup(int iItemInternal);
	void				DetermineItemNameGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemDateGroup(int iItemInternal,int iDateType,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemAttributeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemOwnerGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemVersionGroup(int iItemInternal,TCHAR *szVersionType,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemCameraPropertyGroup(int iItemInternal,PROPID PropertyId,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemExtensionGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemFileSystemGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemNetworkStatus(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;

	/* Listview group support (virtual files). */
	void				DetermineItemTypeGroupVirtual(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemTotalSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemFreeSpaceGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const;
	void				DetermineItemSummaryGroup(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid, TCHAR *szGroupHeader, size_t cchMax, const GlobalFolderSettings &globalFolderSettings) const;

	/* Other grouping support. */
	int					CheckGroup(const TCHAR *szGroupHeader, PFNLVGROUPCOMPARE pfnGroupCompare);
	void				InsertItemIntoGroup(int iItem,int iGroupId);
	void				MoveItemsIntoGroups(void);

	/* LIstview icons. */
	void				QueueIconTask(int internalIndex);
	static boost::optional<IconResult_t>	FindIconAsync(HWND listView, int iconResultId, int internalIndex, const BasicItemInfo_t &basicItemInfo);
	void				ProcessIconResult(int iconResultId);

	/* Thumbnails view. */
	void				QueueThumbnailTask(int internalIndex);
	static boost::optional<ThumbnailResult_t>	FindThumbnailAsync(HWND listView, int thumbnailResultId, int internalIndex, const BasicItemInfo_t &basicItemInfo);
	void				ProcessThumbnailResult(int thumbnailResultId);
	void				SetupThumbnailsView(void);
	void				RemoveThumbnailsView(void);
	int					GetIconThumbnail(int iInternalIndex) const;
	int					GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const;
	int					GetThumbnailInternal(int iType, int iInternalIndex, HBITMAP hThumbnailBitmap) const;
	void				DrawIconThumbnailInternal(HDC hdcBacking, int iInternalIndex) const;
	void				DrawThumbnailInternal(HDC hdcBacking, HBITMAP hThumbnailBitmap) const;

	/* Tiles view. */
	void				InsertTileViewColumns(void);
	void				DeleteTileViewColumns(void);
	void				SetTileViewInfo(void);
	void				SetTileViewItemInfo(int iItem,int iItemInternal);

	/* Drag and Drop support. */
	HRESULT				InitializeDragDropHelpers(void);
	DWORD				CheckItemLocations(IDataObject *pDataObject,int iDroppedItem);
	void				HandleDragSelection(const POINT *ppt);
	void				RepositionLocalFiles(const POINT *ppt);
	void				ScrollListViewFromCursor(HWND hListView, const POINT *CursorPos);
	void				PositionDroppedItems(void);
	void				OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt);

	/* Miscellaneous. */
	BOOL				CompareVirtualFolders(UINT uFolderCSIDL) const;
	int					LocateFileItemInternalIndex(const TCHAR *szFileName) const;
	boost::optional<int>	LocateItemByInternalIndex(int internalIndex) const;
	void				ApplyHeaderSortArrow(void);
	void				QueryFullItemNameInternal(int iItemInternal,TCHAR *szFullFileName,UINT cchMax) const;


	int					m_iRefCount;

	HWND				m_hListView;
	BOOL				m_ListViewSubclassed;
	BOOL				m_ListViewParentSubclassed;
	HWND				m_hOwner;

	// Each instance of this class will subclass the parent window. As
	// they'll all use the same static procedure, it's important that
	// they use different subclass IDs (as the procedure and ID uniquely
	// identify a subclass).
	static int listViewParentSubclassIdCounter;
	int m_listViewParentSubclassId;

	BOOL				m_bPerformingDrag;
	HIMAGELIST			m_hListViewImageList;

	int					m_itemIDCounter;

	/* Stores various extra information on files, such
	as display name. */
	std::unordered_map<int, ItemInfo_t>	m_itemInfoMap;

	ctpl::thread_pool	m_columnThreadPool;
	std::unordered_map<int, std::future<ColumnResult_t>> m_columnResults;
	int					m_columnResultIDCounter;

	ctpl::thread_pool	m_itemImageThreadPool;

	std::unordered_map<int, std::future<boost::optional<IconResult_t>>> m_iconResults;
	int					m_iconResultIDCounter;

	std::unordered_map<int, std::future<boost::optional<ThumbnailResult_t>>> m_thumbnailResults;
	int					m_thumbnailResultIDCounter;

	/* Cached folder size data. */
	mutable std::unordered_map<int, ULONGLONG>	m_cachedFolderSizes;

	/* Manages browsing history. */
	CPathManager		m_pathManager;

	/* Internal state. */
	LPITEMIDLIST		m_pidlDirectory;
	HINSTANCE			m_hResourceModule;
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
	int					m_iUniqueFolderIndex;

	/* User options variables. */
	BOOL				m_bInsertSorted;

	const GlobalFolderSettings	*m_globalFolderSettings;
	FolderSettings		m_folderSettings;

	/* ID. */
	int					m_ID;

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
	int					m_nAwaitingAdd;

	/* Shell new. */
	BOOL				m_bNewItemCreated;
	LPITEMIDLIST		m_pidlNewItem;
	int					m_iIndexNewItem;

	/* File selection. */
	std::list<std::wstring>	m_FileSelectionList;

	/* Thumbnails. */
	BOOL				m_bThumbnailsSetup;

	/* Column related data. */
	std::list<Column_t> *m_pActiveColumnList;
	std::list<Column_t>	m_RealFolderColumnList;
	std::list<Column_t>	m_MyComputerColumnList;
	std::list<Column_t>	m_ControlPanelColumnList;
	std::list<Column_t>	m_RecycleBinColumnList;
	std::list<Column_t>	m_PrintersColumnList;
	std::list<Column_t>	m_NetworkConnectionsColumnList;
	std::list<Column_t>	m_MyNetworkPlacesColumnList;
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
	DragTypes_t			m_DragType;
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