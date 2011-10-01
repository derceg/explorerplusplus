#ifndef SHELLBROWSER_INTERNAL_INCLUDED
#define SHELLBROWSER_INTERNAL_INCLUDED

#include "../Helper/Helper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/DropHandler.h"

#define DEFAULT_MEM_ALLOC					250

#define MEM_ALLOCATION_LEVEL_LOW	1000
#define MEM_ALLOCATION_LEVEL_MEDIUM	5000

/* This is only defined for Windows Vista and later.
If building for XP as well, this definition will
need to be included here. */
#define SHIL_JUMBO	0x4

#define THUMBNAIL_TYPE_ICON			0
#define THUMBNAIL_TYPE_EXTRACTED	1

typedef struct
{
	HWND			hListView;
	LPITEMIDLIST	pidlFull;
	int				iItem;
	CItemObject		*m_pExtraItemInfo;
	HANDLE			hEvent;
} ListViewInfo_t;

extern std::list<ListViewInfo_t>	g_pListViewInfoList;
extern CRITICAL_SECTION	g_icon_cs;
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

#endif