#ifndef EXPLORERINTERNAL_INCLUDED
#define EXPLORERINTERNAL_INCLUDED

#include <list>
#include "../ShellBrowser/iShellView.h"
#include "../Helper/StatusBar.h"

/* Used to share global data across the
classes within the Explorer++ project. */
namespace NExplorerplusplus
{
	const TCHAR WINDOW_NAME[]		= _T("Explorer++");
	const TCHAR CLASS_NAME[]		= _T("Explorer++");

	const TCHAR HELP_FILE_NAME[]	= _T("Explorer++ Help.chm");

	/* The name of the XML file that preferences are
	saved to/loaded from. */
	const TCHAR XML_FILENAME[]		= _T("config.xml");

	/* Command line arguments supplied to the program
	for each jump list task. */
	const TCHAR JUMPLIST_TASK_NEWTAB_ARGUMENT[]	= _T("-open_new_tab");

	/* When sending data across Explorer++ processes via
	WM_COPYDATA, the type (as specified by this enumeration)
	must ALWAYS be specified first. */
	enum IPNotificationType_t
	{
		IP_NOTIFICATION_TYPE_NEW_TAB = 1,

		/* Bookmark notifications. */
		IP_NOTIFICATION_TYPE_BOOKMARK_ADDED = 2,
		IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_ADDED = 3,
		IP_NOTIFICATION_TYPE_BOOKMARK_MODIFIED = 4,
		IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_MODIFIED = 5,
		IP_NOTIFICATION_TYPE_BOOKMARK_REMOVED = 6,
		IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_REMOVED = 7
	};

	struct IPBookmarkNotification_t
	{
		/* Use a series of different structures. This field
		must ALWAYS appear first in each of the structures.
		Decode by switching on the type and casting to the
		appropriate structure. */
		IPNotificationType_t Type;
	};
}

/* Basic interface between Explorerplusplus
and the dialogs. */
__interface IExplorerplusplus
{
	HWND			GetActiveListView() const;
	CShellBrowser	*GetActiveShellBrowser() const;

	std::wstring	GetTabName(int iTab) const;
	void			SetTabName(int iTab,std::wstring strName,BOOL bUseCustomName);
	void			RefreshTab(int iTabId);
	int				GetCurrentTabId() const;

	/* Temporary. */
	void			SetTabSelection(int Index);

	int				LookupColumnNameStringIndex(int iColumnId);
	int				LookupColumnDescriptionStringIndex(int iColumnId);

	void			OpenItem(LPITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);

	CStatusBar		*GetStatusBar();

	HRESULT			BrowseFolder(const TCHAR *szPath,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);
	HRESULT			BrowseFolder(LPITEMIDLIST pidlDirectory,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);
};

extern HINSTANCE g_hLanguageModule;

/* Used when setting Explorer++ as the default
file manager. */
#define SHELL_DEFAULT_INTERNAL_COMMAND_NAME _T("openinexplorer++")
#define SHELL_DEFAULT_MENU_TEXT _T("Open In Explorer++")

/* This is only defined for Windows Vista and later.
If building for XP as well, this definition will
need to be included here. */
#ifndef SHIL_JUMBO
	#define SHIL_JUMBO	0x4
#endif

#define WM_APP_TABMCLICK		(WM_APP + 53)
#define WM_APP_ASSOCCHANGED		(WM_APP + 54)
#define WM_APP_KEYDOWN			(WM_APP + 55)

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

#define VALIDATE_REALFOLDER_COLUMNS			0
#define VALIDATE_CONTROLPANEL_COLUMNS		1
#define VALIDATE_MYCOMPUTER_COLUMNS			2
#define VALIDATE_RECYCLEBIN_COLUMNS			3
#define VALIDATE_PRINTERS_COLUMNS			4
#define VALIDATE_NETWORKCONNECTIONS_COLUMNS	5
#define VALIDATE_MYNETWORKPLACES_COLUMNS	6

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
#define TOOLBAR_OPENCOMMANDPROMPT	(TOOLBAR_ID_START + 20)
#define TOOLBAR_ORGANIZEBOOKMARKS	(TOOLBAR_ID_START + 21)
#define TOOLBAR_DELETEPERMANENTLY	(TOOLBAR_ID_START + 22)

/* This represents the TOTAL set off buttons that may be
placed on the toolbar. */
const int ToolbarButtonSet[] =
{TOOLBAR_BACK,TOOLBAR_FORWARD,TOOLBAR_UP,TOOLBAR_FOLDERS,
TOOLBAR_COPYTO,TOOLBAR_MOVETO,TOOLBAR_NEWFOLDER,TOOLBAR_COPY,
TOOLBAR_CUT,TOOLBAR_PASTE,TOOLBAR_DELETE,TOOLBAR_VIEWS,
TOOLBAR_SEARCH,TOOLBAR_PROPERTIES,TOOLBAR_REFRESH,
TOOLBAR_ADDBOOKMARK,TOOLBAR_NEWTAB,TOOLBAR_OPENCOMMANDPROMPT,
TOOLBAR_ORGANIZEBOOKMARKS,TOOLBAR_DELETEPERMANENTLY};

#define TOOLBAR_BOOKMARK_START			(TOOLBAR_ID_START + 1000)
#define TOOLBAR_BOOKMARK_END			(TOOLBAR_BOOKMARK_START + 1000)
#define TOOLBAR_DRIVES_ID_START			(TOOLBAR_BOOKMARK_END + 1)
#define TOOLBAR_DRIVES_ID_END			(TOOLBAR_DRIVES_ID_START + 1000)
#define TOOLBAR_APPLICATIONS_ID_START	(TOOLBAR_DRIVES_ID_END + 1)
#define TOOLBAR_APPLICATIONS_ID_END		(TOOLBAR_APPLICATIONS_ID_START + 1000)

/* These define the order of the images
within the shell bitmap resource. The
order of these definitions is critical
(in that each individual image has a
specific and fixed location within the
bitmap). */
#define SHELLIMAGES_BACK				 0
#define SHELLIMAGES_FORWARD				 1
#define SHELLIMAGES_CUT					 2
#define SHELLIMAGES_COPY				 3
#define SHELLIMAGES_PASTE				 4
#define SHELLIMAGES_DELETE				 5
#define SHELLIMAGES_PROPERTIES			 6
#define SHELLIMAGES_VIEWS				 7
#define SHELLIMAGES_UP					 8
#define SHELLIMAGES_NEWFOLDER			 9
#define SHELLIMAGES_MOVETO				10
#define SHELLIMAGES_COPYTO				11
#define SHELLIMAGES_REFRESH				12
#define SHELLIMAGES_STOP				13
#define SHELLIMAGES_FOLDERS				14
#define SHELLIMAGES_NEWTAB				15
#define SHELLIMAGES_HELP				16
#define SHELLIMAGES_SEARCH				17
#define SHELLIMAGES_CMD					18
#define SHELLIMAGES_ADDFAV				19
#define SHELLIMAGES_FAV					20
#define SHELLIMAGES_MENUCHECKMARK		21
#define SHELLIMAGES_RIGHTARROW			22
#define SHELLIMAGES_CUSTOMIZECOLORS		23
#define SHELLIMAGES_RENAME				24
#define SHELLIMAGES_OPTIONS				25
#define SHELLIMAGES_FILTER				26
#define SHELLIMAGES_UNDO				27
#define SHELLIMAGES_REDO				28
#define SHELLIMAGES_LOCK				29
#define SHELLIMAGES_PASTESHORTCUT		30
#define SHELLIMAGES_DELETEPERMANENTLY	31
#define SHELLIMAGES_CMDADMIN			32

/* This structure is stored with
every listview. */
struct ListViewInfo_t
{
	int		iObjectIndex;
};

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

struct FolderSizeExtraInfo_t
{
	void	*pContainer;
	int		uId;
};

struct DWFolderSize_t
{
	int	uId;
	int	iTabId;
	BOOL bValid;
};

struct DWFolderSizeCompletion_t
{
	ULARGE_INTEGER	liFolderSize;
	int				uId;
	int				iTabId;
};

/* Used to store settings for individual directories. */
struct DirectorySettingsInternal_t
{
	UINT	SortMode;
	UINT	ViewMode;

	std::list<Column_t>	RealFolderColumnList;
	std::list<Column_t>	MyComputerColumnList;
	std::list<Column_t>	ControlPanelColumnList;
	std::list<Column_t>	RecycleBinColumnList;
	std::list<Column_t>	PrintersColumnList;
	std::list<Column_t>	NetworkConnectionsColumnList;
	std::list<Column_t>	MyNetworkPlacesColumnList;
};

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
{CM_VIRTUALCOMMENTS,TRUE,DEFAULT_COLUMN_WIDTH}};

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

extern CRITICAL_SECTION g_csDirMonCallback;
extern BOOL g_bForceLanguageLoad;
extern TCHAR g_szLang[32];

extern HWND g_hwndSearch;
extern HWND g_hwndOptions;
extern HWND g_hwndManageBookmarks;

/* Save/load interface. This allows multiple
methods of saving/loading data, as long as it
conforms to this specification. */
class ILoadSave
{
public:

	virtual ~ILoadSave(){};

	/* Loading functions. */
	virtual void	LoadGenericSettings() = 0;
	virtual void	LoadBookmarks() = 0;
	virtual int		LoadPreviousTabs() = 0;
	virtual void	LoadDefaultColumns() = 0;
	virtual void	LoadApplicationToolbar() = 0;
	virtual void	LoadToolbarInformation() = 0;
	virtual void	LoadColorRules() = 0;
	virtual void	LoadState() = 0;

	/* Saving functions. */
	virtual void	SaveGenericSettings() = 0;
	virtual void	SaveBookmarks() = 0;
	virtual void	SaveTabs() = 0;
	virtual void	SaveDefaultColumns() = 0;
	virtual void	SaveApplicationToolbar() = 0;
	virtual void	SaveToolbarInformation() = 0;
	virtual void	SaveColorRules() = 0;
	virtual void	SaveState() = 0;
};

BOOL TestConfigFileInternal(void);
BOOL LoadWindowPosition(WINDOWPLACEMENT *pwndpl);
BOOL LoadWindowPositionFromXML(WINDOWPLACEMENT *pwndpl);
BOOL LoadAllowMultipleInstancesFromRegistry(void);
BOOL LoadAllowMultipleInstancesFromXML(void);

#endif