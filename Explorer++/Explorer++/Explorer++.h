#ifndef EXPLORER_INCLUDED
#define EXPLORER_INCLUDED

#include "Explorer++_internal.h"
#include "BookmarkHelper.h"
#include "BookmarksToolbar.h"
#include "DrivesToolbar.h"
#include "TabContainer.h"
#include "../ShellBrowser/iShellView.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/Bookmark.h"
#import <msxml3.dll> raw_interfaces_only

#define MAX_TABS					100
#define MENU_BOOKMARK_STARTID		10000
#define MENU_BOOKMARK_ENDID			11000
#define MENU_HEADER_STARTID			12000
#define MENU_HEADER_ENDID			13000

#define TOOLBAR_START				5000
#define TABTOOLBAR_CLOSE			(TOOLBAR_START + 33)

/* wParam contains virtual code of key that was pressed,
lParam not currently used. */
#define CBN_KEYDOWN					WM_APP + 2

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED	WM_APP + 3

/* Private definitions. */
#define FROM_LISTVIEW				0
#define FROM_TREEVIEW				1
#define FROM_DRIVEBAR				3

/* Registry keys used to store program settings. */
#define REG_MAIN_KEY				_T("Software\\Explorer++")
#define REG_SETTINGS_KEY			_T("Software\\Explorer++\\Settings")

#define TAB_WINDOW_HEIGHT			24
#define DEFAULT_TREEVIEW_WIDTH		208

class Explorerplusplus : public IExplorerplusplus, public IFileContextMenuExternal
{
public:

	Explorerplusplus(HWND);
	~Explorerplusplus();

	LRESULT CALLBACK	WindowProcedure(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	ListViewSubclassProc(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	ListViewEditProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	EditSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	RebarSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TabBackingProc(HWND hTabCtrl,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TreeViewHolderProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TabSubclassProc(HWND hTab,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TreeViewSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TabProxyWndProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam,int iTabId);
	INT_PTR CALLBACK	GeneralSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	FilesFoldersProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	WindowProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	DefaultSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	TabSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	ApplicationButtonPropertiesProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	ApplicationToolbarNewButtonProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam);

	void				FolderSizeCallback(FolderSizeExtraInfo_t *pfsei,int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize);

	/* Directory modification. */
	static void			DirectoryAlteredCallback(TCHAR *szFileName,DWORD dwAction,void *pData);

private:

	static const COLORREF	CF_COMPRESSED = RGB(0,0,255);
	static const COLORREF	CF_ENCRYPTED = RGB(0,128,0);

	static const int		DEFAULT_LISTVIEW_HOVER_TIME = 500;

	static const int		MIN_SHELL_MENU_ID = 1;
	static const int		MAX_SHELL_MENU_ID = 1000;

	enum MousewheelSource_t
	{
		MOUSEWHEEL_SOURCE_LISTVIEW,
		MOUSEWHEEL_SOURCE_TREEVIEW,
		MOUSEWHEEL_SOURCE_OTHER
	};

	enum StartupMode_t
	{
		STARTUP_PREVIOUSTABS	= 1,
		STARTUP_DEFAULTFOLDER	= 2
	};

	enum InfoTipType_t
	{
		INFOTIP_SYSTEM	= 0,
		INFOTIP_CUSTOM	= 1
	};

	/* Describes the view modes and their order
	(as they differ on Windows XP and Vista/7). */
	struct ViewMode_t
	{
		UINT uViewMode;
	};

	struct ArrangeMenuItem_t
	{
		UINT SortById;
		UINT GroupById;
	};

	struct ToolbarButton_t
	{
		int iItemID;
	};

	struct TabInfo_t
	{
		BOOL	bLocked;
		BOOL	bAddressLocked;
		BOOL	bUseCustomName;
		TCHAR	szName[MAX_PATH];

		/* Although each tab manages its
		own columns, it does not know
		about any column defaults.
		Therefore, it makes more sense
		for this setting to remain here. */
		//BOOL	bUsingDefaultColumns;
	};

	struct TabProxyInfo_t
	{
		ATOM	atomClass;
		HWND	hProxy;
		int		iTabId;
	};

	struct TabPreviewInfo_t
	{
		int		iTabId;
		HBITMAP	hbm;
		POINT	ptOrigin;
	};

	struct FileContextMenuInfo_t
	{
		UINT uFrom;
	};

	struct DirectorySettings_t
	{
		LPITEMIDLIST				pidlDirectory;
		DirectorySettingsInternal_t	dsi;
	};

	class CLoadSaveRegistry : public ILoadSave
	{
	public:

		CLoadSaveRegistry(Explorerplusplus *pContainer);
		~CLoadSaveRegistry();

		/* Loading functions. */
		void	LoadGenericSettings(void);
		void	LoadBookmarks(void);
		int		LoadPreviousTabs(void);
		void	LoadDefaultColumns(void);
		void	LoadApplicationToolbar(void);
		void	LoadToolbarInformation(void);
		void	LoadColorRules(void);
		void	LoadState(void);

		/* Saving functions. */
		void	SaveGenericSettings(void);
		void	SaveBookmarks(void);
		void	SaveTabs(void);
		void	SaveDefaultColumns(void);
		void	SaveApplicationToolbar(void);
		void	SaveToolbarInformation(void);
		void	SaveColorRules(void);
		void	SaveState(void);

	private:

		Explorerplusplus *m_pContainer;
	};

	class CLoadSaveXML : public ILoadSave
	{
	public:

		CLoadSaveXML(Explorerplusplus *pContainer,BOOL bLoad);
		~CLoadSaveXML();

		void	InitializeLoadEnvironment(void);
		void	ReleaseLoadEnvironment(void);
		void	InitializeSaveEnvironment(void);
		void	ReleaseSaveEnvironment(void);

		/* Loading functions. */
		void	LoadGenericSettings(void);
		void	LoadBookmarks(void);
		int		LoadPreviousTabs(void);
		void	LoadDefaultColumns(void);
		void	LoadApplicationToolbar(void);
		void	LoadToolbarInformation(void);
		void	LoadColorRules(void);
		void	LoadState(void);

		/* Saving functions. */
		void	SaveGenericSettings(void);
		void	SaveBookmarks(void);
		void	SaveTabs(void);
		void	SaveDefaultColumns(void);
		void	SaveApplicationToolbar(void);
		void	SaveToolbarInformation(void);
		void	SaveColorRules(void);
		void	SaveState(void);

	private:

		Explorerplusplus		*m_pContainer;
		BOOL					m_bLoad;

		/* These are used for saving + loading. */
		MSXML2::IXMLDOMDocument	*m_pXMLDom;

		/* Used exclusively for loading. */
		BOOL					m_bLoadedCorrectly;

		/* Used exclusively for saving. */
		MSXML2::IXMLDOMElement	*m_pRoot;
	};

	friend CLoadSaveXML;

	class CApplicationToolbarDrop : public IDropTarget
	{
	public:
		CApplicationToolbarDrop(Explorerplusplus *pContainer);
		~CApplicationToolbarDrop();

		/* IUnknown methods. */
		HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
		ULONG __stdcall		AddRef(void);
		ULONG __stdcall		Release(void);

		/* Drag and drop. */
		HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyStat,POINTL pt,DWORD *pdwEffect);
		HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
		HRESULT _stdcall	DragLeave(void);
		HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect);

	private:
		int m_iRefCount;
		Explorerplusplus *m_pContainer;

		/* Drag and drop. */
		IDragSourceHelper *	m_pDragSourceHelper;
		IDropTargetHelper *	m_pDropTargetHelper;
		BOOL m_bAcceptData;

		HRESULT	InitializeDragDropHelpers(void);
	};

	friend CApplicationToolbarDrop;

	/* Internal private functions. */
	void					OnTabChangeInternal(BOOL bSetFocus);
	void					UpdateArrangeMenuItems(void);

	/* <----Private message handlers.----> */

	/* Main window private message handlers. */
	LRESULT CALLBACK		CommandHandler(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK		NotifyHandler(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
	BOOL					OnSize(int MainWindowWidth,int MainWindowHeight);
	int						OnClose(void);
	int						OnDestroy(void);
	void					OnSaveDirectoryListing(void);
	void					OnCopyItemPath(void);
	void					OnCopyUniversalPaths(void);
	void					OnCopy(BOOL bCopy);
	void					OnRightClick(NMHDR *nmhdr);
	void					OnTabSelectionChange(void);
	void					OnSetFileAttributes(void);
	void					OnShowFileProperties(void);
	void					OnWindowCreate(void);
	void					OnComboBoxKeyDown(WPARAM wParam);
	void					OnNewTab(void);
	void					OnFileRename(void);
	void					OnFileDelete(BOOL bPermanent);
	void					OnWildcardSelect(BOOL bSelect);
	void					OnSaveFileSlack(void);
	void					OnDirChanged(int iTabId);
	void					OnResolveLink(void);
	void					OnTabCtrlGetDispInfo(LPARAM lParam);
	void					OnDrawClipboard(void);
	void					OnChangeCBChain(WPARAM wParam,LPARAM lParam);
	void					OnSetFocus(void);
	int						HighlightSimilarFiles(HWND ListView);
	void					ShowHiddenFiles(void);
	HRESULT					OnDeviceChange(WPARAM wParam,LPARAM lParam);
	LRESULT					StatusBarMenuSelect(WPARAM wParam,LPARAM lParam);
	void					HandleDirectoryMonitoring(int iTabId);
	void					OnTbnDropDown(LPARAM lParam);
	void					OnTabMClick(WPARAM wParam,LPARAM lParam);
	void					OnDisplayWindowResized(WPARAM wParam);
	void					OnStartedBrowsing(int iTabId,TCHAR *szPath);
	void					OnDuplicateTab(int iTab);
	void					OnLockTab(int iTab);
	void					OnLockTabInternal(int iTab,int iTabId);
	void					OnLockTabAndAddress(int iTab);
	void					HandleTabToolbarItemStates(void);
	void					OnAutoSizeColumns(void);
	BOOL					OnMeasureItem(WPARAM wParam,LPARAM lParam);
	BOOL					OnDrawItem(WPARAM wParam,LPARAM lParam);
	void					OnToolbarViews(void);
	void					ShowToolbarViewsDropdown(void);
	void					OnMainToolbarRClick(void);
	void					OnApplicationToolbarRClick(void);
	void					OnAddressBarGo(void);
	void					OnSortByAscending(BOOL bSortAscending);
	void					OnPreviousWindow(void);
	void					OnNextWindow(void);
	HRESULT					BrowseFolder(const TCHAR *szPath,UINT wFlags);
	HRESULT					BrowseFolder(const TCHAR *szPath,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);
	HRESULT					BrowseFolder(LPITEMIDLIST pidlDirectory,UINT wFlags);
	HRESULT					BrowseFolder(LPITEMIDLIST pidlDirectory,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);
	int						DetermineListViewObjectIndex(HWND hListView);
	void					OnLockToolbars(void);
	void					LoadAllSettings(ILoadSave **pLoadSave);
	void					OnShellNewItemCreated(LPARAM lParam);
	void					OnCreateNewFolder(void);
	void					OnPaste(void);
	void					OnAppCommand(WPARAM wParam,LPARAM lParam);
	void					OnBrowseBack(void);
	void					OnBrowseForward(void);
	void					OnRefresh(void);
	void					OnDirectoryModified(int iTabId);
	void					OnIdaRClick(void);
	void					OnAssocChanged(void);
	void					OnNdwRClick(WPARAM wParam,LPARAM lParam);
	void					OnNdwIconRClick(WPARAM wParam,LPARAM lParam);
	LRESULT					OnCustomDraw(LPARAM lParam);
	void					OnSortBy(UINT uSortMode);
	void					OnGroupBy(UINT uSortMode);
	void					OnHome(void);
	void					OnNavigateUp(void);
	void					OnSelectTab(int iTab);
	void					OnSelectTab(int iTab,BOOL bSetFocus);

	/* ListView private message handlers. */
	void					OnListViewMButtonDown(WPARAM wParam,LPARAM lParam);
	void					OnListViewMButtonUp(WPARAM wParam,LPARAM lParam);
	LRESULT					OnListViewLButtonDown(WPARAM wParam,LPARAM lParam);
	void					OnListViewDoubleClick(NMHDR *nmhdr);
	void					OnListViewFileRename(void);
	void					OnListViewColumnClick(LPARAM lParam);
	void					OnListViewGetInfoTip(LPARAM lParam);
	LRESULT					OnListViewKeyDown(LPARAM lParam);
	void					OnListViewItemChanged(LPARAM lParam);
	HRESULT					OnListViewBeginDrag(LPARAM lParam,DragTypes_t DragType);
	BOOL					OnListViewBeginLabelEdit(LPARAM lParam);
	BOOL					OnListViewEndLabelEdit(LPARAM lParam);
	void					OnListViewGetDisplayInfo(LPARAM lParam);
	void					OnListViewFileDelete(BOOL bPermanent);
	void					OnListViewRClick(HWND hParent,POINT *pCursorPos);
	void					OnListViewBackgroundRClick(POINT *pCursorPos);
	void					OnListViewItemRClick(POINT *pCursorPos);
	void					OnListViewHeaderRClick(POINT *pCursorPos);
	int						GetColumnHeaderMenuList(unsigned int **pHeaderList);
	void					OnListViewShowFileProperties(void);
	void					OnListViewCopyItemPath(void);
	void					OnListViewCopyUniversalPaths(void);
	void					OnListViewSetFileAttributes(void);
	void					OnListViewPaste(void);

	/* TreeView private message handlers. */
	void					OnTreeViewFileRename(void);
	void					OnTreeViewFileDelete(BOOL bPermanent);
	void					OnTreeViewRightClick(WPARAM wParam,LPARAM lParam);
	void					OnTreeViewShowFileProperties(void);
	BOOL					OnTreeViewItemExpanding(LPARAM lParam);
	void					OnTreeViewSelChanged(LPARAM lParam);
	int						OnTreeViewBeginLabelEdit(LPARAM lParam);
	int						OnTreeViewEndLabelEdit(LPARAM lParam);
	LRESULT					OnTreeViewKeyDown(LPARAM lParam);
	void					OnTreeViewCopyItemPath(void);
	void					OnTreeViewCopy(BOOL bCopy);
	void					OnTreeViewSetFileAttributes(void);
	void					OnTreeViewCopyUniversalPaths(void);
	void					OnTreeViewPaste(void);

	/* Main toolbar private message handlers. */
	BOOL					OnTBQueryInsert(LPARAM lParam);
	BOOL					OnTBQueryDelete(LPARAM lParam);
	BOOL					OnTBGetButtonInfo(LPARAM lParam);
	BOOL					OnTBRestore(LPARAM lParam);
	void					OnTBReset(void);
	void					OnTBGetInfoTip(LPARAM lParam);

	/* Tab control private message handlers. */
	void					OnInitTabMenu(WPARAM wParam);
	void					OnTabCtrlLButtonDown(WPARAM wParam,LPARAM lParam);
	void					OnTabCtrlLButtonUp(void);
	void					OnTabCtrlMouseMove(WPARAM wParam,LPARAM lParam);
	void					OnTabCtrlRButtonUp(WPARAM wParam,LPARAM lParam);
	void					ProcessTabCommand(UINT uMenuID,int iTabHit);

	/* Address bar private message handlers. */
	void					OnAddressBarBeginDrag(void);

	/* Holder window private message handlers. */
	LRESULT CALLBACK		TreeViewHolderWindowCommandHandler(WPARAM wParam);
	LRESULT CALLBACK		TreeViewHolderWindowNotifyHandler(LPARAM lParam);
	void					OnTreeViewHolderWindowTimer(void);

	/* Tabs. */
	void					SelectAdjacentTab(BOOL bNextTab);
	HRESULT					CreateNewTab(TCHAR *TabDirectory,InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,int *pTabObjectIndex);
	HRESULT					CreateNewTab(LPITEMIDLIST pidlDirectory,InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,int *pTabObjectIndex);
	void					InsertNewTab(LPITEMIDLIST pidlDirectory,int iNewTabIndex,int iTabId);
	bool					CloseTab(int TabIndex);
	void					RemoveTabFromControl(int iTab);
	bool					OnCloseTab(void);
	HRESULT					RestoreTabs(ILoadSave *pLoadSave);
	void					RefreshTab(int iTabId);
	void					RefreshAllTabs(void);
	void					CloseOtherTabs(int iTab);
	int						GetCurrentTabId();

	/* Clone Window. */
	void					OnCloneWindow(void);

	/* Menus. */
	HMENU					InitializeRightClickMenu(void);
	void					SetProgramMenuItemStates(HMENU hProgramMenu);
	void					SetArrangeMenuItemStates(HMENU);

	/* Columns. */
	void					SetAllDefaultColumns(void);
	void					SetDefaultRealFolderColumns(std::list<Column_t> *pColumns);
	void					SetDefaultControlPanelColumns(std::list<Column_t> *pColumns);
	void					SetDefaultMyComputerColumns(std::list<Column_t> *pColumns);
	void					SetDefaultRecycleBinColumns(std::list<Column_t> *pColumns);
	void					SetDefaultPrintersColumns(std::list<Column_t> *pColumns);
	void					SetDefaultNetworkConnectionsColumns(std::list<Column_t> *pColumns);
	void					SetDefaultMyNetworkPlacesColumns(std::list<Column_t> *pColumns);

	/* Application toolbar. */
	void					InitializeApplicationToolbar(void);
	void					ApplicationToolbarNewButton(void);
	ApplicationButton_t		*ApplicationToolbarAddItem(TCHAR *szName,TCHAR *szCommand,BOOL bShowNameOnToolbar);
	void					ApplicationToolbarAddButtonsToToolbar(void);
	void					ApplicationToolbarAddButtonToToolbar(ApplicationButton_t *pab);
	void					ApplicationToolbarRefreshButton(int iItem);
	void					ApplicationToolbarOpenItem(int iItem,TCHAR *szParameters);
	void					ApplicationToolbarDeleteItem(int iItem);
	void					ApplicationToolbarShowItemProperties(int iItem);

	/* Customize colors. */
	std::vector<ColorRule_t>	m_ColorRuleList;

	/* Application button properties dialog. */
	void					OnApplicationButtonPropertiesInit(HWND hDlg);
	void					OnApplicationButtonPropertiesOk(HWND hDlg);

	/* New application button dialog. */
	void					OnApplicationToolbarNewButtonInit(HWND hDlg);
	void					OnApplicationToolbarNewButtonOk(HWND hDlg);

	/* Shared between application dialogs. */
	void					OnApplicationToolbarCommandButton(HWND hDlg);

	/* File infotips. */
	void					CreateFileInfoTip(int iItem,TCHAR *szInfoTip,UINT cchMax);

	/* Control creation. */
	HWND					CreateMainListView(HWND hParent,DWORD Style);
	void					CreateMainControls(void);
	void					CreateFolderControls(void);
	void					CreateMainToolbar(void);
	void					CreateAddressBar(void);
	void					CreateBookmarksToolbar(void);
	void					CreateDrivesToolbar(void);
	void					CreateApplicationToolbar(void);
	HWND					CreateTabToolbar(HWND hParent,int idCommand,TCHAR *szTip);
	void					CreateTabBacking(void);

	/* Main toolbars. */
	void					InitializeMainToolbars(void);
	void					AdjustMainToolbarSize(void);

	/* Directory specific settings. */
	void					SaveDirectorySpecificSettings(int iTab);
	void					SetDirectorySpecificSettings(int iTab,LPITEMIDLIST pidlDirectory);

	/* Settings. */
	void					SaveAllSettings(void);
	LONG					SaveSettings();
	LONG					LoadSettings(LPCTSTR);
	void					ValidateLoadedSettings(void);
	void					ValidateToolbarSettings(void);
	void					ValidateColumns(void);
	void					ValidateSingleColumnSet(int iColumnSet,std::list<Column_t> *pColumnList);
	void					ApplyLoadedSettings(void);
	void					ApplyToolbarSettings(void);
	void					AddStyleToToolbar(UINT *fStyle,UINT fStyleToAdd);
	void					SetDefaultValues(void);
	void					TestConfigFile(void);
	void					SaveTabSettingsToRegistry(void);
	int						LoadTabSettingsFromRegistry(void);
	void					LoadColumnFromRegistry(HKEY hColumnsKey,TCHAR *szKeyName,std::list<Column_t> *pColumns);
	void					SaveColumnToRegistry(HKEY hColumnsKey,TCHAR *szKeyName,std::list<Column_t> *pColumns);
	void					LoadColumnWidthsFromRegistry(HKEY hColumnsKey,TCHAR *szKeyName,std::list<Column_t> *pColumns);
	void					SaveColumnWidthsToRegistry(HKEY hColumnsKey,TCHAR *szKeyName,std::list<Column_t> *pColumns);
	void					LoadDefaultColumnsFromRegistry(void);
	void					SaveDefaultColumnsToRegistry(void);
	void					InitializeBookmarks(void);
	void					SaveBookmarksToRegistry(void);
	void					LoadBookmarksFromRegistry(void);
	void					LoadApplicationToolbarFromRegistry(void);
	void					LoadApplicationToolbarFromRegistryInternal(HKEY hKey);
	void					SaveApplicationToolbarToRegistry(void);
	void					LoadColorRulesFromRegistry(void);
	void					LoadColorRulesFromRegistryInternal(HKEY hKey);
	void					SaveColorRulesToRegistry(void);
	void					SaveColorRulesToRegistryInternal(HKEY hKey,ColorRule_t *pColorRule,int iCount);
	void					SaveApplicationToolbarToRegistryInternal(HKEY hKey,ApplicationButton_t	*pab,int count);
	void					SaveToolbarInformationToRegistry(void);
	void					LoadToolbarInformationFromRegistry(void);
	void					SaveStateToRegistry(void);
	void					LoadStateFromRegistry(void);

	/* Window state update. */
	void					UpdateWindowStates(void);
	void					HandleMainWindowText(void);
	void					HandleComboBoxText(void);
	void					HandleTabText(void);
	void					HandleTabText(int iTabId);
	void					HandleTabText(int iTab,int iTabId);
	void					SetTabIcon(void);
	void					SetTabIcon(int iTabId);
	void					SetTabIcon(int iIndex,int iTabId);
	void					SetTabIcon(int iIndex,int iTabId,LPITEMIDLIST pidlDirectory);
	void					HandleTreeViewSelection(void);
	void					SetStatusBarParts(int width);
	void					ResizeWindows(void);
	void					SetListViewInitialPosition(HWND hListView);
	void					AdjustFolderPanePosition(void);
	void					SetComboBoxExTitleString(HWND CbEx,LPITEMIDLIST pidl,TCHAR *szDisplayText);
	void					HandleToolbarItemStates(void);
	HRESULT					HandleStatusText(void);
	void					ToggleFolders(void);

	/* Windows 7 taskbar thumbnail previews. */
	ATOM					RegisterTabProxyClass(TCHAR *szClassName,LPITEMIDLIST pidlDirectory);
	void					CreateTabProxy(LPITEMIDLIST pidlDirectory,int iTabId,BOOL bSwitchToNewTab);
	void					RegisterTab(HWND hTabProxy,TCHAR *szDisplayName,BOOL bTabActive);
	HBITMAP					CaptureTabScreenshot(int iTabId);
	void					GetTabLivePreviewBitmap(int iTabId,TabPreviewInfo_t *ptpi);

	/* Windows 7 jumplist tasks. */
	void					SetupJumplistTasks();

	/* Tab ID's. */
	void					InitializeTabMap(void);
	void					ReleaseTabId(int iTabId);
	int						GenerateUniqueTabId(void);
	BOOL					CheckTabIdStatus(int iTabId);

	/* Languages. */
	void					SetLanguageModule(void);

	/* Default settings. */
	void					SetDefaultTabSettings(TabInfo_t *pTabInfo);

	/* Color rules. */
	void					InitializeColorRules(void);

	/* Arrange menu. */
	void					InitializeArrangeMenuItems(void);
	void					SetActiveArrangeMenuItems(void);
	int						InsertArrangeMenuItems(HMENU hMenu);
	UINT					GetArrangeMenuItemStringIndex(UINT uItemId);
	void					DeletePreviousArrangeMenuItems(void);
	int						DetermineSortModeMenuId(int SortMode);
	int						DetermineGroupModeMenuId(int SortMode);

	/* File operations. */
	void					CopyToFolder(BOOL bMove);
	void					GotoFolder(int FolderCSIDL);
	void					OpenAllSelectedItems(BOOL bOpenInNewTab);
	void					OpenListViewItem(int iItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenItem(TCHAR *szItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenItem(LPITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenFolderItem(LPITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenFileItem(LPITEMIDLIST pidlItem,TCHAR *szParameters);
	HRESULT					OnListViewCopy(BOOL bCopy);
	HRESULT					ProcessShellMenuCommand(IContextMenu *pContextMenu,UINT CmdIDOffset,UINT iStartOffset);
	HRESULT					ShowMultipleFileProperties(LPITEMIDLIST pidlDirectory,LPCITEMIDLIST *ppidl,int nFiles);
	HRESULT					ExecuteActionFromContextMenu(LPITEMIDLIST pidlDirectory,LPCITEMIDLIST *ppidl,int nFiles,TCHAR *szAction,DWORD fMask);

	/* File context menu. */
	void					AddMenuEntries(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu);
	BOOL					HandleShellMenuItem(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,TCHAR *szCmd);
	void					HandleCustomMenuItem(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,int iCmd);

	/* Listview selection file tests. */
	void					BuildListViewFileSelectionList(HWND hListView,std::list<std::wstring> *pFileSelectionList);
	HRESULT					TestListViewSelectionAttributes(SFGAOF *pItemAttributes);
	HRESULT					TestTreeViewSelectionAttributes(SFGAOF *pItemAttributes);	
	HRESULT					GetSelectionAttributes(SFGAOF *pItemAttributes);
	BOOL					IsRenamePossible(void);
	BOOL					IsDeletionPossible(void);
	BOOL					CanShowFileProperties(void);
	BOOL					CanCutOrCopySelection(void);
	BOOL					AreAllSelectedFilesReal(void);
	BOOL					CanPaste(void);

	/* Tabs. */
	void					DuplicateTab(int iTabInternal);

	/* Tab proxy's. */
	void					SetTabProxyIcon(int iTabId,HICON hIcon);

	/* Display window file information. */
	void					HandleFileSelectionDisplay(void);
	void					HandleFileSelectionDisplayZero(void);
	void					HandleFileSelectionDisplayOne(void);
	void					HandleFileSelectionDisplayMore(void);
	void					FormatDisplayString(TCHAR *szDisplayRaw,int iSelected,TCHAR *szFullFileName,TCHAR *szDisplayFinal,UINT cchMax);
	void					TranslateDisplayWindowBuffer(TCHAR *szSymbol,UINT cchMax,int iSelected,TCHAR *szFullFileName);

	/* Columns. */
	void					OnSelectColumns();
	int						LookupColumnNameStringIndex(int iColumnId);
	int						LookupColumnDescriptionStringIndex(int iColumnId);
	void					CopyColumnInfoToClipboard(void);

	/* Bookmark handling. */
	HRESULT					ExpandAndBrowsePath(TCHAR *szPath);
	HRESULT					ExpandAndBrowsePath(TCHAR *szPath,BOOL bOpenInNewTab,BOOL bSwitchToNewTab);

	/* Filtering. */
	void					SetFilterStatus(void);

	/* Default settings dialog. */
	void					OnDefaultSettingsNewTabDir(HWND hDlg);
	void					DefaultSettingsSetNewTabDir(HWND hEdit,LPITEMIDLIST pidl);
	void					DefaultSettingsSetNewTabDir(HWND hEdit,TCHAR *szPath);

	/* Files and folders dialog. */
	void					SetInfoTipWindowStates(HWND hDlg);
	void					SetFolderSizeWindowState(HWND hDlg);

	/* XML Settings. */
	void					LoadGenericSettingsFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					SaveGenericSettingsToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	int						LoadTabSettingsFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					SaveTabSettingsToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	void					SaveTabSettingsToXMLnternal(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe);
	int						LoadColumnFromXML(MSXML2::IXMLDOMNode *pNode,std::list<Column_t> *pColumns);
	void					SaveColumnToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pColumnsNode,std::list<Column_t> *pColumns,TCHAR *szColumnSet,int iIndent);
	int						LoadBookmarksFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					SaveBookmarksToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	int						LoadDefaultColumnsFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					SaveDefaultColumnsToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	void					SaveDefaultColumnsToXMLInternal(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pColumnsNode);
	void					SaveWindowPositionToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	void					SaveWindowPositionToXMLInternal(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pWndPosNode);
	void					LoadApplicationToolbarFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					LoadApplicationToolbarFromXMLInternal(MSXML2::IXMLDOMNode *pNode);
	void					SaveApplicationToolbarToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	void					SaveApplicationToolbarToXMLInternal(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe,ApplicationButton_t *pab);
	void					LoadColorRulesFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					LoadColorRulesFromXMLInternal(MSXML2::IXMLDOMNode *pNode);
	void					SaveColorRulesToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	void					SaveColorRulesToXMLInternal(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe,const ColorRule_t &ColorRule);
	void					LoadToolbarInformationFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					SaveToolbarInformationToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	void					SaveToolbarInformationToXMLnternal(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe);
	void					LoadStateFromXML(MSXML2::IXMLDOMDocument *pXMLDom);
	void					SaveStateToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot);
	void					MapAttributeToValue(MSXML2::IXMLDOMNode *pNode,WCHAR *wszName,WCHAR *wszValue);
	void					MapTabAttributeValue(WCHAR *wszName,WCHAR *wszValue,InitialSettings_t *pSettings,TabInfo_t *pTabInfo);

	/* Tabs. */
	std::wstring			GetTabName(int iTab);
	void					SetTabName(int iTab,std::wstring strName,BOOL bUseCustomName);
	void					SetTabSelection(int Index);

	/* IExplorerplusplus methods. */
	HWND					GetActiveListView();
	IShellBrowser2			*GetActiveShellBrowser();

	/* Miscellaneous. */
	BOOL					CompareVirtualFolders(UINT uFolderCSIDL);
	BOOL					CompareVirtualFolders(TCHAR *szDirectory,UINT uFolderCSIDL);
	void					OnShowOptions(void);
	void					AddLanguages(HWND hDlg);
	WORD					AddLanguageToComboBox(HWND hComboBox,TCHAR *szImageDirectory,TCHAR *szFileName);
	int						GetLanguageIDFromIndex(HWND hDlg,int iIndex);
	void					PushGlobalSettingsToTab(int iTabId);
	void					PushGlobalSettingsToAllTabs(void);
	void					CreateViewsMenu(POINT *ptOrigin);
	void					SetMenuItemBitmap(HMENU hMenu,UINT ItemID,int iBitmap);
	void					SetMenuOwnerDraw(HMENU hMenu);
	void					SetMenuOwnerDrawInternal(HMENU hMenu,int nMenus);
	void					SetMenuItemOwnerDrawn(HMENU hMenu,int iItem);
	void					SetInitialToolbarButtons(void);
	int						LookupToolbarButtonTextID(int iButtonID);
	int						LookupToolbarButtonImage(int iButtonID);
	BYTE					LookupToolbarButtonExtraStyles(int iButtonID);
	void					InsertToolbarButtons(void);
	void					AddStringsToMainToolbar(void);
	void					CreateStatusBar(void);
	void					InitializeDisplayWindow(void);
	void					InitializeTabs(void);
	void					AddDefaultTabIcons(HIMAGELIST himlTab);
	void					InitializeMenus(void);
	void					SetGoMenuName(HMENU hMenu,UINT uMenuID,UINT csidl);
	int						CreateDriveFreeSpaceString(TCHAR *szPath,TCHAR *szBuffer,int nBuffer);
	BOOL					CheckItemSelection(void);
	HWND					MyGetNextWindow(HWND hwndCurrent);
	BOOL					IsNextWindowVisible(HWND hNext);
	void					ShowMainRebarBand(HWND hwnd,BOOL bShow);
	BOOL					OnMouseWheel(MousewheelSource_t MousewheelSource,WPARAM wParam,LPARAM lParam);
	void					CycleViewState(BOOL bCycleForward);
	int						GetViewModeMenuId(UINT uViewMode);
	int						GetViewModeMenuStringId(UINT uViewMode);
	BOOL					VerifyLanguageVersion(TCHAR *szLanguageModule);
	HMENU					CreateRebarHistoryMenu(BOOL bBack);
	void					PlayNavigationSound(void);
	CStatusBar				*GetStatusBar();
	UINT					GetDefaultSortMode(const LPITEMIDLIST &pidlDirectory);
	unsigned int			DetermineColumnSortMode(UINT uColumnId);




	/* ------ Internal state. ------ */

	HWND					m_hContainer;
	HWND					m_hStatusBar;
	HWND					m_hMainRebar;
	HWND					m_hDisplayWindow;
	HWND					m_hTabCtrl;
	HWND					m_hTabWindowToolbar;
	HWND					m_hTreeView;
	HWND					m_hHolder;
	HWND					m_hAddressBar;
	HWND					m_hMainToolbar;
	HWND					m_hListView[MAX_TABS];
	HWND					m_hFoldersToolbar;
	HWND					m_hTabBacking;
	HWND					m_hBookmarksToolbar;
	HWND					m_hDrivesToolbar;
	HWND					m_hApplicationToolbar;

	MyIFolderView2 *		m_pFolderView[MAX_TABS];
	IShellBrowser2 *		m_pShellBrowser[MAX_TABS];
	IShellBrowser2 *		m_pActiveShellBrowser;
	IDirectoryMonitor *		m_pDirMon;
	CMyTreeView *			m_pMyTreeView;
	CCustomMenu *			m_pCustomMenu;
	CStatusBar *			m_pStatusBar;
	HANDLE					m_hIconThread;
	HANDLE					m_hTreeViewIconThread;
	HANDLE					m_hFolderSizeThread;

	/** Internal state. **/
	HWND					m_hActiveListView;
	HWND					m_hLastActiveWindow;
	HWND					m_hNextClipboardViewer;
	LPITEMIDLIST			m_pidlDirectory;
	HMENU					m_hArrangeSubMenu;
	HMENU					m_hGroupBySubMenu;
	HMENU					m_hArrangeSubMenuRClick;
	HMENU					m_hGroupBySubMenuRClick;
	HMENU					m_hTabRightClickMenu;
	HMENU					m_hToolbarRightClickMenu;
	HMENU					m_hApplicationRightClickMenu;
	HMENU					m_hDisplayWindowRightClickMenu;
	HMENU					m_hViewsMenu;
	TCHAR					m_CurrentDirectory[MAX_PATH];
	TCHAR					m_DefaultTabDirectoryStatic[MAX_PATH];
	TCHAR					m_DefaultTabDirectory[MAX_PATH];
	TCHAR					m_OldTreeViewFileName[MAX_PATH];
	DWORD					m_Language;
	DWORD					m_dwMajorVersion;
	DWORD					m_dwMinorVersion;
	LONG					m_DisplayWindowHeight;
	BOOL					m_bTreeViewRightClick;
	BOOL					m_bSelectingTreeViewDirectory;
	BOOL					m_bAttemptToolbarRestore;
	BOOL					m_bLanguageLoaded;
	BOOL					m_bTreeViewOpenInNewTab;
	unsigned int			m_TreeViewWidth;
	int						m_iObjectIndex;
	int						m_iMaxArrangeMenuItem;
	int						m_iLastSelectedTab;
	int						m_iTabSelectedItem;
	int						m_ListViewEditingStage;

	std::list<ViewMode_t>	m_ViewModes;

	/* Initialization. */
	BOOL					m_bLoadSettingsFromXML;
	Gdiplus::Color			m_DisplayWindowCentreColor;
	Gdiplus::Color			m_DisplayWindowSurroundColor;
	COLORREF				m_DisplayWindowTextColor;
	HFONT					m_DisplayWindowFont;

	/* Internal tab state. */
	TabInfo_t				m_TabInfo[MAX_TABS];
	UINT					m_uTabMap[MAX_TABS];

	/* Directory-specific settings. */
	std::list<DirectorySettings_t>	m_DirectorySettingsList;

	/* User options variables. */
	BOOL					m_bShowStatusBar;
	BOOL					m_bShowFolders;
	BOOL					m_bShowAddressBar;
	BOOL					m_bShowDisplayWindow;
	BOOL					m_bShowMainToolbar;
	BOOL					m_bShowBookmarksToolbar;
	BOOL					m_bShowDrivesToolbar;
	BOOL					m_bShowApplicationToolbar;
	BOOL					m_bAlwaysShowTabBar;
	BOOL					m_bShowTabBar;
	BOOL					m_bShowFullTitlePath;
	BOOL					m_bAlwaysOpenNewTab;
	BOOL					m_bShowFolderSizes;
	BOOL					m_bDisableFolderSizesNetworkRemovable;
	BOOL					m_bUnlockFolders;
	BOOL					m_bOpenNewTabNextToCurrent;
	BOOL					m_bConfirmCloseTabs;
	BOOL					m_bTreeViewDelayEnabled;
	BOOL					m_bSavePreferencesToXMLFile;
	BOOL					m_bLockToolbars;
	BOOL					m_bExtendTabControl;
	BOOL					m_bUseFullRowSelect;
	BOOL					m_bShowUserNameInTitleBar;
	BOOL					m_bShowPrivilegeLevelInTitleBar;
	BOOL					m_bShowFilePreviews;
	BOOL					m_bOneClickActivate;
	UINT					m_OneClickActivateHoverTime;
	BOOL					m_bAllowMultipleInstances;
	BOOL					m_bForceSameTabWidth;
	BOOL					m_bDoubleClickTabClose;
	BOOL					m_bHandleZipFiles;
	BOOL					m_bInsertSorted;
	BOOL					m_bOverwriteExistingFilesConfirmation;
	BOOL					m_bCheckBoxSelection;
	BOOL					m_bForceSize;
	BOOL					m_bCloseMainWindowOnTabClose;
	BOOL					m_bShowTabBarAtBottom;
	BOOL					m_bShowTaskbarThumbnails;
	BOOL					m_bShowTaskbarThumbnailsProvisional;
	BOOL					m_bSynchronizeTreeview;
	BOOL					m_bTVAutoExpandSelected;
	BOOL					m_bLargeToolbarIcons;
	BOOL					m_bPlayNavigationSound;
	SizeDisplayFormat_t		m_SizeDisplayFormat;
	StartupMode_t			m_StartupMode;
	NDefaultFileManager::ReplaceExplorerModes_t	m_ReplaceExplorerMode;

	/* Infotips (user options). */
	BOOL					m_bShowInfoTips;
	InfoTipType_t			m_InfoTipType;

	/* Global options. */
	DWORD					m_ViewModeGlobal;
	BOOL					m_bShowHiddenGlobal;
	BOOL					m_bShowExtensionsGlobal;
	BOOL					m_bShowInGroupsGlobal;
	BOOL					m_bAutoArrangeGlobal;
	BOOL					m_bSortAscendingGlobal;
	BOOL					m_bShowFriendlyDatesGlobal;
	BOOL					m_bShowGridlinesGlobal;
	BOOL					m_bHideSystemFilesGlobal;
	BOOL					m_bHideLinkExtensionGlobal;
	
	/* Windows 7 taskbar thumbnail previews. */
	DwmInvalidateIconicBitmapsProc	DwmInvalidateIconicBitmaps;
	HMODULE					m_hDwmapi;
	ITaskbarList4			*m_pTaskbarList3;
	std::list<TabProxyInfo_t>	m_TabProxyList;
	UINT					m_uTaskbarButtonCreatedMessage;
	BOOL					m_bTaskbarInitialised;

	/* Tabs. */
	CTabContainer			*m_pTabContainer;

	/* Bookmarks. */
	CBookmarkFolder *		m_bfAllBookmarks;
	GUID					m_guidBookmarksToolbar;
	GUID					m_guidBookmarksMenu;
	CBookmarksToolbar		*m_pBookmarksToolbar;
	CIPBookmarkItemNotifier	*m_pipbin;

	/* Undo support. */
	CFileActionHandler		m_FileActionHandler;

	/* Main toolbars. */
	REBARBANDINFO			m_ToolbarInformation[NUM_MAIN_TOOLBARS];

	/* Main toolbar. */
	HIMAGELIST				m_himlToolbarSmall;
	HIMAGELIST				m_himlToolbarLarge;

	/* Toolbar buttons. */
	std::list<ToolbarButton_t>	m_tbInitial;

	/* Drives toolbar. */
	CDrivesToolbar			*m_pDrivesToolbar;

	/* Application toolbar. */
	ApplicationButton_t		*m_pAppButtons;
	ApplicationButton_t		*m_pAppButtonSelected;
	int						m_nAppButtons;
	int						m_iAppIdOffset;
	int						m_iSelectedRClick;

	/* Display window folder sizes. */
	std::list<DWFolderSize_t>	m_DWFolderSizes;
	int						m_iDWFolderSizeUniqueId;

	/* Default columns. */
	std::list<Column_t>		m_RealFolderColumnList;
	std::list<Column_t>		m_MyComputerColumnList;
	std::list<Column_t>		m_ControlPanelColumnList;
	std::list<Column_t>		m_RecycleBinColumnList;
	std::list<Column_t>		m_PrintersColumnList;
	std::list<Column_t>		m_NetworkConnectionsColumnList;
	std::list<Column_t>		m_MyNetworkPlacesColumnList;

	/* ListView selection. */
	BOOL					m_bCountingUp;
	BOOL					m_bCountingDown;
	BOOL					m_bInverted;
	BOOL					m_bSelectionFromNowhere;
	int						m_nSelected;
	int						m_nSelectedOnInvert;
	int						m_ListViewMButtonItem;

	/* Copy/cut. */
	IDataObject				*m_pClipboardDataObject;
	HTREEITEM				m_hCutTreeViewItem;

	/* Drag and drop. */
	BOOL					m_bDragging;
	BOOL					m_bDragCancelled;
	BOOL					m_bDragAllowed;

	/* Rename support. */
	BOOL					m_bListViewBeginRename;
	BOOL					m_bListViewRenaming;
	int						m_iItemEditing;

	/* Tab handler data. */
	std::vector<int>		m_TabSelectionHistory;
	int						m_iPreviousTabSelectionId;
	BOOL					m_bTabBeenDragged;
	RECT					m_rcDraggedTab;
	int						m_iTabMenuItem;
	
	/* Cut items data. */
	std::list<std::wstring>	m_CutFileNameList;
	int						m_iCutTabInternal;

	/* Arrange menu related data. */
	std::list<ArrangeMenuItem_t>	m_ArrangeList;
	std::list<ArrangeMenuItem_t>	*m_pActiveArrangeMenuItems;
	std::list<ArrangeMenuItem_t>	m_ArrangeMenuRealFolder;
	std::list<ArrangeMenuItem_t>	m_ArrangeMenuMyComputer;
	std::list<ArrangeMenuItem_t>	m_ArrangeMenuControlPanel;
	std::list<ArrangeMenuItem_t>	m_ArrangeMenuRecycleBin;

	/* Mousewheel. */
	int						m_zDeltaTotal;

	/* TreeView middle click. */
	HTREEITEM				m_hTVMButtonItem;

	BOOL					m_bBlockNext;
};

struct TabProxy_t
{
	Explorerplusplus	*pContainer;
	int			iTabId;
};

#endif