#ifndef SHELLVIEW_INCLUDED
#define SHELLVIEW_INCLUDED

#include <windows.h>
#include <unknwn.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <list>
#include "iPathManager.h"
#include "../Helper/Helper.h"


#define WM_USER_UPDATEWINDOWS		(WM_APP + 17)
#define WM_USER_FILESADDED			(WM_APP + 51)
#define WM_USER_RELEASEBROWSER		(WM_APP + 52)
#define WM_USER_STARTEDBROWSING		(WM_APP + 55)
#define WM_USER_NEWITEMINSERTED		(WM_APP + 200)
#define WM_USER_FOLDEREMPTY			(WM_APP + 201)
#define WM_USER_FILTERINGAPPLIED	(WM_APP + 202)
#define WM_USER_GETCOLUMNNAMEINDEX	(WM_APP + 203)
#define WM_USER_DIRECTORYMODIFIED	(WM_APP + 204)

/* {A330851D-E911-41cd-8310-3BFC12A8D7F4} */
DEFINE_GUID(IID_IFolderView, 
0xa330851d, 0xe911, 0x41cd, 0x83, 0x10, 0x3b, 0xfc, 0x12, 0xa8, 0xd7, 0xf4);

/* {34B619D6-81F7-4f19-BFF1-093008FB61A3} */
DEFINE_GUID(IID_IShellView, 
0x34b619d6, 0x81f7, 0x4f19, 0xbf, 0xf1, 0x9, 0x30, 0x8, 0xfb, 0x61, 0xa3);

/* {CCBA1097-DF3D-4d4f-AD2D-41EF5D79AEA6} */
DEFINE_GUID(IID_IShellBrowser, 
0xccba1097, 0xdf3d, 0x4d4f, 0xad, 0x2d, 0x41, 0xef, 0x5d, 0x79, 0xae, 0xa6);

/* {EA626B86-B1C3-4c0d-BE71-8369FDD8EFE0} */
DEFINE_GUID(IID_IShellFolder, 
0xea626b86, 0xb1c3, 0x4c0d, 0xbe, 0x71, 0x83, 0x69, 0xfd, 0xd8, 0xef, 0xe0);

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
	FSM_VIRTUALTYPE			= 35,

	FSM_NUMPRINTERDOCUMENTS	= 36,
	FSM_PRINTERSTATUS		= 37,
	FSM_PRINTERCOMMENTS		= 38,
	FSM_PRINTERLOCATION		= 39,

	FSM_NETWORKADAPTER_STATUS	= 40
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
	CM_VIRTUALTYPE			= 32,

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
	//CM_MEDIA_TITLE
	//CM_MEDIA_SUBTITLE
	//CM_MEDIA_WIDTH
	//CM_MEDIA_HEIGHT
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

[
	  uuid(62F890DA-C361-11d1-A54D-0000F8751BA7)
]
__interface MyIFolderView2 : IUnknown
{
	virtual BOOL	GetAutoArrange(void);
	virtual HRESULT	SortFolder(UINT);
	virtual HRESULT	SetCurrentViewMode(DWORD);
	virtual HRESULT	GetCurrentViewMode(UINT *);
	virtual HRESULT	GetSortMode(UINT *);
	virtual HRESULT	SetSortMode(UINT SortMode);
	virtual BOOL	IsGroupViewEnabled(void);
	virtual BOOL	IsSortAscending(void);
	virtual int		GetId(void);
	virtual void	SetId(int ID);
	virtual void	SetTerminationStatus(void);
	virtual void	SetResourceModule(HINSTANCE hResourceModule);
};

__interface MyIShellView3 : IUnknown
{
	virtual HRESULT Refresh(void);
};

[
	  uuid(62F890DA-C361-11d1-A54D-0000F8751BA8)
]
__interface IShellBrowser2 : IUnknown
{
	virtual HRESULT			BrowseFolder(TCHAR *Path,UINT wFlags);
	virtual HRESULT			BrowseFolder(LPITEMIDLIST pidlDirectory,UINT wFlags);
	virtual void			UpdateFileSelectionInfo(int,BOOL);
	virtual int				GetFolderIndex(void);
	virtual void			FilesModified(DWORD Action,TCHAR *FileName,int EventId,int iFolderIndex);
	virtual void			ParentModified(DWORD Action,TCHAR *FileName);
	virtual void			DirectoryAltered(void);
	virtual HRESULT			GetStoredName(int,TCHAR *,unsigned int);
	virtual void			QueryName(int iIndex,TCHAR *FileName);
	virtual HRESULT			QueryFullItemName(int iIndex,TCHAR *FullItemPath);
	virtual UINT			QueryCurrentDirectory(int,TCHAR *);
	virtual LPITEMIDLIST	QueryCurrentDirectoryIdl(void);
	virtual HRESULT			CreateHistoryPopup(IN HWND hParent,OUT LPITEMIDLIST *pidl,IN POINT *pt,IN BOOL bBackOrForward);
	virtual int				QueryDisplayName(int iItem,UINT BufferSize,TCHAR *Buffer);
	virtual BOOL			IsBackHistory(void);
	virtual BOOL			IsForwardHistory(void);
	virtual void			GetBackHistory(std::list<LPITEMIDLIST> *lHistory);
	virtual void			GetForwardHistory(std::list<LPITEMIDLIST> *lHistory);
	virtual LPITEMIDLIST	RetrieveHistoryItemWithoutUpdate(int iItem);
	virtual LPITEMIDLIST	RetrieveHistoryItem(int iItem);
	virtual BOOL			CanBrowseUp();
	virtual LPWIN32_FIND_DATA	QueryItemInfo(int iItem);
	virtual void			ToggleGrouping(void);
	virtual void			SetGrouping(BOOL bShowInGroups);
	virtual void			SetGroupingFlag(BOOL bShowInGroups);
	virtual HRESULT			SetEnumFlags(DWORD);
	virtual int				SelectFiles(TCHAR *FileNamePattern);
	virtual void			SelectItems(const std::list<std::wstring> &PastedFileList);
	virtual DWORD			QueryFileAttributes(int iItem);
	virtual void			DragStarted(int iFirstItem,POINT *ptCursor);
	virtual LPWIN32_FIND_DATA	QueryFileFindData(int iItem);
	virtual void			OnListViewGetDisplayInfo(LPARAM lParam);
	virtual LPITEMIDLIST		QueryItemRelativeIdl(int iItem);
	virtual BOOL			InVirtualFolder(void);
	virtual BOOL			CanDeleteItem(int iItem);
	virtual BOOL			CanCreate(void);
	virtual void			SetDirMonitorId(int iDirMonitorId);
	virtual void			SetParentDirMointorId(int iParentDirMonitorId);
	virtual int				GetDirMonitorId(void);
	virtual int				GetParentDirMointorId(void);
	virtual void			DragStopped(void);
	virtual BOOL			IsFileReal(int iItem);
	virtual int				LocateFileItemIndex(const TCHAR *szFileName);
	virtual BOOL			DeghostItem(int iItem);
	virtual BOOL			GhostItem(int iItem);
	virtual int				QueryNumItems(void);
	virtual int				QueryNumSelectedFiles(void);
	virtual int				QueryNumSelectedFolders(void);
	virtual void			QueryFolderInfo(FolderInfo_t *pFolderInfo);
	virtual void			GetFilter(TCHAR *szFilter,int cchMax);
	virtual void			SetFilter(TCHAR *szFilter);
	virtual BOOL			GetFilterStatus(void);
	virtual void			SetFilterStatus(BOOL bFilter);
	virtual BOOL			GetFilterCaseSensitive(void);
	virtual void			SetFilterCaseSensitive(BOOL bCaseSensitive);
	virtual BOOL			ToggleSortAscending(void);
	virtual BOOL			ToggleAutoArrange(void);
	virtual BOOL			QuerySortAscending(void);
	virtual BOOL			QueryAutoArrange(void);
	virtual BOOL			QueryShowHidden(void);
	virtual BOOL			ToggleShowHidden(void);
	virtual BOOL			GetSortAscending(void);
	virtual BOOL			SetSortAscending(BOOL bAscending);
	virtual BOOL			SetShowHidden(BOOL bShowHidden);
	virtual void			SetUserOptions(InitialSettings_t *is);
	virtual InitialSettings_t	*QueryUserOptions(void);
	virtual BOOL			QueryDragging(void);
	virtual void			ColumnClicked(int iColumn);
	virtual void			QueryCurrentSortModes(std::list<int> *pSortModes);
	virtual void			SetGlobalSettings(GlobalSettings_t *gs);
	virtual int				QueryNumSelected(void);
	virtual size_t			QueryNumActiveColumns(void);
	virtual void			ToggleGridlines(void);
	virtual BOOL			QueryGridlinesActive(void);
	virtual HICON			GetItemIcon(int iItem);
	virtual void			ExportCurrentColumns(std::list<Column_t> *pColumns);
	virtual void			ImportColumns(std::list<Column_t> *pColumns,BOOL bColumnsSwapped);
	virtual void			ImportAllColumns(ColumnExport_t *pce);
	virtual void			ExportAllColumns(ColumnExport_t *pcie);
	virtual void			QueueRename(LPITEMIDLIST pidlItem);
	virtual void			RefreshAllIcons(void);
	virtual void			OnDeviceChange(WPARAM wParam,LPARAM lParam);
	virtual void			SetHideSystemFiles(BOOL bHideSystemFiles);
	virtual BOOL			GetHideSystemFiles(void);
	virtual void			SetShowExtensions(BOOL bShowExtensions);
	virtual BOOL			GetShowExtensions(void);
	virtual void			SetHideLinkExtension(BOOL bHideLinkExtension);
	virtual BOOL			GetHideLinkExtension(void);
	virtual void			SetShowFolderSizes(BOOL bShowFolderSizes);
	virtual BOOL			GetShowFolderSizes(void);
	virtual void			SetDisableFolderSizesNetworkRemovable(BOOL bDisableFolderSizesNetworkRemovable);
	virtual void			SetShowFriendlyDates(BOOL bShowFriendlyDates);
	virtual BOOL			GetShowFriendlyDates(void);
	virtual void			SetInsertSorted(BOOL bInsertSorted);
	virtual BOOL			GetInsertSorted(void);
	virtual void			SetForceSize(BOOL bForceSize);
	virtual void			SetSizeDisplayFormat(SizeDisplayFormat_t sdf);
};

__interface IShellFolder3 : IUnknown
{
	virtual HRESULT GetAttributesOf(TCHAR *,UINT *);
	virtual HRESULT CreateViewObject(HWND hOwner,REFIID iid,void **ppv);
};

void InitializeFolderView(HWND hOwner,HWND hListView,
MyIFolderView2 **pFolderView,InitialSettings_t *pSettings,
HANDLE hIconThread,HANDLE hFolderSizeThread);

#endif