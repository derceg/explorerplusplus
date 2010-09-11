/*************************************************************
Explorer++_internal.h
Contains declarations and defintions that are private
to Explorer++.cpp. Should not be included in external modules.
*************************************************************/

#ifndef EXPLORERINTERNAL_INCLUDED
#define EXPLORERINTERNAL_INCLUDED

#include <list>
#include "Misc.h"
#include "Settings.h"
#include "iDropSource.h"
#include "../Helper/Controls.h"
#include "../Helper/Registry.h"
#include "../Helper/Helper.h"
#include "../Helper/FolderSize.h"
#include "../MyTreeView/MyTreeView.h"
#include "../Helper/iDataObject.h"
#include "../Helper/Buffer.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Bookmark.h"
#include "../Helper/CustomMenu.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../HolderWindow/HolderWindow.h"
#include "MainResource.h"


extern HINSTANCE g_hLanguageModule;

#define VERSION_NUMBER		_T("1.2")

#define WINDOW_NAME			_T("Explorer++")
#define CLASS_NAME			_T("Explorer++")
#define WEBSITE_URL			_T("http://www.explorerplusplus.com")
#define WEBSITE_URL_TEXT	_T("www.explorerplusplus.com"

/* The name of the XML file that preferences are
saved to/loaded from. */
#define XML_FILENAME	_T("config.xml")

#define CONTROL_PANEL_CATEGORY_VIEW	_T("::{26EE0668-A00A-44D7-9371-BEB064C98683}")

/* Command line arguments supplied to the program
for each jump list task. */
#define JUMPLIST_TASK_NEWTAB_ARGUMENT	_T("-open_new_tab")

#define LANG_SINHALA	1115

/* This is only defined for Windows Vista and later.
If building for XP as well, this definition will
need to be included here. */
#ifndef SHIL_JUMBO
	#define SHIL_JUMBO	0x4
#endif

/* These are used with the IncludeItems()
method of INewMenuClient. INewMenuClient
is used to support the shell 'new' menu. */
#define NMCII_ITEMS		0x0001
#define NMCII_FOLDERS	0x0002

/* These two flags are used with the
SelectAndEdit() method of INewClient. */
#define NMCSAEI_SELECT	0x0000
#define NMCSAEI_EDIT	0x0001

/* Default window size/position. */
#define DEFAULT_WINDOWPOS_LEFT_PERCENTAGE	0.02
#define DEFAULT_WINDOWPOS_TOP_PERCENTAGE	0.05
#define DEFAULT_WINDOWPOS_WIDTH_PERCENTAGE	0.96
#define DEFAULT_WINDOWPOS_HEIGHT_PERCENTAGE	0.82

#define LISTVIEW_RENAME_FILENAME	0
#define LISTVIEW_RENAME_EXTENSION	1
#define LISTVIEW_RENAME_ENTIRE		2

#define SORTBY_BASE	50000
#define SORTBY_END	50099

#define GROUPBY_BASE	50100
#define GROUPBY_END		50199

#define DEFAULT_SORT_MODE			FSM_NAME

#define MAX_TABS					100
#define LISTVIEW_COLUMN_WIDTH		180
#define TREEVIEW_HEADERSIZE			16
#define MIN_TREEVIEW_WIDTH			20
#define MIN_TREEVIEW_HEIGHT			400

/* Display window defaults. */
#define MINIMUM_DISPLAYWINDOW_HEIGHT	70
#define DEFAULT_DISPLAYWINDOW_HEIGHT	90

#define DEFAULT_DISPLAYWINDOW_CENTRE_COLOR		Color(255,255,255)
#define DEFAULT_DISPLAYWINDOW_SURROUND_COLOR	Color(0,94,138)

/* The default color used by the new/edit color rule
dialog. */
#define DEFAULT_INITIAL_COLOR	RGB(0,94,138)

#define WM_USER_TABMCLICK		(WM_APP + 53)
#define WM_USER_ASSOCCHANGED	(WM_APP + 54)
#define WM_USER_KEYDOWN			(WM_APP + 55)

/* Window constant abstractions. Used
for window cycling. */
#define WINDOW_LISTVIEW		0
#define WINDOW_TREEVIEW		1
#define WINDOW_ADDRESSBAR	2

/* The number of toolbars that appear in the
main rebar. */
#define NUM_MAIN_TOOLBARS	5

/* Main toolbar id's. */
#define	ID_MAINTOOLBAR			0
#define	ID_ADDRESSTOOLBAR		1
#define	ID_BOOKMARKSTOOLBAR		2
#define	ID_DRIVESTOOLBAR		3
#define	ID_APPLICATIONSTOOLBAR	4

/* Rebar menu id's. */
#define ID_REBAR_MENU_BACK_START	2000
#define ID_REBAR_MENU_BACK_END		2999
#define ID_REBAR_MENU_FORWARD_START	3000
#define ID_REBAR_MENU_FORWARD_END	3999

/* Tab drag and drop timer information. */
#define TABDRAG_TIMER_ID		0
#define TABDRAG_TIMER_ELAPSED	500

/* These are used to support the undo
functionality. They define the various
operations that can be undone. */
#define FILE_UNDOACTION_RENAMED		0
#define FILE_UNDOACTION_COPIED		1
#define FILE_UNDOACTION_MOVED		2
#define FILE_UNDOACTION_DELETE		3

#define FOLDER_SIZE_LINE_INDEX	1

/* The treeview is offset by a small
amount on the left. */
#define TREEVIEW_X_CLEARANCE		1

/* The offset from the top of the parent
window to the treeview. */
#define TREEVIEW_Y_CLEARANCE		20

/* The spacing between the right edge of
the treeview and the holder window. */
#define TREEVIEW_HOLDER_CLEARANCE	4

/* Defines the distance between the cursor
and the right edge of the treeview during
a resizing operation. */
#define TREEVIEW_DRAG_OFFSET		8

#define FOLDERS_TOOLBAR_X_OFFSET	(-20)
#define FOLDERS_TOOLBAR_Y_OFFSET	3

/* Width and height of the toolbar on
the folders pane. */
#define FOLDERS_TOOLBAR_WIDTH		16
#define FOLDERS_TOOLBAR_HEIGHT		16

/* Small/large toolbar image sizes. */
#define TOOLBAR_IMAGE_SIZE_SMALL_X	16
#define TOOLBAR_IMAGE_SIZE_SMALL_Y	16
#define TOOLBAR_IMAGE_SIZE_LARGE_X	24
#define TOOLBAR_IMAGE_SIZE_LARGE_Y	24

#define TAB_TOOLBAR_X_OFFSET		(-20)
#define TAB_TOOLBAR_Y_OFFSET		5

/* Width and height of the toolbar that
appears on the tab control. */
#define TAB_TOOLBAR_WIDTH			20
#define TAB_TOOLBAR_HEIGHT			20

#define DEFAULT_COLUMN_WIDTH	150

#define LISTVIEW_WHEEL_MULTIPLIER	3
#define TREEVIEW_WHEEL_MULTIPLIER	3

/* These definitions are needed to target
Windows 7 specific features, while remaining
compliant with XP and Vista. They are copied
directly from the appropriate header file. */
#define WM_DWMSENDICONICTHUMBNAIL           0x0323
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP   0x0326
#define MSGFLT_ADD 1

typedef BOOL (WINAPI *ChangeWindowMessageFilterProc)(UINT message,DWORD dwFlag);
typedef HRESULT (STDAPICALLTYPE *DwmSetWindowAttributeProc)(HWND hwnd,DWORD dwAttribute,LPCVOID pvAttribute,DWORD cbAttribute);
typedef HRESULT (STDAPICALLTYPE *DwmSetIconicThumbnailProc)(HWND hwnd,HBITMAP hbmp,DWORD dwSITFlags);
typedef HRESULT (STDAPICALLTYPE *DwmSetIconicLivePreviewBitmapProc)(HWND hwnd,HBITMAP hbmp,POINT *pptClient,DWORD dwSITFlags);
typedef HRESULT (STDAPICALLTYPE *DwmInvalidateIconicBitmapsProc)(HWND hwnd);

#define VALIDATE_REALFOLDER_COLUMNS			0
#define VALIDATE_CONTROLPANEL_COLUMNS		1
#define VALIDATE_MYCOMPUTER_COLUMNS			2
#define VALIDATE_RECYCLEBIN_COLUMNS			3
#define VALIDATE_PRINTERS_COLUMNS			4
#define VALIDATE_NETWORKCONNECTIONS_COLUMNS	5
#define VALIDATE_MYNETWORKPLACES_COLUMNS	6

/* Keep all identifiers below 70000. Above what can be handled. */
#define TOOLBAR_ID_START			45000
#define TOOLBAR_SEPARATOR			(TOOLBAR_ID_START + 1)
#define TOOLBAR_BACK				(TOOLBAR_ID_START + 2)
#define TOOLBAR_FORWARD				(TOOLBAR_ID_START + 3)
#define TOOLBAR_UP					(TOOLBAR_ID_START + 4)
#define TOOLBAR_FOLDERS				(TOOLBAR_ID_START + 5)
#define TOOLBAR_COPYTO				(TOOLBAR_ID_START + 6)
#define TOOLBAR_MOVETO				(TOOLBAR_ID_START + 7)
#define TOOLBAR_NEWFOLDER			(TOOLBAR_ID_START + 8)
#define TOOLBAR_COPY				(TOOLBAR_ID_START + 9)
#define TOOLBAR_CUT					(TOOLBAR_ID_START + 10)
#define TOOLBAR_PASTE				(TOOLBAR_ID_START + 11)
#define TOOLBAR_DELETE				(TOOLBAR_ID_START + 12)
#define TOOLBAR_VIEWS				(TOOLBAR_ID_START + 13)
#define TOOLBAR_SEARCH				(TOOLBAR_ID_START + 14)
#define TOOLBAR_PROPERTIES			(TOOLBAR_ID_START + 15)
#define TOOLBAR_REFRESH				(TOOLBAR_ID_START + 17)
#define TOOLBAR_ADDBOOKMARK			(TOOLBAR_ID_START + 18)
#define TOOLBAR_NEWTAB				(TOOLBAR_ID_START + 19)
#define TOOLBAR_SHOWCOMMANDPROMPT	(TOOLBAR_ID_START + 20)
#define TOOLBAR_ORGANIZEBOOKMARKS	(TOOLBAR_ID_START + 21)

/* This represents the TOTAL set off buttons that may be
placed on the toolbar. */
const int ToolbarButtonSet[] =
{TOOLBAR_BACK,TOOLBAR_FORWARD,TOOLBAR_UP,TOOLBAR_FOLDERS,
TOOLBAR_COPYTO,TOOLBAR_MOVETO,TOOLBAR_NEWFOLDER,TOOLBAR_COPY,
TOOLBAR_CUT,TOOLBAR_PASTE,TOOLBAR_DELETE,TOOLBAR_VIEWS,
TOOLBAR_SEARCH,TOOLBAR_PROPERTIES,TOOLBAR_REFRESH,
TOOLBAR_ADDBOOKMARK,TOOLBAR_NEWTAB,TOOLBAR_SHOWCOMMANDPROMPT,
TOOLBAR_ORGANIZEBOOKMARKS};

#define TOOLBAR_BOOKMARK_START		46000
#define TOOLBAR_DRIVES_ID_START		47000
#define TOOLBAR_ADDRESSBAR_GO		48000
#define TOOLBAR_APPLICATIONS_ID_START	49000

/* These define the order of the images
within the shell bitmap resource. The
order of these definitions is critical
(in that each individual image has a
specific and fixed location within the
bitmap). */
#define SHELLIMAGES_BACK			0
#define SHELLIMAGES_FORWARD			1
#define SHELLIMAGES_CUT				2
#define SHELLIMAGES_COPY			3
#define SHELLIMAGES_PASTE			4
#define SHELLIMAGES_DELETE			5
#define SHELLIMAGES_PROPERTIES		6
#define SHELLIMAGES_VIEWS			7
#define SHELLIMAGES_UP				8
#define SHELLIMAGES_NEWFOLDER		9
#define SHELLIMAGES_MOVETO			10
#define SHELLIMAGES_COPYTO			11
#define SHELLIMAGES_REFRESH			12
#define SHELLIMAGES_STOP			13
#define SHELLIMAGES_FOLDERS			14
#define SHELLIMAGES_NEWTAB			15
#define SHELLIMAGES_HELP			16
#define SHELLIMAGES_SEARCH			17
#define SHELLIMAGES_CMD				18
#define SHELLIMAGES_ADDFAV			19
#define SHELLIMAGES_FAV				20
#define SHELLIMAGES_MENUCHECKMARK	21
#define SHELLIMAGES_RIGHTARROW		22
#define SHELLIMAGES_CUSTOMIZECOLORS	23
#define SHELLIMAGES_RENAME			24
#define SHELLIMAGES_OPTIONS			25
#define SHELLIMAGES_FILTER			26
#define SHELLIMAGES_UNDO			27
#define SHELLIMAGES_REDO			28
#define SHELLIMAGES_LOCK			29
#define SHELLIMAGES_PASTESHORTCUT	30

/* Tab icons. */
#define TAB_ICON_LOCK_INDEX			0

/* Used with the rename tab dialog. */
typedef struct
{
	void	*pContainer;
	int		iTab;
} RenameTabInfo_t;

/* Used with the bookmark propeties dialog. */
typedef struct
{
	void	*pContainer;
	void	*pBookmarkHandle;
} BookmarkPropertiesInfo_t;

typedef struct
{
	TCHAR Dir[MAX_PATH];
} TabDirectory_t;

typedef struct
{
	/* The type of operation that
	occurred. */
	UINT	Type;

	/* Name of the file. */
	TCHAR	szFileName[MAX_PATH + 1];

	/* If the file was renamed, this
	element contains the previous name
	of the file. */
	TCHAR	szOldFileName[MAX_PATH + 1];
} UndoItem_t;

/* This structure is stored with
every listview. */
typedef struct
{
	void	*pContainer;
	int		iObjectIndex;
} ListViewInfo_t;

/* This information is used by
the 'Add Bookmark' dialog. */
typedef struct
{
	void			*pContainer;
	void			*pParentBookmark;
	LPITEMIDLIST	pidlDirectory;
	BOOL			bExpandInitial;
} AddBookmarkInfo_t;

typedef struct
{
	RECT	rcNormalPosition;
	BOOL	bMaximized;
} InitialWindowPos_t;

struct ApplicationButton_t
{
	/* External. */
	TCHAR	szName[512];
	TCHAR	szCommand[512];
	BOOL	bShowNameOnToolbar;

	/* Internal. */
	ApplicationButton_t *pNext;
	ApplicationButton_t	*pPrevious;
	int		iImage;
};

typedef struct
{
	void	*pContainer;
	int		uId;
} FolderSizeExtraInfo_t;

typedef struct
{
	TCHAR szPattern[256];
} WildcardSelectInfo_t;

typedef struct
{
	TCHAR szDirectory[MAX_PATH];
} SearchDirectoryInfo_t;

typedef struct
{
	TCHAR szPattern[MAX_PATH];
} SearchPatternInfo_t;

typedef struct
{
	int	uId;
	int	iTabId;
	BOOL bValid;
} DWFolderSize_t;

typedef struct
{
	ULARGE_INTEGER	liFolderSize;
	int				uId;
	int				iTabId;
} DWFolderSizeCompletion_t;

typedef struct
{
	SizeDisplayFormat_t	sdf;
	TCHAR				szDisplayName[64];
} FileSize_t;

/* Used to store settings for individual directories. */
struct DirectorySettingsInternal_t
{
	UINT	SortMode;
	UINT	ViewMode;

	list<Column_t>	RealFolderColumnList;
	list<Column_t>	MyComputerColumnList;
	list<Column_t>	ControlPanelColumnList;
	list<Column_t>	RecycleBinColumnList;
	list<Column_t>	PrintersColumnList;
	list<Column_t>	NetworkConnectionsColumnList;
	list<Column_t>	MyNetworkPlacesColumnList;
};

struct DirectorySettings_t
{
	LPITEMIDLIST				pidlDirectory;
	DirectorySettingsInternal_t	dsi;
};

typedef enum
{
	REPLACEEXPLORER_NONE		= 1,
	REPLACEEXPLORER_FILESYSTEM	= 2,
	REPLACEEXPLORER_ALL			= 3
} ReplaceExplorerModes_t;

typedef enum
{
	DW_NAME,
	DW_TYPE,
	DW_SIZE,
	DW_DATEMODIFIED,
	DW_ATTRIBUTES,
	DW_REALSIZE,
	DW_SHORTNAME,
	DW_OWNER,

	/*File version information.*/
	DW_PRODUCTNAME,
	DW_COMPANY,
	DW_DESCRIPTION,
	DW_FILEVERSION,
	DW_PRODUCTVERSION,

	DW_SHORTCUTTO,
	DW_HARDLINKS,
	DW_EXTENSION,
	DW_CREATED,
	DW_ACCESSED,

	/*File summary information.*/
	DW_TITLE,
	DW_SUBJECT,
	DW_AUTHOR,
	DW_KEYWORDS,
	DW_COMMENT,

	/*Photo data.*/
	DW_CAMERAMODEL,
	DW_DATETAKEN,
	DW_WIDTH,
	DW_HEIGHT,

	/* Control panel. */
	DW_VIRTUALCOMMENTS,

	/* My Computer. */
	DW_TOTALSIZE,
	DW_FREESPACE,
	DW_FILESYSTEM,
	DW_VIRTUALTYPE,

	/* Recycle Bin. */
	DW_ORIGINALLOCATION,
	DW_DATEDELETED,

	/* Printer columns. */
	DW_NUMPRINTERDOCUMENTS,
	DW_PRINTERSTATUS,
	DW_PRINTERCOMMENTS,
	DW_PRINTERLOCATION,

	/* Network connections columns. */
	DW_NETWORKADAPTER_STATUS,

	/* Media metadata. */
	DW_MEDIA_BITRATE,
	DW_MEDIA_LENGTH,
	DW_MEDIA_RATING,
	DW_MEDIA_TITLE,
	DW_MEDIA_ALBUMARTIST,
	DW_MEDIA_ALBUMCOVERURL,
	DW_MEDIA_ALBUMTITLE,
	DW_MEDIA_BEATSPERMINUTE,
	DW_MEDIA_CATEGORY,
	DW_MEDIA_COMPOSER,
	DW_MEDIA_CONDUCTOR,
	DW_MEDIA_DIRECTOR,
	DW_MEDIA_PARENTALRATING,
	DW_MEDIA_PARENTALRATINGREASON,
	DW_MEDIA_PARTOFSET,
	DW_MEDIA_PRODUCER,
	DW_MEDIA_PUBLISHER,
	DW_MEDIA_TRACKNUMBER,
	DW_MEDIA_YEAR
	/*DW_MEDIA_,
	DW_MEDIA_,
	DW_MEDIA_,*/
} DWInfoType_t;

static Column_t g_RealFolderColumns[] =
{{CM_NAME,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_TYPE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_SIZE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_DATEMODIFIED,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_ATTRIBUTES,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_REALSIZE,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_SHORTNAME,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_OWNER,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_PRODUCTNAME,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_COMPANY,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_DESCRIPTION,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_FILEVERSION,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_PRODUCTVERSION,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_SHORTCUTTO,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_HARDLINKS,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_EXTENSION,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_CREATED,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_ACCESSED,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_TITLE,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_SUBJECT,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_AUTHOR,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_KEYWORDS,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_COMMENT,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_CAMERAMODEL,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_DATETAKEN,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_WIDTH,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_HEIGHT,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_BITRATE,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_COPYRIGHT,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_DURATION,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_PROTECTED,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_RATING,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_ALBUMARTIST,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_ALBUM,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_BEATSPERMINUTE,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_COMPOSER,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_CONDUCTOR,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_DIRECTOR,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_GENRE,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_LANGUAGE,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_BROADCASTDATE,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_CHANNEL,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_STATIONNAME,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_MOOD,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_PARENTALRATING,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_PARENTALRATINGREASON,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_PERIOD,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_PRODUCER,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_PUBLISHER,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_WRITER,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_MEDIA_YEAR,FALSE,DEFAULT_COLUMN_WIDTH}};

static Column_t g_ControlPanelColumns[] =
{{CM_NAME,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_VIRTUALCOMMENTS,TRUE,DEFAULT_COLUMN_WIDTH}};

static Column_t g_MyComputerColumns[] =
{{CM_NAME,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_VIRTUALTYPE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_TOTALSIZE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_FREESPACE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_VIRTUALCOMMENTS,FALSE,DEFAULT_COLUMN_WIDTH},
{CM_FILESYSTEM,FALSE,DEFAULT_COLUMN_WIDTH}};

static Column_t g_RecycleBinColumns[] =
{{CM_NAME,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_ORIGINALLOCATION,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_DATEDELETED,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_SIZE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_TYPE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_DATEMODIFIED,TRUE,DEFAULT_COLUMN_WIDTH}};

static Column_t g_PrintersColumns[] =
{{CM_NAME,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_NUMPRINTERDOCUMENTS,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_PRINTERSTATUS,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_PRINTERCOMMENTS,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_PRINTERLOCATION,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_PRINTERMODEL,TRUE,DEFAULT_COLUMN_WIDTH}};

static Column_t g_NetworkConnectionsColumns[] =
{{CM_NAME,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_VIRTUALTYPE,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_NETWORKADAPTER_STATUS,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_OWNER,TRUE,DEFAULT_COLUMN_WIDTH}};

static Column_t g_MyNetworkPlacesColumns[] =
{{CM_NAME,TRUE,DEFAULT_COLUMN_WIDTH},
{CM_VIRTUALCOMMENTS,TRUE,DEFAULT_COLUMN_WIDTH}};//{CM_NETWORKCOMPUTER,TRUE},{CM_NETWORKLOCATION,TRUE}

static unsigned int g_RealFolderHeaderList[] =
{CM_NAME,CM_TYPE,CM_SIZE,CM_DATEMODIFIED,
CM_AUTHOR,CM_TITLE};

static unsigned int g_ControlPanelHeaderList[] =
{CM_NAME,CM_VIRTUALCOMMENTS};

static unsigned int g_MyComputerHeaderList[] =
{CM_NAME,CM_VIRTUALTYPE,CM_TOTALSIZE,
CM_FREESPACE,CM_VIRTUALCOMMENTS,
CM_FILESYSTEM};

static unsigned int g_NetworkConnectionsHeaderList[] =
{CM_NAME,CM_VIRTUALTYPE,CM_NETWORKADAPTER_STATUS,
CM_OWNER};

static unsigned int g_NetworkHeaderList[] =
{CM_NAME,CM_VIRTUALCOMMENTS};

static unsigned int g_PrintersHeaderList[] =
{CM_NAME,CM_NUMPRINTERDOCUMENTS,CM_PRINTERSTATUS,
CM_PRINTERCOMMENTS,CM_PRINTERLOCATION};

static unsigned int g_RecycleBinHeaderList[] =
{CM_NAME,CM_ORIGINALLOCATION,CM_DATEDELETED,
CM_SIZE,CM_TYPE,CM_DATEMODIFIED};

const FileSize_t g_FileSizes[] =
{{FORMAT_BYTES,_T("Bytes")},
{FORMAT_KBYTES,_T("KB")},
{FORMAT_MBYTES,_T("MB")},
{FORMAT_GBYTES,_T("GB")},
{FORMAT_TBYTES,_T("TB")},
{FORMAT_PBYTES,_T("PB")}};

extern CRITICAL_SECTION g_csDirMonCallback;
extern BOOL g_bConfigFileFound;
extern BOOL g_bForceLanguageLoad;
extern TCHAR g_szLang[32];

extern HWND g_hwndSearch;
extern HWND g_hwndOptions;

/* Save/load interface. This allows multiple
methods of saving/loading data, as long as it
conforms to this specification. */
__interface ILoadSave : IUnknown
{
public:

	/* Loading functions. */
	virtual void	LoadGenericSettings(void);
	virtual LONG	LoadWindowPosition(InitialWindowPos_t *piwp);
	virtual void	LoadFilters(void);
	virtual void	LoadBookmarks();
	virtual int		LoadPreviousTabs(void);
	virtual void	LoadDefaultColumns(void);
	virtual void	LoadApplicationToolbar(void);
	virtual void	LoadToolbarInformation(void);
	virtual void	LoadColorRules(void);
	virtual void	LoadState(void);

	/* Saving functions. */
	virtual void	SaveGenericSettings(void);
	virtual LONG	SaveWindowPosition(void);
	virtual void	SaveFilters(void);
	virtual void	SaveBookmarks();
	virtual void	SaveTabs(void);
	virtual void	SaveDefaultColumns(void);
	virtual void	SaveApplicationToolbar(void);
	virtual void	SaveToolbarInformation(void);
	virtual void	SaveColorRules(void);
	virtual void	SaveState(void);
};

BOOL TestConfigFileInternal(void);
BOOL LoadAllowMultipleInstancesFromXML(void);

extern LRESULT CALLBACK ListViewSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK TreeViewHolderProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
LRESULT CALLBACK RebarSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
LRESULT CALLBACK BookmarksToolbarSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
LRESULT CALLBACK DrivesToolbarSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
LRESULT CALLBACK EditSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
LRESULT CALLBACK TabBackingProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
LRESULT CALLBACK TabProxyWndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK FilterProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK DestroyFilesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK ApplicationButtonPropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK ApplicationToolbarNewButtonProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK ColorFilteringProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK ColorRuleProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK SearchProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK DWChangeDetailsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK DWLinePropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
ATOM RegisterMainWindowClass(void);

/* Dialog handler stubs. */
INT_PTR CALLBACK	AboutDialogProcedure(HWND,UINT,WPARAM,LPARAM);
INT_PTR CALLBACK	MassRenameProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	RenameTabProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	SplitFileProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	MergeFilesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
int CALLBACK		PropSheetProcStub(HWND hDlg,UINT msg,LPARAM lParam);
INT_PTR CALLBACK	BookmarkTabDlgProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	NewBookmarkFolderProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	OrganizeBookmarksStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	BookmarkPropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	BookmarkFolderPropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	ChangeDisplayColoursStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	WildcardSelectProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	SetFileAttributesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	SelectColumnsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	SetDefaultColumnsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

/* Options dialog handler stubs. */
INT_PTR CALLBACK	FilesFoldersProcStub(HWND,UINT,WPARAM,LPARAM);
INT_PTR CALLBACK	WindowProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	GeneralSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	DefaultSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	TabSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

/* Window message handler stubs. */
LRESULT CALLBACK	TabSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK	ShellMenuHookProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK	ShellMenuHookProcStubMainWindow(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK	TabSubclassProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

/* Paste file callback (will select pasted files). */
void				PasteFilesCallback(void *pData,list<PastedFile_t> *pPastedFileList);
void				FolderSizeCallbackStub(int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize,LPVOID pData);

/* Treeview window procedure stub. */
LRESULT CALLBACK	TreeViewSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

/* Default file manager. */
BOOL				SetAsDefaultFileManagerFileSystem(void);
BOOL				SetAsDefaultFileManagerAll(void);
BOOL				SetAsDefaultFileManagerInternal(ReplaceExplorerModes_t ReplacementType);
BOOL				RemoveAsDefaultFileManagerFileSystem(void);
BOOL				RemoveAsDefaultFileManagerAll(void);
BOOL				RemoveAsDefaultFileManagerInternal(ReplaceExplorerModes_t ReplacementType);

typedef struct
{
	int		iIndex;
	int		iFolderIndex;
	void	*pData;
} DirectoryAltered_t;

#endif