#ifndef SHELLBROWSER_INTERNAL_INCLUDED
#define SHELLBROWSER_INTERNAL_INCLUDED

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include "../Helper/Helper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/DropHandler.h"

#define KBYTE					1024
#define MBYTE					1024 * 1024
#define GBYTE					1024 * 1024 *1024

#define DEFAULT_MEM_ALLOC					250

#define MEM_ALLOCATION_LEVEL_LOW	1000
#define MEM_ALLOCATION_LEVEL_MEDIUM	5000

/* Used for testing what version of Windows we're running on. */
#define WINDOWS_VISTA_MAJORVERSION	6
#define WINDOWS_XP_MAJORVERSION		5

/* This is only defined for Windows Vista and later.
If building for XP as well, this definition will
need to be included here. */
#define SHIL_JUMBO	0x4

#define VALIDATE_REALFOLDER_COLUMNS			0
#define VALIDATE_CONTROLPANEL_COLUMNS		1
#define VALIDATE_MYCOMPUTER_COLUMNS			2
#define VALIDATE_RECYCLEBIN_COLUMNS			3
#define VALIDATE_PRINTERS_COLUMNS			4
#define VALIDATE_NETWORKCONNECTIONS_COLUMNS	5
#define VALIDATE_MYNETWORKPLACES_COLUMNS	6

#define DEFAULT_UNKNOWN_ICON				3

#define THUMBNAIL_ITEM_WIDTH				120
#define THUMBNAIL_ITEM_HEIGHT				120
#define THUMBNAIL_ITEM_HORIZONTAL_SPACING	20
#define THUMBNAIL_ITEM_VERTICAL_SPACING		20

#define THUMBNAIL_TYPE_ICON			0
#define THUMBNAIL_TYPE_EXTRACTED	1

typedef struct
{
	TCHAR *pszFilter;
} Filter_t;

typedef struct
{
	TCHAR szFileName[MAX_PATH];
	POINT DropPoint;
} DroppedFile_t;

typedef struct
{
	TCHAR	szFileName[MAX_PATH];
	DWORD	dwAction;
	int		iFolderIndex;
} AlteredFile_t;

typedef struct
{
	TCHAR szHeader[512];
	int iGroupId;

	/* Used to record the number of items in this group.
	Mimics the feature available in Windows Vista and later. */
	int nItems;
} TypeGroup_t;

typedef struct
{
	int		iItem;
	int		iItemInternal;

	BOOL	bPosition;
	int		iAfter;
} AwaitingAdd_t;

typedef struct
{
	int		iItemInternal;
	TCHAR	szNewFileName[MAX_PATH];
} AwaitingRename_t;

typedef struct
{
	TCHAR szFileName[MAX_PATH];
} DraggedFile_t;

typedef struct
{
	int iColumnId;
	TCHAR ColumnText[256];
	BOOL bActive;
} Columns_t;

class CItemObject
{
public:

	LPITEMIDLIST	pridl;
	TCHAR			szDisplayName[MAX_PATH];
	BOOL			bReal;
	BOOL			bIconRetrieved;
	BOOL			bThumbnailRetreived;
	int				iIcon;

	/* Only used for folders. Records whether
	the folders size has been retrieved yet. */
	BOOL			bFolderSizeRetrieved;

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

typedef struct
{
	HWND			hListView;
	LPITEMIDLIST	pidlFull;
	int				iItem;
	CItemObject		*m_pExtraItemInfo;
	HANDLE			hEvent;
} ListViewInfo_t;

typedef struct
{
	TCHAR szFileName[MAX_PATH];
} Added_t;

extern std::list<ListViewInfo_t>	g_pListViewInfoList;
extern int					g_nItemsInInfoList;
extern int					g_nInfoListAllocation;
extern CRITICAL_SECTION	g_icon_cs;
extern BOOL				g_bIconThreadSleeping;
extern int					g_nAPCsRan;
extern int					g_nAPCsQueued;

/* Sort Modes. */
static const UINT RealFolderSortModes[] =
{FSM_NAME,FSM_SIZE,FSM_TYPE,FSM_DATEMODIFIED,FSM_ATTRIBUTES,
FSM_REALSIZE,FSM_SHORTNAME,FSM_OWNER,FSM_PRODUCTNAME,
FSM_COMPANY,FSM_DESCRIPTION,FSM_FILEVERSION,FSM_PRODUCTVERSION,
FSM_SHORTCUTTO,FSM_HARDLINKS,FSM_EXTENSION,FSM_CREATED,
FSM_ACCESSED,FSM_TITLE,FSM_SUBJECT,FSM_AUTHOR,FSM_KEYWORDS,
FSM_COMMENTS,FSM_CAMERAMODEL,FSM_DATETAKEN,FSM_WIDTH,FSM_HEIGHT};

static const UINT ControlPanelSortModes[] =
{FSM_NAME,FSM_VIRTUALCOMMENTS};

static const UINT MyComputerSortModes[] =
{FSM_NAME,FSM_VIRTUALTYPE,FSM_TOTALSIZE,FSM_FREESPACE,
FSM_VIRTUALCOMMENTS,FSM_FILESYSTEM};

static const UINT RecycleBinSortModes[] =
{FSM_NAME,FSM_ORIGINALLOCATION,FSM_DATEDELETED,
FSM_SIZE,FSM_TYPE,FSM_DATEMODIFIED};

static const UINT PrintersSortModes[] =
{FSM_NAME,FSM_NUMPRINTERDOCUMENTS,FSM_PRINTERSTATUS,
FSM_PRINTERCOMMENTS,FSM_PRINTERLOCATION};

static const UINT NetworkConnectionsSortModes[] =
{FSM_NAME,FSM_VIRTUALTYPE,FSM_NETWORKADAPTER_STATUS,
FSM_OWNER};

extern CRITICAL_SECTION g_csThumbnails;
extern BOOL g_bcsThumbnailInitialized;

/* Thumbnails. */
void CALLBACK	FindThumbnailAPC(ULONG_PTR dwParam);

void CALLBACK SetAllColumnDataAPC(ULONG_PTR dwParam);
void CALLBACK SetAllFolderSizeColumnDataAPC(ULONG_PTR dwParam);

int CALLBACK SortByDateModifiedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByTypeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortBySizeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByNameStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByTotalSizeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByFreeSpaceStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByCommentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByDateDeletedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByOriginalLocationStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByAttributesStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByRealSizeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByShortNameStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByOwnerStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByProductNameStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByCompanyStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByDescriptionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByFileVersionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByProductVersionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByShortcutToStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByHardlinksStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByExtensionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByDateCreatedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByDateAccessedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByTitleStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortBySubjectStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByAuthorStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByKeywordsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByCameraModelStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByDateTakenStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByWidthStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByHeightStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByVirtualCommentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByFileSystemStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByVirtualTypeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByNumPrinterDocumentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByPrinterStatusStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByPrinterCommentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByPrinterLocationStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
int CALLBACK SortByNetworkAdapterStatusStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

class CFolderView :  public MyIFolderView2, public MyIShellView3, public IShellBrowser2,
public IShellFolder3, public IDropTarget, public IDropFilesCallback
{
public:

	CFolderView(HWND hOwner,HWND hListView,InitialSettings_t *pSettings,
	HANDLE hIconThread,HANDLE hFolderSizeThread);
	~CFolderView();

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	/* Navigation. */
	HRESULT				BrowseFolder(LPITEMIDLIST pidlDirectory,UINT wFlags);
	HRESULT				BrowseFolder(TCHAR *szPath,UINT wFlags);
	HRESULT				Refresh(void);

	/* Drag and Drop. */
	void				DragStarted(int iFirstItem,POINT *ptCursor);
	void				DragStopped(void);
	HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragLeave(void);
	HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect);

	/* Get/Set current state. */
	LPITEMIDLIST		QueryCurrentDirectoryIdl(void);
	UINT				QueryCurrentDirectory(int BufferSize,TCHAR *Buffer);
	BOOL				GetAutoArrange(void);
	HRESULT				SortFolder(UINT SortMode);
	HRESULT				SetCurrentViewMode(DWORD ViewMode);
	HRESULT				GetCurrentViewMode(UINT *pViewMode);
	HRESULT				GetSortMode(UINT *SortMode);
	HRESULT				SetSortMode(UINT SortMode);
	HRESULT				SetEnumFlags(DWORD pgrfFlags);
	BOOL				IsSortAscending(void);
	BOOL				IsGroupViewEnabled(void);
	BOOL				ToggleSortAscending(void);
	BOOL				GetSortAscending(void);
	BOOL				SetSortAscending(BOOL bAscending);
	BOOL				ToggleAutoArrange(void);
	BOOL				QuerySortAscending(void);
	BOOL				QueryAutoArrange(void);
	BOOL				QueryShowHidden(void);
	BOOL				SetShowHidden(BOOL bShowHidden);
	BOOL				ToggleShowHidden(void);
	BOOL				IsBackHistory(void);
	BOOL				IsForwardHistory(void);
	void				GetBackHistory(std::list<LPITEMIDLIST> *lHistory);
	void				GetForwardHistory(std::list<LPITEMIDLIST> *lHistory);
	LPITEMIDLIST		RetrieveHistoryItemWithoutUpdate(int iItem);
	LPITEMIDLIST		RetrieveHistoryItem(int iItem);
	BOOL				CanBrowseUp(void);
	int					QueryNumItems(void);
	int					QueryNumSelectedFiles(void);
	int					QueryNumSelectedFolders(void);
	int					QueryNumSelected(void);

	/* Settings. */
	void				SetUserOptions(InitialSettings_t *is);
	void				SetGlobalSettings(GlobalSettings_t *gs);
	InitialSettings_t	*QueryUserOptions(void);

	/* ID. */
	int					GetId(void);
	void				SetId(int ID);

	/* Directory modification support. */
	void				FilesModified(DWORD Action,TCHAR *FileName,int EventId,int iFolderIndex);
	void				ParentModified(DWORD Action,TCHAR *FileName);
	void				DirectoryAltered(void);
	void				SetDirMonitorId(int iDirMonitorId);
	void				SetParentDirMointorId(int iParentDirMonitorId);
	int					GetDirMonitorId(void);
	int					GetParentDirMointorId(void);
	int					GetFolderIndex(void);

	/* Item information. */
	LPWIN32_FIND_DATA	QueryItemInfo(int iItem);
	LPWIN32_FIND_DATA	QueryFileFindData(int iItem);
	LPITEMIDLIST		QueryItemRelativeIdl(int iItem);
	HRESULT				GetStoredName(int,TCHAR *,unsigned int);
	DWORD				QueryFileAttributes(int iItem);
	int					QueryDisplayName(int iItem,UINT BufferSize,TCHAR *Buffer);
	BOOL				IsFileReal(int iItem);
	void				QueryName(int iIndex,TCHAR *FileName);
	HRESULT				QueryFullItemName(int iIndex,TCHAR *FullItemPath);
	HICON				GetItemIcon(int iItem);
	
	/* Column support. */
	int					SetAllColumnData(void);
	void				ExportCurrentColumns(std::list<Column_t> *pColumns);
	void				ImportColumns(std::list<Column_t> *pColumns,BOOL bColumnsSwapped);

	/* Thumbnails view. */
	int					GetExtractedThumbnail(HBITMAP hThumbnailBitmap);

	/* Folder size support. */
	int					SetAllFolderSizeColumnData(void);

	/* Filtering. */
	void				GetFilter(TCHAR *szFilter,int cchMax);
	void				SetFilter(TCHAR *szFilter);
	BOOL				GetFilterStatus(void);
	void				SetFilterStatus(BOOL bFilter);
	BOOL				GetFilterCaseSensitive(void);
	void				SetFilterCaseSensitive(BOOL bCaseSensitive);

	void				UpdateFileSelectionInfo(int,BOOL);
	HRESULT				CreateHistoryPopup(IN HWND hParent,OUT LPITEMIDLIST *pidl,IN POINT *pt,IN BOOL bBackOrForward);
	int					SelectFiles(TCHAR *FileNamePattern);
	void				QueryFolderInfo(FolderInfo_t *pFolderInfo);
	int					LocateFileItemIndex(const TCHAR *szFileName);
	BOOL				DeghostItem(int iItem);
	BOOL				GhostItem(int iItem);
	void				OnListViewGetDisplayInfo(LPARAM lParam);
	void				AddToIconFinderQueue(LVITEM *plvItem);
	void				EmptyIconFinderQueue(void);
	void				AddToThumbnailFinderQueue(LPARAM lParam);
	void				EmptyThumbnailsQueue(void);
	BOOL				InVirtualFolder(void);
	BOOL				CanDeleteItem(int iItem);
	BOOL				CanCreate(void);
	HRESULT				GetAttributesOf(TCHAR *Object,UINT *Attributes);
	HRESULT				CreateViewObject(HWND hOwner,REFIID iid,void **ppv);

	/* Column queueing. */
	void				AddToColumnQueue(int iItem);
	void				EmptyColumnQueue(void);
	BOOL				RemoveFromColumnQueue(int *iItem);

	/* Folder size queueing. */
	void				AddToFolderQueue(int iItem);
	void				EmptyFolderQueue(void);
	BOOL				RemoveFromFolderQueue(int *iItem);

	/* Listview sorting. */
	int CALLBACK		Sort(LPARAM lParam1,LPARAM lParam2);
	int					SortByDate(LPARAM lParam1,LPARAM lParam2,int DateType);
	int CALLBACK		SortByName(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortBySize(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByType(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByDateModified(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByTotalSize(LPARAM lParam1,LPARAM lParam2,BOOL bTotalSize);
	int CALLBACK		SortByComments(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByDateDeleted(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByOriginalLocation(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByAttributes(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByRealSize(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByShortName(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByOwner(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByProductName(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByCompany(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByDescription(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByFileVersion(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByProductVersion(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByVersionInfo(LPARAM lParam1,LPARAM lParam2,int VersionProperty);
	int CALLBACK		SortByShortcutTo(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByHardlinks(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByExtension(LPARAM lParam1,LPARAM lParam2);
	int	CALLBACK		SortByDateCreated(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByDateAccessed(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByTitle(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortBySubject(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByAuthor(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByKeywords(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortBySummaryProperty(LPARAM lParam1,LPARAM lParam2,DWORD dwPropertyType);
	int CALLBACK		SortByCameraModel(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByDateTaken(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByWidth(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByHeight(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByImageProperty(LPARAM lParam1,LPARAM lParam2,PROPID PropertyId);
	int CALLBACK		SortByVirtualComments(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByFileSystem(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByVirtualType(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByNumPrinterDocuments(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByPrinterStatus(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByPrinterComments(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByPrinterLocation(LPARAM lParam1,LPARAM lParam2);
	int CALLBACK		SortByNetworkAdapterStatus(LPARAM lParam1,LPARAM lParam2);

	int CALLBACK		SortTemporary(LPARAM lParam1,LPARAM lParam2);

	BOOL				GetTerminationStatus(void);
	void				SetTerminationStatus(void);
	void				Terminate(void);
	void				SetThumbnailFlag(CItemObject *m_pExtraItemInfo);

	int					GetNumAPCsRun(void);
	int					GetNumAPCsQueued(void);
	void				IncrementNumAPCsRan(void);

	void				ColumnClicked(int iClickedColumn);
	void				QueryCurrentSortModes(std::list<int> *pSortModes);
	size_t				QueryNumActiveColumns(void);
	void				ToggleGridlines(void);
	BOOL				QueryGridlinesActive(void);
	void				SetResourceModule(HINSTANCE hResourceModule);
	void				ImportAllColumns(ColumnExport_t *pce);
	void				ExportAllColumns(ColumnExport_t *pce);
	void				QueueRename(LPITEMIDLIST pidlItem);
	void				SelectItems(const std::list<std::wstring> &PastedFileList);
	void				RefreshAllIcons(void);
	void				OnDeviceChange(WPARAM wParam,LPARAM lParam);


private:

	/* Internal private functions. */
	void				InitializeItemMap(int iStart,int iEnd);
	int					GenerateUniqueItemId(void);
	BOOL				GhostItemInternal(int iItem,BOOL bGhost);
	void				DetermineFolderVirtual(LPITEMIDLIST pidlDirectory);
	void				VerifySortMode(void);
	void				AllocateInitialItemMemory(void);

	/* Browsing support. */
	int					BrowseVirtualFolder(TCHAR *szParsingName);
	int					BrowseVirtualFolder(LPITEMIDLIST pidlDirectory);
	HRESULT				ParsePath(LPITEMIDLIST *pidlDirectory,UINT uFlags,BOOL *bWriteHistory);
	void inline			InsertAwaitingItems(BOOL bInsertIntoGroup);
	BOOL				IsFileFiltered(int iItemInternal);
	TCHAR				*ProcessItemFileName(int iItemInternal);
	HRESULT inline		AddItemInternal(LPITEMIDLIST pidlDirectory,LPITEMIDLIST pidlRelative,TCHAR *szFileName,int iItemIndex,BOOL bPosition);
	HRESULT inline		AddItemInternal(int iItemIndex,int iItemId,BOOL bPosition);
	int inline			SetItemInformation(LPITEMIDLIST pidlDirectory,LPITEMIDLIST pidlRelative,TCHAR *szFileName);
	void				CheckFolderLockState(TCHAR *szParsingPath);
	void				ResetFolderMemoryAllocations(void);
	void				SetCurrentViewModeInternal(DWORD ViewMode);

	/* Listview column support. */
	void				PlaceColumns(void);
	void				SetColumnData(unsigned int ColumnId,int iItem,int iColumnIndex);
	void				InsertColumn(unsigned int ColumnId,int iColumndIndex,int iWidth);
	void				SetActiveColumnSet(void);
	unsigned int		DetermineColumnSortMode(int iColumnId);
	void				GetColumnInternal(unsigned int id,Column_t *pci);
	void				SaveColumnWidths(void);

	/* Listview columns - set column data. */
	int					SetNameColumnData(HWND hListView,int iItem,int iColumn);
	int					SetSizeColumnData(HWND hListView,int iItem,int iColumn);
	int					SetRealSizeColumnData(HWND hListView,int iItem,int iColumn);
	int					SetTypeColumnData(HWND hListView,int iItem,int iColumn);
	void				SetVirtualTypeColumnData(int iItem,int iColumn);
	void				SetTotalSizeColumnData(int iItem,int iColumn,BOOL bTotalSize);
	void				SetFileSystemColumnData(int iItem,int iColumn);
	int					SetTimeColumnData(HWND hListView,int iItem,int iColumn,int TimeType);
	int					SetAttributeColumnData(HWND hListView,int iItem,int iColumn);
	int					SetShortNameColumnData(HWND hListView,int iItem,int iColumn);
	int					SetOwnerColumnData(HWND hListView,int iItem,int iColumn);
	int					SetVersionColumnData(HWND hListView,int iItem,int iColumn,TCHAR *lpszVersion);
	int					SetShortcutColumnData(HWND hListView,int iItem,int iColumn);
	int					SetHardLinksColumnData(HWND hListView,int iItem,int iColumn);
	int					SetExtensionColumnData(HWND hListView,int iItem,int iColumn);
	int					SetSummaryColumnData(HWND hListView,int iItem,int iColumn,DWORD dwPropertyType);
	int					SetImageColumnData(HWND hListView,int iItem,int iColumn,PROPID PropertyId);
	void				SetControlPanelComments(int iItem,int iColumn);
	void				SetNumPrinterDocumentsColumnData(int iItem,int iColumn);
	void				SetPrinterStatusColumnData(int iItem,int iColumn);
	void				SetPrinterCommentsColumnData(int iItem,int iColumn);
	void				SetPrinterLocationColumnData(int iItem,int iColumn);
	void				SetPrinterModelColumnData(int iItem,int iColumn);
	void				SetNetworkAdapterStatusColumnData(int iItem,int iColumn);
	void				SetMediaStatusColumnData(int iItem,int iColumn,int iType);

	/* Device change support. */
	void				UpdateDriveIcon(TCHAR *szDrive);
	void				RemoveDrive(TCHAR *szDrive);
	
	/* Directory altered support. */
	void				OnFileActionAdded(TCHAR *szFileName);
	void				RemoveItem(int iItemInternal);
	void				RemoveItemInternal(TCHAR *szFileName);
	void				ModifyItemInternal(TCHAR *FileName);
	void				OnFileActionRenamedOldName(TCHAR *szFileName);
	void				OnFileActionRenamedNewName(TCHAR *szFileName);
	void				RenameItem(int iItemInternal,TCHAR *szNewFileName);
	int					DetermineItemSortedPosition(LPARAM lParam);
	int					SortItemsRelative(LPARAM lParam1,LPARAM lParam2);
	int					DetermineRelativeItemPositions(LPARAM lParam1,LPARAM lParam2);

	/* Filtering support. */
	BOOL				IsFilenameFiltered(TCHAR *FileName);
	void				RemoveFilteredItems(void);
	void				RemoveFilteredItem(int iItem,int iItemInternal);
	void				UpdateFiltering(void);
	void				UnfilterAllItems(void);

	/* Listview group support (real files). */
	int					DetermineItemGroup(int iItemInternal);
	void				DetermineItemNameGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemDateGroup(int iItemInternal,int iDateType,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemAttributeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemOwnerGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemVersionGroup(int iItemInternal,TCHAR *szVersionType,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemCameraPropertyGroup(int iItemInternal,PROPID PropertyId,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemExtensionGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemFileSystemGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemNetworkStatus(int iItemInternal,TCHAR *szGroupHeader,int cchMax);

	/* Listview group support (virtual files). */
	void				DetermineItemTypeGroupVirtual(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemTotalSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemFreeSpaceGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax);
	void				DetermineItemCommentGroup(int iItemInternal,DWORD dwPropertyType,TCHAR *szGroupHeader,int cchMax);

	/* Other grouping support. */
	int					CheckGroup(TCHAR *szGroupHeader,PFNLVGROUPCOMPARE pfnGroupCompare);
	void				InsertItemIntoGroup(int iItem,int iGroupId);
	void				ToggleGrouping(void);
	void				SetGrouping(BOOL bShowInGroups);
	void				SetGroupingFlag(BOOL bShowInGroups);
	void				MoveItemsIntoGroups(void);
	void				HandleNameGroup(int iGroupId);

	/* Thumbnails view. */
	void				SetupThumbnailsView(void);
	void				RemoveThumbnailsView(void);
	HICON				GetItemIconInternal(int iItemInternal);
	int					GetIconThumbnail(int iInternalIndex);
	int					GetThumbnailInternal(int iType,int iInternalIndex,HBITMAP hThumbnailBitmap);
	void				DrawIconThumbnailInternal(HDC hdcBacking,int iInternalIndex);
	void				DrawThumbnailInternal(HDC hdcBacking,HBITMAP hThumbnailBitmap);

	/* Tiles view. */
	void				InsertTileViewColumns(void);
	void				DeleteTileViewColumns(void);
	void				SetTileViewInfo(void);
	void				SetTileViewItemInfo(int iItem,int iItemInternal);

	/* Drag and Drop support. */
	HRESULT				InitializeDragDropHelpers(void);
	DWORD				CheckItemLocations(IDataObject *pDataObject,int iDroppedItem);
	void				HandleDragSelection(POINT *ppt);
	void				RepositionLocalFiles(POINT *ppt);
	void				ScrollListViewFromCursor(HWND hListView,POINT *CursorPos);
	void				PositionDroppedItems(void);
	BOOL				QueryDragging(void);
	void				OnDropFile(const std::list<std::wstring> &PastedFileList,POINT *ppt);

	/* Miscellaneous. */
	BOOL				CompareVirtualFolders(UINT uFolderCSIDL);
	int					LocateFileItemInternalIndex(const TCHAR *szFileName);
	HRESULT				RetrieveItemInfoTip(int iItem,TCHAR *szInfoTip,size_t cchMax);
	void				ApplyHeaderSortArrow(void);
	void				QueryFullItemNameInternal(int iItemInternal,TCHAR *szFullFileName);
	void				CopyColumnsInternal(std::list<Column_t> *pInternalColumns,std::list<Column_t> *pColumns);
	void				SetHideSystemFiles(BOOL bHideSystemFiles);
	BOOL				GetHideSystemFiles(void);
	void				SetShowExtensions(BOOL bShowExtensions);
	BOOL				GetShowExtensions(void);
	void				SetHideLinkExtension(BOOL bHideLinkExtension);
	BOOL				GetHideLinkExtension(void);
	void				SetShowFolderSizes(BOOL bShowFolderSizes);
	BOOL				GetShowFolderSizes(void);
	void				SetDisableFolderSizesNetworkRemovable(BOOL bDisableFolderSizesNetworkRemovable);
	void				SetShowFriendlyDates(BOOL bShowFriendlyDates);
	BOOL				GetShowFriendlyDates(void);
	void				SetInsertSorted(BOOL bInsertSorted);
	BOOL				GetInsertSorted(void);
	void				SetForceSize(BOOL bForceSize);
	void				SetSizeDisplayFormat(SizeDisplayFormat_t sdf);




	/* ------ Internal state. ------ */

	int					m_iRefCount;

	HWND				m_hListView;
	HWND				m_hOwner;

	BOOL				m_bPerformingDrag;
	BOOL				m_bNotifiedOfTermination;
	HIMAGELIST			m_hListViewImageList;

	/* Stores a WIN32_FIND_DATA structure for each file.
	Only valid for 'real' files. */
	WIN32_FIND_DATA *	m_pwfdFiles;

	/* Stores various extra information on files, such
	as display name. */
	CItemObject *		m_pExtraItemInfo;

	/* Manages browsing history. */
	IPathManager *		m_pPathManager;

	HANDLE				m_hThread;
	HANDLE				m_hFolderSizeThread;
	int					m_nAPCsRan;
	int					m_nAPCsQueued;

	/* Internal state. */
	LPITEMIDLIST		m_pidlDirectory;
	HINSTANCE			m_hResourceModule;
	HANDLE				m_hIconEvent;
	TCHAR				m_CurDir[MAX_PATH];
	ULARGE_INTEGER		m_ulTotalDirSize;
	ULARGE_INTEGER		m_ulFileSelectionSize;
	DWORD				m_EnumFlags;
	DWORD				m_dwMajorVersion;
	UINT				m_SortMode;
	UINT				m_ViewMode;
	BOOL				m_bVirtualFolder;
	BOOL				m_bFolderVisited;
	BOOL				m_bCurrentFolderRenamed;
	BOOL				m_bShowGridlines;
	BOOL				m_bFolderChanging;
	BOOL				m_bShowFolderSizes;
	BOOL				m_bDisableFolderSizesNetworkRemovable;
	BOOL				m_bViewSet;
	BOOL				m_bForceSize;
	SizeDisplayFormat_t	m_SizeDisplayFormat;
	int					m_nTotalItems;
	int					m_PreviousNumSelected;
	int					m_NumFilesSelected;
	int					m_NumFoldersSelected;
	int					m_iCurrentAllocation;
	int					m_iCachedPosition;
	int					m_iDirMonitorId;
	int					m_iParentDirMonitorId;
	int					m_iFolderIcon;
	int					m_iFileIcon;
	int *				m_pItemMap;
	int					m_iDropped;

	/* Stores a unique index for each folder.
	This may be needed so that folders can be
	told apart when adding files from directory
	modification. */
	int					m_iUniqueFolderIndex;

	/* User options variables. */
	BOOL				m_bUnlockFolders;
	BOOL				m_bAutoArrange;
	BOOL				m_bShowInGroups;
	BOOL				m_bSortAscending;
	BOOL				m_bShowFriendlyDates;
	BOOL				m_bGridlinesActive;
	BOOL				m_bShowHidden;
	BOOL				m_bShowExtensions;
	BOOL				m_bHideSystemFiles;
	BOOL				m_bHideLinkExtension;
	BOOL				m_bInsertSorted;

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

	/* Icon gathering information. */
	int					m_nItemsInInfoList;
	int					m_nInfoListAllocation;
	BOOL				m_bIconThreadSleeping;

	/* Column gathering information. */
	std::list<int>		m_pColumnInfoList;
	CRITICAL_SECTION	m_column_cs;
	HANDLE				m_hColumnQueueEvent;

	/* Folder size information. */
	std::list<int>		m_pFolderInfoList;
	CRITICAL_SECTION	m_folder_cs;
	HANDLE				m_hFolderQueueEvent;

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
	TCHAR				m_szFilter[512];
	BOOL				m_bApplyFilter;
	BOOL				m_bFilterCaseSensitive;

	BOOL volatile		m_bBrowsing;
};

#endif