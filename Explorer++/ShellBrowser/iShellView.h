#pragma once

#include <list>
#include "iPathManager.h"
#include "../Helper/Helper.h"
#include "../Helper/DropHandler.h"
#include "../Helper/StringHelper.h"
#include "../Helper/Macros.h"

#define WM_USER_UPDATEWINDOWS		(WM_APP + 17)
#define WM_USER_FILESADDED			(WM_APP + 51)
#define WM_USER_STARTEDBROWSING		(WM_APP + 55)
#define WM_USER_NEWITEMINSERTED		(WM_APP + 200)
#define WM_USER_FOLDEREMPTY			(WM_APP + 201)
#define WM_USER_FILTERINGAPPLIED	(WM_APP + 202)
#define WM_USER_GETCOLUMNNAMEINDEX	(WM_APP + 203)
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

typedef struct
{
	UINT	SortMode;
	UINT	ViewMode;
	BOOL	bSortAscending;
	BOOL	bShowInGroups;
	BOOL	bShowHidden;
	BOOL	bAutoArrange;
	BOOL	bGridlinesActive;
	BOOL	bApplyFilter;
	BOOL	bFilterCaseSensitive;
	BOOL	bShowFolderSizes;
	BOOL	bDisableFolderSizesNetworkRemovable;
	BOOL	bHideSystemFiles;
	BOOL	bHideLinkExtension;
	BOOL	bForceSize;
	SizeDisplayFormat_t	sdf;
	TCHAR	szFilter[512];

	/* Initial columns. */
	std::list<Column_t>	*pRealFolderColumnList;
	std::list<Column_t>	*pMyComputerColumnList;
	std::list<Column_t>	*pControlPanelColumnList;
	std::list<Column_t>	*pRecycleBinColumnList;
	std::list<Column_t>	*pPrintersColumnList;
	std::list<Column_t>	*pNetworkConnectionsColumnList;
	std::list<Column_t>	*pMyNetworkPlacesColumnList;
} InitialSettings_t;

typedef struct
{
	BOOL bShowExtensions;
	BOOL bShowSizesInBytes;
	BOOL bShowFriendlyDates;
	BOOL bShowFolderSizes;
} GlobalSettings_t;

typedef struct
{
	ULARGE_INTEGER TotalFolderSize;
	ULARGE_INTEGER TotalSelectionSize;
} FolderInfo_t;

typedef enum
{
	FSM_NAME				= 1,
	FSM_DATEMODIFIED		= 2,
	FSM_SIZE				= 3,
	FSM_TYPE				= 4,
	FSM_TOTALSIZE			= 5,
	FSM_FREESPACE			= 6,
	FSM_COMMENTS			= 7,
	FSM_DATEDELETED			= 8,
	FSM_ORIGINALLOCATION	= 9,
	FSM_ATTRIBUTES			= 10,
	FSM_REALSIZE			= 11,
	FSM_SHORTNAME			= 12,
	FSM_OWNER				= 13,

	FSM_PRODUCTNAME			= 14,
	FSM_COMPANY				= 15,
	FSM_DESCRIPTION			= 16,
	FSM_FILEVERSION			= 17,
	FSM_PRODUCTVERSION		= 18,

	FSM_SHORTCUTTO			= 19,
	FSM_HARDLINKS			= 20,
	FSM_EXTENSION			= 21,
	FSM_CREATED				= 22,
	FSM_ACCESSED			= 23,

	FSM_TITLE				= 24,
	FSM_SUBJECT				= 25,
	FSM_AUTHOR				= 26,
	FSM_KEYWORDS			= 27,

	FSM_CAMERAMODEL			= 29,
	FSM_DATETAKEN			= 30,
	FSM_WIDTH				= 31,
	FSM_HEIGHT				= 32,

	FSM_VIRTUALCOMMENTS		= 33,

	FSM_FILESYSTEM			= 34,

	FSM_NUMPRINTERDOCUMENTS	= 36,
	FSM_PRINTERSTATUS		= 37,
	FSM_PRINTERCOMMENTS		= 38,
	FSM_PRINTERLOCATION		= 39,

	FSM_NETWORKADAPTER_STATUS	= 40,

	FSM_MEDIA_BITRATE		= 41,
	FSM_MEDIA_COPYRIGHT		= 42,
	FSM_MEDIA_DURATION		= 43,
	FSM_MEDIA_PROTECTED		= 44,
	FSM_MEDIA_RATING		= 45,
	FSM_MEDIA_ALBUMARTIST	= 46,
	FSM_MEDIA_ALBUM			= 47,
	FSM_MEDIA_BEATSPERMINUTE	= 48,
	FSM_MEDIA_COMPOSER		= 49,
	FSM_MEDIA_CONDUCTOR		= 50,
	FSM_MEDIA_DIRECTOR		= 51,
	FSM_MEDIA_GENRE			= 52,
	FSM_MEDIA_LANGUAGE		= 53,
	FSM_MEDIA_BROADCASTDATE	= 54,
	FSM_MEDIA_CHANNEL		= 55,
	FSM_MEDIA_STATIONNAME	= 56,
	FSM_MEDIA_MOOD			= 57,
	FSM_MEDIA_PARENTALRATING	= 58,
	FSM_MEDIA_PARENTALRATINGREASON	= 59,
	FSM_MEDIA_PERIOD		= 60,
	FSM_MEDIA_PRODUCER		= 61,
	FSM_MEDIA_PUBLISHER		= 62,
	FSM_MEDIA_WRITER			= 63,
	FSM_MEDIA_YEAR			= 64
} SORT_MODES;

typedef enum
{
	VM_ICONS				= 1,
	VM_SMALLICONS			= 2,
	VM_LIST					= 3,
	VM_DETAILS				= 4,
	VM_TILES				= 5,
	VM_THUMBNAILS			= 6,
	VM_EXTRALARGEICONS		= 7,
	VM_LARGEICONS			= 8,
} VIEW_MODES;

typedef enum
{
	CM_NAME					= 1,
	CM_TYPE					= 2,
	CM_SIZE					= 3,
	CM_DATEMODIFIED			= 4,
	CM_ATTRIBUTES			= 5,
	CM_REALSIZE				= 6,
	CM_SHORTNAME			= 7,
	CM_OWNER				= 8,

	/*File version information.*/
	CM_PRODUCTNAME			= 9,
	CM_COMPANY				= 10,
	CM_DESCRIPTION			= 11,
	CM_FILEVERSION			= 12,
	CM_PRODUCTVERSION		= 13,

	CM_SHORTCUTTO			= 14,
	CM_HARDLINKS			= 15,
	CM_EXTENSION			= 16,
	CM_CREATED				= 17,
	CM_ACCESSED				= 18,

	/* File summary information. */
	CM_TITLE				= 19,
	CM_SUBJECT				= 20,
	CM_AUTHOR				= 21,
	CM_KEYWORDS				= 22,
	CM_COMMENT				= 23,

	/* Photo data. */
	CM_CAMERAMODEL			= 24,
	CM_DATETAKEN			= 25,
	CM_WIDTH				= 26,
	CM_HEIGHT				= 27,

	/* Control panel. */
	CM_VIRTUALCOMMENTS		= 28,

	/* My Computer. */
	CM_TOTALSIZE			= 29,
	CM_FREESPACE			= 30,
	CM_FILESYSTEM			= 31,

	/* Recycle Bin. */
	CM_ORIGINALLOCATION		= 33,
	CM_DATEDELETED			= 34,

	/* Printer columns. */
	CM_NUMPRINTERDOCUMENTS	= 35,
	CM_PRINTERSTATUS		= 36,
	CM_PRINTERCOMMENTS		= 37,
	CM_PRINTERLOCATION		= 38,

	/* Network connections columns. */
	CM_NETWORKADAPTER_STATUS	= 39,

	/* Media metadata. */
	CM_MEDIA_BITRATE		= 40,
	CM_MEDIA_COPYRIGHT		= 41,
	CM_MEDIA_DURATION		= 42,
	CM_MEDIA_PROTECTED		= 43,
	CM_MEDIA_RATING			= 44,
	CM_MEDIA_ALBUMARTIST	= 45,
	CM_MEDIA_ALBUM			= 46,
	CM_MEDIA_BEATSPERMINUTE	= 47,
	CM_MEDIA_COMPOSER		= 48,
	CM_MEDIA_CONDUCTOR		= 49,
	CM_MEDIA_DIRECTOR		= 50,
	CM_MEDIA_GENRE			= 51,
	CM_MEDIA_LANGUAGE		= 52,
	CM_MEDIA_BROADCASTDATE	= 53,
	CM_MEDIA_CHANNEL		= 54,
	CM_MEDIA_STATIONNAME	= 55,
	CM_MEDIA_MOOD			= 56,
	CM_MEDIA_PARENTALRATING	= 57,
	CM_MEDIA_PARENTALRATINGREASON	= 58,
	CM_MEDIA_PERIOD			= 59,
	CM_MEDIA_PRODUCER		= 60,
	CM_MEDIA_PUBLISHER		= 61,
	CM_MEDIA_WRITER			= 62,
	CM_MEDIA_YEAR			= 63,

	/* Printer columns. */
	CM_PRINTERMODEL			= 64
} COLUMNS;

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
	TCHAR szHeader[512];
	int iGroupId;

	/* Used to record the number of items in this group.
	Mimics the feature available in Windows Vista and later. */
	int nItems;
} TypeGroup_t;

class CShellBrowser : public IDropTarget, public IDropFilesCallback
{
	friend int CALLBACK SortStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
	friend void CALLBACK SetAllColumnDataAPC(ULONG_PTR dwParam);

public:

	/* TODO: Private. */
	static const int THUMBNAIL_ITEM_WIDTH = 120;
	static const int THUMBNAIL_ITEM_HEIGHT = 120;

	static CShellBrowser *CreateNew(HWND hOwner, HWND hListView,
		const InitialSettings_t *pSettings, HANDLE hIconThread,
		HANDLE hFolderSizeThread);

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
	BOOL				ToggleAutoArrange(void);
	UINT				GetCurrentViewMode() const;
	void				SetCurrentViewMode(UINT ViewMode);
	UINT				GetSortMode() const;
	void				SetSortMode(UINT SortMode);
	void				SortFolder(UINT SortMode);
	BOOL				IsGroupViewEnabled(void) const;
	BOOL				GetSortAscending(void) const;
	BOOL				SetSortAscending(BOOL bAscending);
	BOOL				ToggleSortAscending(void);
	BOOL				GetShowHidden(void) const;
	BOOL				SetShowHidden(BOOL bShowHidden);
	BOOL				ToggleShowHidden(void);
	BOOL				IsBackHistory(void) const;
	BOOL				IsForwardHistory(void) const;
	std::list<LPITEMIDLIST>	GetBackHistory() const;
	std::list<LPITEMIDLIST>	GetForwardHistory() const;
	LPITEMIDLIST		RetrieveHistoryItemWithoutUpdate(int iItem);
	LPITEMIDLIST		RetrieveHistoryItem(int iItem);
	BOOL				CanBrowseUp(void) const;
	int					QueryNumItems(void) const;
	int					QueryNumSelectedFiles(void) const;
	int					QueryNumSelectedFolders(void) const;
	int					QueryNumSelected(void) const;

	/* Settings. */
	void				SetUserOptions(const InitialSettings_t *is);
	void				SetGlobalSettings(const GlobalSettings_t *gs);

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
	LPWIN32_FIND_DATA	QueryFileFindData(int iItem) const;
	LPITEMIDLIST		QueryItemRelativeIdl(int iItem) const;
	DWORD				QueryFileAttributes(int iItem) const;
	int					QueryDisplayName(int iItem,UINT BufferSize,TCHAR *Buffer) const;
	BOOL				IsFileReal(int iItem) const;
	HRESULT				QueryFullItemName(int iIndex,TCHAR *FullItemPath,UINT cchMax) const;
	
	/* Column support. */
	void				ExportCurrentColumns(std::list<Column_t> *pColumns);
	void				ImportColumns(std::list<Column_t> *pColumns);

	/* Thumbnails view. */
	int					GetExtractedThumbnail(HBITMAP hThumbnailBitmap);

	/* Folder size support. */
	int					SetAllFolderSizeColumnData(void);

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
	void				QueryFolderInfo(FolderInfo_t *pFolderInfo);
	int					LocateFileItemIndex(const TCHAR *szFileName) const;
	BOOL				DeghostItem(int iItem);
	BOOL				GhostItem(int iItem);
	void				OnListViewGetDisplayInfo(LPARAM lParam);
	void				AddToIconFinderQueue(const LVITEM *plvItem);
	void				EmptyIconFinderQueue(void);
	void				AddToThumbnailFinderQueue(LPARAM lParam);
	void				EmptyThumbnailsQueue(void);
	BOOL				InVirtualFolder(void) const;
	BOOL				CanCreate(void) const;

	/* Column queueing. */
	void				AddToColumnQueue(int iItem);
	void				EmptyColumnQueue(void);
	BOOL				RemoveFromColumnQueue(int *iItem);

	/* Folder size queueing. */
	void				AddToFolderQueue(int iItem);
	void				EmptyFolderQueue(void);
	BOOL				RemoveFromFolderQueue(int *iItem);

	void				ToggleGrouping(void);
	void				SetGrouping(BOOL bShowInGroups);
	void				SetGroupingFlag(BOOL bShowInGroups);

	void				SetHideSystemFiles(BOOL bHideSystemFiles);
	void				SetShowExtensions(BOOL bShowExtensions);
	void				SetHideLinkExtension(BOOL bHideLinkExtension);
	void				SetShowFolderSizes(BOOL bShowFolderSizes);
	void				SetDisableFolderSizesNetworkRemovable(BOOL bDisableFolderSizesNetworkRemovable);
	void				SetShowFriendlyDates(BOOL bShowFriendlyDates);
	void				SetInsertSorted(BOOL bInsertSorted);
	void				SetForceSize(BOOL bForceSize);
	void				SetSizeDisplayFormat(SizeDisplayFormat_t sdf);

	int CALLBACK		SortTemporary(LPARAM lParam1,LPARAM lParam2);

	BOOL				GetTerminationStatus(void) const;
	void				SetTerminationStatus(void);

	void				ColumnClicked(int iClickedColumn);
	std::list<int>		QueryCurrentSortModes() const;
	size_t				QueryNumActiveColumns(void) const;
	void				ToggleGridlines(void);
	BOOL				QueryGridlinesActive(void) const;
	void				SetResourceModule(HINSTANCE hResourceModule);
	void				ImportAllColumns(const ColumnExport_t *pce);
	void				ExportAllColumns(ColumnExport_t *pce);
	void				QueueRename(LPCITEMIDLIST pidlItem);
	void				SelectItems(const std::list<std::wstring> &PastedFileList);
	void				RefreshAllIcons(void);
	void				OnDeviceChange(WPARAM wParam,LPARAM lParam);

private:

	DISALLOW_COPY_AND_ASSIGN(CShellBrowser);

	enum DateType_t
	{
		DATE_TYPE_CREATED,
		DATE_TYPE_MODIFIED,
		DATE_TYPE_ACCESSED
	};

	enum MediaMetadataType_t
	{
		MEDIAMETADATA_TYPE_BITRATE,
		MEDIAMETADATA_TYPE_COPYRIGHT,
		MEDIAMETADATA_TYPE_DURATION,
		MEDIAMETADATA_TYPE_PROTECTED,
		MEDIAMETADATA_TYPE_RATING,
		MEDIAMETADATA_TYPE_ALBUM_ARTIST,
		MEDIAMETADATA_TYPE_ALBUM_TITLE,
		MEDIAMETADATA_TYPE_BEATS_PER_MINUTE,
		MEDIAMETADATA_TYPE_COMPOSER,
		MEDIAMETADATA_TYPE_CONDUCTOR,
		MEDIAMETADATA_TYPE_DIRECTOR,
		MEDIAMETADATA_TYPE_GENRE,
		MEDIAMETADATA_TYPE_LANGUAGE,
		MEDIAMETADATA_TYPE_BROADCASTDATE,
		MEDIAMETADATA_TYPE_CHANNEL,
		MEDIAMETADATA_TYPE_STATIONNAME,
		MEDIAMETADATA_TYPE_MOOD,
		MEDIAMETADATA_TYPE_PARENTALRATING,
		MEDIAMETADATA_TYPE_PARENTALRATINGREASON,
		MEDIAMETADATA_TYPE_PERIOD,
		MEDIAMETADATA_TYPE_PRODUCER,
		MEDIAMETADATA_TYPE_PUBLISHER,
		MEDIAMETADATA_TYPE_WRITER,
		MEDIAMETADATA_TYPE_YEAR
	};

	enum TimeType_t
	{
		COLUMN_TIME_MODIFIED,
		COLUMN_TIME_CREATED,
		COLUMN_TIME_ACCESSED
	};

	enum VersionInfoType_t
	{
		VERSION_INFO_PRODUCT_NAME,
		VERSION_INFO_COMPANY,
		VERSION_INFO_DESCRIPTION,
		VERSION_INFO_FILE_VERSION,
		VERSION_INFO_PRODUCT_VERSION
	};

	enum PrinterInformationType_t
	{
		PRINTER_INFORMATION_TYPE_NUM_JOBS,
		PRINTER_INFORMATION_TYPE_STATUS,
		PRINTER_INFORMATION_TYPE_COMMENTS,
		PRINTER_INFORMATION_TYPE_LOCATION,
		PRINTER_INFORMATION_TYPE_MODEL
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

	static const int THUMBNAIL_ITEM_HORIZONTAL_SPACING = 20;
	static const int THUMBNAIL_ITEM_VERTICAL_SPACING = 20;

	CShellBrowser(HWND hOwner, HWND hListView,
		const InitialSettings_t *pSettings, HANDLE hIconThread,
		HANDLE hFolderSizeThread);
	~CShellBrowser();

	void				InitializeItemMap(int iStart,int iEnd);
	int					GenerateUniqueItemId(void);
	BOOL				GhostItemInternal(int iItem,BOOL bGhost);
	void				DetermineFolderVirtual(LPITEMIDLIST pidlDirectory);
	void				VerifySortMode(void);
	void				AllocateInitialItemMemory(void);

	/* Browsing support. */
	void				BrowseVirtualFolder(LPITEMIDLIST pidlDirectory);
	HRESULT				ParsePath(LPITEMIDLIST *pidlDirectory,UINT uFlags,BOOL *bWriteHistory);
	void inline			InsertAwaitingItems(BOOL bInsertIntoGroup);
	BOOL				IsFileFiltered(int iItemInternal) const;
	TCHAR				*ProcessItemFileName(int iItemInternal) const;
	HRESULT inline		AddItemInternal(LPITEMIDLIST pidlDirectory, LPITEMIDLIST pidlRelative, const TCHAR *szFileName, int iItemIndex, BOOL bPosition);
	HRESULT inline		AddItemInternal(int iItemIndex,int iItemId,BOOL bPosition);
	int inline			SetItemInformation(LPITEMIDLIST pidlDirectory, LPITEMIDLIST pidlRelative, const TCHAR *szFileName);
	void				ResetFolderMemoryAllocations(void);
	void				SetCurrentViewModeInternal(UINT ViewMode);

	/* Sorting. */
	int CALLBACK		Sort(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByName(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortBySize(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByType(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByDate(int InternalIndex1,int InternalIndex2,DateType_t DateType) const;
	int CALLBACK		SortByTotalSize(int InternalIndex1,int InternalIndex2,bool TotalSize) const;
	int CALLBACK		SortByAttributes(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByRealSize(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByShortName(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByOwner(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByVersionInfo(int InternalIndex1,int InternalIndex2,VersionInfoType_t VersioninfoType) const;
	int CALLBACK		SortByShortcutTo(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByHardlinks(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByExtension(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByItemDetails(int InternalIndex1, int InternalIndex2, const SHCOLUMNID *pscid) const;
	int CALLBACK		SortByImageProperty(int InternalIndex1,int InternalIndex2,PROPID PropertyId) const;
	int CALLBACK		SortByVirtualComments(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByFileSystem(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByPrinterProperty(int InternalIndex1,int InternalIndex2,PrinterInformationType_t PrinterInformationType) const;
	int CALLBACK		SortByNetworkAdapterStatus(int InternalIndex1,int InternalIndex2) const;
	int CALLBACK		SortByMediaMetadata(int InternalIndex1,int InternalIndex2,MediaMetadataType_t MediaMetaDataType) const;

	/* Listview column support. */
	void				SetAllColumnText(void);
	void				SetColumnText(UINT ColumnID,int ItemIndex,int ColumnIndex);
	void				PlaceColumns(void);
	std::wstring		GetColumnText(UINT ColumnID,int InternalIndex) const;
	void				InsertColumn(unsigned int ColumnId,int iColumndIndex,int iWidth);
	void				SetActiveColumnSet(void);
	unsigned int		DetermineColumnSortMode(int iColumnId) const;
	void				GetColumnInternal(unsigned int id,Column_t *pci) const;
	void				SaveColumnWidths(void);

	/* Listview columns. */
	std::wstring		GetNameColumnText(int InternalIndex) const;
	std::wstring		GetTypeColumnText(int InternalIndex) const;
	std::wstring		GetSizeColumnText(int InternalIndex) const;
	std::wstring		GetTimeColumnText(int InternalIndex,TimeType_t TimeType) const;
	std::wstring		GetAttributeColumnText(int InternalIndex) const;
	bool				GetRealSizeColumnRawData(int InternalIndex,ULARGE_INTEGER &RealFileSize) const;
	std::wstring		GetRealSizeColumnText(int InternalIndex) const;
	std::wstring		GetShortNameColumnText(int InternalIndex) const;
	std::wstring		GetOwnerColumnText(int InternalIndex) const;
	std::wstring		GetVersionColumnText(int InternalIndex,VersionInfoType_t VersioninfoType) const;
	std::wstring		GetShortcutToColumnText(int InternalIndex) const;
	DWORD				GetHardLinksColumnRawData(int InternalIndex) const;
	std::wstring		GetHardLinksColumnText(int InternalIndex) const;
	std::wstring		GetExtensionColumnText(int InternalIndex) const;
	HRESULT				GetItemDetails(int InternalIndex, const SHCOLUMNID *pscid, TCHAR *szDetail, size_t cchMax) const;
	HRESULT				GetItemDetailsRawData(int InternalIndex, const SHCOLUMNID *pscid, VARIANT *vt) const;
	std::wstring		GetItemDetailsColumnText(int InternalIndex, const SHCOLUMNID *pscid) const;
	std::wstring		GetImageColumnText(int InternalIndex,PROPID PropertyID) const;
	std::wstring		GetFileSystemColumnText(int InternalIndex) const;
	BOOL				GetDriveSpaceColumnRawData(int InternalIndex,bool TotalSize,ULARGE_INTEGER &DriveSpace) const;
	std::wstring		GetDriveSpaceColumnText(int InternalIndex,bool TotalSize) const;
	std::wstring		GetControlPanelCommentsColumnText(int InternalIndex) const;
	std::wstring		GetPrinterColumnText(int InternalIndex,PrinterInformationType_t PrinterInformationType) const;
	std::wstring		GetNetworkAdapterColumnText(int InternalIndex) const;
	std::wstring		GetMediaMetadataColumnText(int InternalIndex,MediaMetadataType_t MediaMetaDataType) const;
	const TCHAR			*GetMediaMetadataAttributeName(MediaMetadataType_t MediaMetaDataType) const;

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
	void				DetermineItemSummaryGroup(int iItemInternal, const SHCOLUMNID *pscid, TCHAR *szGroupHeader, size_t cchMax) const;

	/* Other grouping support. */
	int					CheckGroup(const TCHAR *szGroupHeader, PFNLVGROUPCOMPARE pfnGroupCompare);
	void				InsertItemIntoGroup(int iItem,int iGroupId);
	void				MoveItemsIntoGroups(void);

	/* Thumbnails view. */
	void				SetupThumbnailsView(void);
	void				RemoveThumbnailsView(void);
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
	void				HandleDragSelection(const POINT *ppt);
	void				RepositionLocalFiles(const POINT *ppt);
	void				ScrollListViewFromCursor(HWND hListView, const POINT *CursorPos);
	void				PositionDroppedItems(void);
	void				OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt);

	/* Miscellaneous. */
	BOOL				CompareVirtualFolders(UINT uFolderCSIDL) const;
	int					LocateFileItemInternalIndex(const TCHAR *szFileName) const;
	void				ApplyHeaderSortArrow(void);
	void				QueryFullItemNameInternal(int iItemInternal,TCHAR *szFullFileName,UINT cchMax) const;


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
	CPathManager *		m_pPathManager;

	HANDLE				m_hThread;
	HANDLE				m_hFolderSizeThread;

	/* Internal state. */
	LPITEMIDLIST		m_pidlDirectory;
	HINSTANCE			m_hResourceModule;
	HANDLE				m_hIconEvent;
	TCHAR				m_CurDir[MAX_PATH];
	ULARGE_INTEGER		m_ulTotalDirSize;
	ULARGE_INTEGER		m_ulFileSelectionSize;
	UINT				m_SortMode;
	UINT				m_ViewMode;
	BOOL				m_bVirtualFolder;
	BOOL				m_bFolderVisited;
	BOOL				m_bShowFolderSizes;
	BOOL				m_bDisableFolderSizesNetworkRemovable;
	BOOL				m_bForceSize;
	SizeDisplayFormat_t	m_SizeDisplayFormat;
	int					m_nTotalItems;
	int					m_NumFilesSelected;
	int					m_NumFoldersSelected;
	int					m_iCurrentAllocation;
	int					m_iCachedPosition;
	int					m_iDirMonitorId;
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
	TCHAR				m_szFilter[512];
	BOOL				m_bApplyFilter;
	BOOL				m_bFilterCaseSensitive;
};