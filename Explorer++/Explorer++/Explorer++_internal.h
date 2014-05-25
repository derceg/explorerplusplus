#pragma once

#include <list>
#include "../ShellBrowser/iShellView.h"
#include "../Helper/StatusBar.h"

/* Used to share global data across the
classes within the Explorer++ project. */
namespace NExplorerplusplus
{
	const TCHAR APP_NAME[]			= _T("Explorer++");
	const TCHAR CLASS_NAME[]		= _T("Explorer++");

	const TCHAR HELP_FILE_NAME[]	= _T("Explorer++ Help.chm");

	/* The name of the XML file that preferences are
	saved to/loaded from. */
	const TCHAR XML_FILENAME[]		= _T("config.xml");

	const TCHAR LOG_FILENAME[]		= _T("Explorer++.log");

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
and some of the other components (such as the
dialogs and toolbars). */
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

	void			OpenItem(LPCITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);

	CStatusBar		*GetStatusBar();

	HRESULT			BrowseFolder(const TCHAR *szPath,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);
	HRESULT			BrowseFolder(LPCITEMIDLIST pidlDirectory,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);

	void			OpenFileItem(LPCITEMIDLIST pidlItem,const TCHAR *szParameters);
};

struct ToolbarButton_t
{
	int iItemID;
};

/* Used when setting Explorer++ as the default
file manager. */
static const TCHAR SHELL_DEFAULT_INTERNAL_COMMAND_NAME[] = _T("openinexplorer++");

#define WM_APP_ASSOCCHANGED		(WM_APP + 54)
#define WM_APP_KEYDOWN			(WM_APP + 55)

/* Rebar menu id's. */
#define ID_REBAR_MENU_BACK_START	2000
#define ID_REBAR_MENU_BACK_END		2999
#define ID_REBAR_MENU_FORWARD_START	3000
#define ID_REBAR_MENU_FORWARD_END	3999

/* Small/large toolbar image sizes. */
#define TOOLBAR_IMAGE_SIZE_SMALL_X	16
#define TOOLBAR_IMAGE_SIZE_SMALL_Y	16
#define TOOLBAR_IMAGE_SIZE_LARGE_X	24
#define TOOLBAR_IMAGE_SIZE_LARGE_Y	24

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
static const int TOOLBAR_BUTTON_SET[] =
{TOOLBAR_BACK, TOOLBAR_FORWARD, TOOLBAR_UP, TOOLBAR_FOLDERS,
TOOLBAR_COPYTO, TOOLBAR_MOVETO, TOOLBAR_NEWFOLDER, TOOLBAR_COPY,
TOOLBAR_CUT, TOOLBAR_PASTE, TOOLBAR_DELETE, TOOLBAR_VIEWS,
TOOLBAR_SEARCH, TOOLBAR_PROPERTIES, TOOLBAR_REFRESH,
TOOLBAR_ADDBOOKMARK, TOOLBAR_NEWTAB, TOOLBAR_OPENCOMMANDPROMPT,
TOOLBAR_ORGANIZEBOOKMARKS, TOOLBAR_DELETEPERMANENTLY};

extern CRITICAL_SECTION g_csDirMonCallback;
extern BOOL g_bForceLanguageLoad;
extern TCHAR g_szLang[32];

BOOL TestConfigFileInternal(void);