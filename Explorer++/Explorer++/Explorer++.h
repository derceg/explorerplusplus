// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationToolbar.h"
#include "BookmarkHelper.h"
#include "BookmarksToolbar.h"
#include "ColorRuleHelper.h"
#include "Config.h"
#include "DrivesToolbar.h"
#include "Explorer++_internal.h"
#include "MainToolbar.h"
#include "ShellBrowser/iShellView.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "TabContainer.h"
#include "TabInterface.h"
#include "../Helper/Bookmark.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/ImageWrappers.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../MyTreeView/MyTreeView.h"
#include <boost/optional.hpp>
#include <MsXml2.h>
#include <objbase.h>
#include <unordered_map>

#define MENU_BOOKMARK_STARTID		10000
#define MENU_BOOKMARK_ENDID			11000
#define MENU_HEADER_STARTID			12000
#define MENU_HEADER_ENDID			13000

#define TOOLBAR_START				5000
#define TABTOOLBAR_CLOSE			(TOOLBAR_START + 33)

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED	WM_APP + 3

/* Private definitions. */
#define FROM_LISTVIEW				0
#define FROM_TREEVIEW				1

class Explorerplusplus : public IExplorerplusplus, public TabInterface, public IFileContextMenuExternal
{
	friend LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	friend void FolderSizeCallbackStub(int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize,LPVOID pData);

public:

	Explorerplusplus(HWND);
	~Explorerplusplus();

	LRESULT CALLBACK	ListViewSubclassProc(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);

	/* Address bar edit control. */
	LRESULT CALLBACK	EditSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

	LRESULT CALLBACK	RebarSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TabBackingProc(HWND hTabCtrl,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TabSubclassProc(HWND hTab,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TabProxyWndProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam,int iTabId);

	LRESULT CALLBACK	TreeViewHolderProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TreeViewSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	/* Options dialog. */
	INT_PTR CALLBACK	GeneralSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	FilesFoldersProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	WindowProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	DefaultSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	TabSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	LRESULT CALLBACK	MainWndTaskbarThumbnailProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	/* Directory modification. */
	static void			DirectoryAlteredCallback(const TCHAR *szFileName,DWORD dwAction,void *pData);

private:

	static const int		MIN_SHELL_MENU_ID = 1;
	static const int		MAX_SHELL_MENU_ID = 1000;

	static const UINT		MINIMUM_DISPLAYWINDOW_HEIGHT = 70;
	static const UINT		DEFAULT_DISPLAYWINDOW_HEIGHT = 90;

	static const UINT		DEFAULT_TREEVIEW_WIDTH = 208;

	static const UINT		TAB_WINDOW_HEIGHT = 24;

	/* The number of toolbars that appear in the
	main rebar. */
	static const int NUM_MAIN_TOOLBARS = 5;

	/* Main toolbar id's. */
	static const int ID_MAINTOOLBAR = 0;
	static const int ID_ADDRESSTOOLBAR = 1;
	static const int ID_BOOKMARKSTOOLBAR = 2;
	static const int ID_DRIVESTOOLBAR = 3;
	static const int ID_APPLICATIONSTOOLBAR = 4;

	static const std::vector<std::wstring> BLACKLISTED_BACKGROUND_MENU_CLSID_ENTRIES;

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

	struct ArrangeMenuItem_t
	{
		UINT SortById;
		UINT GroupById;
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

	/* Used to store settings for individual directories. */
	struct DirectorySettingsInternal_t
	{
		SortMode	sortMode;
		ViewMode	viewMode;

		std::list<Column_t>	RealFolderColumnList;
		std::list<Column_t>	MyComputerColumnList;
		std::list<Column_t>	ControlPanelColumnList;
		std::list<Column_t>	RecycleBinColumnList;
		std::list<Column_t>	PrintersColumnList;
		std::list<Column_t>	NetworkConnectionsColumnList;
		std::list<Column_t>	MyNetworkPlacesColumnList;
	};

	struct DirectorySettings_t
	{
		LPITEMIDLIST				pidlDirectory;
		DirectorySettingsInternal_t	dsi;
	};

	struct DirectoryAltered_t
	{
		int		iIndex;
		int		iFolderIndex;
		void	*pData;
	};

	struct DWFolderSizeCompletion_t
	{
		ULARGE_INTEGER	liFolderSize;
		int				uId;
		int				iTabId;
	};

	struct DWFolderSize_t
	{
		int	uId;
		int	iTabId;
		BOOL bValid;
	};

	struct FolderSizeExtraInfo_t
	{
		void	*pContainer;
		int		uId;
	};

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
		virtual void	LoadDialogStates() = 0;

		/* Saving functions. */
		virtual void	SaveGenericSettings() = 0;
		virtual void	SaveBookmarks() = 0;
		virtual void	SaveTabs() = 0;
		virtual void	SaveDefaultColumns() = 0;
		virtual void	SaveApplicationToolbar() = 0;
		virtual void	SaveToolbarInformation() = 0;
		virtual void	SaveColorRules() = 0;
		virtual void	SaveDialogStates() = 0;
	};

	class CLoadSaveRegistry : public ILoadSave
	{
	public:

		CLoadSaveRegistry(Explorerplusplus *pContainer);
		~CLoadSaveRegistry();

		/* Loading functions. */
		void	LoadGenericSettings();
		void	LoadBookmarks();
		int		LoadPreviousTabs();
		void	LoadDefaultColumns();
		void	LoadApplicationToolbar();
		void	LoadToolbarInformation();
		void	LoadColorRules();
		void	LoadDialogStates();

		/* Saving functions. */
		void	SaveGenericSettings();
		void	SaveBookmarks();
		void	SaveTabs();
		void	SaveDefaultColumns();
		void	SaveApplicationToolbar();
		void	SaveToolbarInformation();
		void	SaveColorRules();
		void	SaveDialogStates();

	private:

		Explorerplusplus *m_pContainer;
	};

	class CLoadSaveXML : public ILoadSave
	{
	public:

		CLoadSaveXML(Explorerplusplus *pContainer,BOOL bLoad);
		~CLoadSaveXML();

		/* Loading functions. */
		void	LoadGenericSettings();
		void	LoadBookmarks();
		int		LoadPreviousTabs();
		void	LoadDefaultColumns();
		void	LoadApplicationToolbar();
		void	LoadToolbarInformation();
		void	LoadColorRules();
		void	LoadDialogStates();

		/* Saving functions. */
		void	SaveGenericSettings();
		void	SaveBookmarks();
		void	SaveTabs();
		void	SaveDefaultColumns();
		void	SaveApplicationToolbar();
		void	SaveToolbarInformation();
		void	SaveColorRules();
		void	SaveDialogStates();

	private:

		void	InitializeLoadEnvironment();
		void	ReleaseLoadEnvironment();
		void	InitializeSaveEnvironment();
		void	ReleaseSaveEnvironment();

		Explorerplusplus		*m_pContainer;
		BOOL					m_bLoad;

		/* These are used for saving + loading. */
		IXMLDOMDocument	*m_pXMLDom;

		/* Used exclusively for loading. */
		BOOL					m_bLoadedCorrectly;

		/* Used exclusively for saving. */
		IXMLDOMElement	*m_pRoot;
	};

	friend CLoadSaveXML;

	LRESULT CALLBACK		WindowProcedure(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	/* Internal private functions. */
	void					OnTabChangeInternal(BOOL bSetFocus);
	void					UpdateArrangeMenuItems(void);

	/* <----Private message handlers.----> */

	/* Main window private message handlers. */
	LRESULT CALLBACK		CommandHandler(HWND hwnd, WPARAM wParam);
	LRESULT CALLBACK		NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	BOOL					OnSize(int MainWindowWidth,int MainWindowHeight);
	int						OnClose(void);
	int						OnDestroy(void);
	void					OnCopy(BOOL bCopy);
	void					OnRightClick(NMHDR *nmhdr);
	void					OnTabSelectionChange(void);
	void					OnCreate(void);
	void					OnDirChanged(int iTabId);
	void					OnTabCtrlGetDispInfo(LPARAM lParam);
	void					OnDrawClipboard(void);
	void					OnChangeCBChain(WPARAM wParam,LPARAM lParam);
	void					OnSetFocus(void);
	LRESULT					OnDeviceChange(WPARAM wParam,LPARAM lParam);
	LRESULT					StatusBarMenuSelect(WPARAM wParam,LPARAM lParam);
	void					HandleDirectoryMonitoring(int iTabId);
	void					OnTabCtrlMButtonUp(POINT *pt);
	void					OnDisplayWindowResized(WPARAM wParam);
	void					OnStartedBrowsing(int iTabId, const TCHAR *szPath);
	void					OnDuplicateTab(int iTab);
	void					OnLockTab(int iTab);
	void					OnLockTabInternal(int iTab,int iTabId);
	void					OnLockTabAndAddress(int iTab);
	void					UpdateTabToolbar(void);
	void					OnAutoSizeColumns(void);
	void					OnToolbarViews(void);
	void					OnApplicationToolbarRClick();
	void					OnSortByAscending(BOOL bSortAscending);
	void					OnPreviousWindow(void);
	void					OnNextWindow(void);
	int						DetermineListViewObjectIndex(HWND hListView);
	void					OnLockToolbars(void);
	void					LoadAllSettings(ILoadSave **pLoadSave);
	void					OnShellNewItemCreated(LPARAM lParam);
	void					OnPaste(void);
	void					OnAppCommand(UINT cmd);
	void					OnRefresh(void);
	void					OnDirectoryModified(int iTabId);
	void					OnIdaRClick(void);
	void					OnAssocChanged(void);
	void					OnNdwRClick(POINT *pt);
	void					OnNdwIconRClick(POINT *pt);
	LRESULT					OnCustomDraw(LPARAM lParam);
	void					OnSortBy(SortMode sortMode);
	void					OnGroupBy(SortMode sortMode);
	void					OnSelectTab(const TabInfo_t &tab, BOOL setFocus);
	boost::optional<int>	GetTabIndex(const TabInfo_t &tab);
	boost::optional<TabInfo_t>	GetTabByIndex(int index);
	void					OnSelectTabByIndex(int iTab);
	void					OnSelectTabByIndex(int iTab,BOOL bSetFocus);

	/* Navigation. */
	void					OnBrowseBack();
	void					OnBrowseForward();
	void					OnHome();
	void					OnNavigateUp();
	void					GotoFolder(int FolderCSIDL);
	HRESULT					BrowseFolder(const TCHAR *szPath, UINT wFlags);
	HRESULT					BrowseFolder(const TCHAR *szPath, UINT wFlags, BOOL bOpenInNewTab, BOOL bSwitchToNewTab, BOOL bOpenInNewWindow);
	HRESULT					BrowseFolder(LPCITEMIDLIST pidlDirectory, UINT wFlags);
	HRESULT					BrowseFolder(LPCITEMIDLIST pidlDirectory, UINT wFlags, BOOL bOpenInNewTab, BOOL bSwitchToNewTab, BOOL bOpenInNewWindow);
	void					PlayNavigationSound() const;

	/* Main menu handlers. */
	void					OnChangeDisplayColors();
	void					OnFilterResults();
	void					OnMergeFiles();
	void					OnSplitFile();
	void					OnDestroyFiles();
	void					OnWildcardSelect(BOOL bSelect);
	void					OnSearch();
	void					OnCustomizeColors();
	void					OnRunScript();
	void					OnShowOptions();
	void					OnShowHelp();
	void					OnCheckForUpdates();
	void					OnAbout();
	void					OnSaveDirectoryListing() const;
	void					OnCreateNewFolder();
	void					OnResolveLink();

	void					OnNewTab();
	void					OnCopyItemPath(void) const;
	void					OnCopyUniversalPaths(void) const;
	void					OnSetFileAttributes(void) const;
	void					OnFileDelete(BOOL bPermanent);
	void					OnFileRename(void);
	void					OnShowFileProperties(void) const;
	int						HighlightSimilarFiles(HWND ListView) const;
	void					OnShowHiddenFiles(void);

	/* ListView private message handlers. */
	void					OnListViewMButtonDown(POINT *pt);
	void					OnListViewMButtonUp(POINT *pt);
	void					OnListViewLButtonDown(WPARAM wParam,LPARAM lParam);
	void					OnListViewDoubleClick(NMHDR *nmhdr);
	void					OnListViewFileRename(void);
	void					OnListViewGetInfoTip(LPARAM lParam);
	LRESULT					OnListViewKeyDown(LPARAM lParam);
	void					OnListViewItemChanged(LPARAM lParam);
	HRESULT					OnListViewBeginDrag(LPARAM lParam,DragTypes_t DragType);
	BOOL					OnListViewBeginLabelEdit(LPARAM lParam);
	BOOL					OnListViewEndLabelEdit(LPARAM lParam);
	void					OnListViewFileDelete(BOOL bPermanent);
	void					OnListViewRClick(POINT *pCursorPos);
	void					OnListViewBackgroundRClick(POINT *pCursorPos);
	void					OnListViewItemRClick(POINT *pCursorPos);
	void					OnListViewHeaderRClick(POINT *pCursorPos);
	int						GetColumnHeaderMenuList(unsigned int **pHeaderList);
	void					OnListViewShowFileProperties(void) const;
	void					OnListViewCopyItemPath(void) const;
	void					OnListViewCopyUniversalPaths(void) const;
	void					OnListViewSetFileAttributes(void) const;
	void					OnListViewPaste(void);

	/* TreeView private message handlers. */
	void					OnTreeViewFileRename(void);
	void					OnTreeViewFileDelete(BOOL bPermanent);
	void					OnTreeViewRightClick(WPARAM wParam,LPARAM lParam);
	void					OnTreeViewShowFileProperties(void) const;
	BOOL					OnTreeViewItemExpanding(LPARAM lParam);
	void					OnTreeViewSelChanged(LPARAM lParam);
	int						OnTreeViewBeginLabelEdit(LPARAM lParam);
	int						OnTreeViewEndLabelEdit(LPARAM lParam);
	LRESULT					OnTreeViewKeyDown(LPARAM lParam);
	void					OnTreeViewCopyItemPath(void) const;
	void					OnTreeViewCopy(BOOL bCopy);
	void					OnTreeViewSetFileAttributes(void) const;
	void					OnTreeViewCopyUniversalPaths(void) const;
	void					OnTreeViewPaste(void);

	/* Tab control private message handlers. */
	void					OnInitTabMenu(HMENU hMenu);
	void					OnTabCtrlLButtonDown(POINT *pt);
	void					OnTabCtrlLButtonUp(void);
	void					OnTabCtrlMouseMove(POINT *pt);
	void					OnTabCtrlRButtonUp(POINT *pt);
	void					ProcessTabCommand(UINT uMenuID,int iTabHit);

	/* Address bar. */
	void					CreateAddressBar(void);
	void					OnAddressBarGo(void);
	void					OnAddressBarBeginDrag(void);
	void					SetAddressBarText(LPITEMIDLIST pidl, const TCHAR *szDisplayText);

	/* Holder window private message handlers. */
	LRESULT CALLBACK		TreeViewHolderWindowCommandHandler(WPARAM wParam);
	LRESULT CALLBACK		TreeViewHolderWindowNotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void					OnTreeViewHolderWindowTimer(void);

	/* Tabs. */
	void					InitializeTabs(void);
	void					AddDefaultTabIcons(HIMAGELIST himlTab);
	void					SelectAdjacentTab(BOOL bNextTab);
	HRESULT					CreateNewTab(const TCHAR *TabDirectory,InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,int *pTabObjectIndex);
	HRESULT					CreateNewTab(LPCITEMIDLIST pidlDirectory,InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,int *pTabObjectIndex);
	void					InsertNewTab(LPCITEMIDLIST pidlDirectory,int iNewTabIndex,int iTabId);
	bool					CloseTab(const TabInfo_t &tab);
	void					RemoveTabFromControl(int iTab);
	bool					OnCloseTab(void);
	HRESULT					RestoreTabs(ILoadSave *pLoadSave);
	void					RefreshTab(int iTabId);
	void					RefreshAllTabs(void);
	void					CloseOtherTabs(int iTab);
	int						GetCurrentTabId() const;
	std::wstring			GetTabName(int iTab);
	void					SetTabName(int iTab, std::wstring strName, BOOL bUseCustomName);
	void					SetTabSelection(int Index);
	void					PushGlobalSettingsToTab(int iTabId);
	void					DuplicateTab(int iTabInternal);
	boost::optional<TabInfo_t>	GetTab(int tabId);
	int						GetNumTabs() const;
	int						MoveTab(const TabInfo_t &tab, int newIndex);

	/* Clone Window. */
	void					OnCloneWindow(void);

	/* Menus. */
	HMENU					InitializeRightClickMenu(void);
	void					SetProgramMenuItemStates(HMENU hProgramMenu);
	void					SetArrangeMenuItemStates();

	/* Columns. */
	void					SetDefaultColumns();

	/* File infotips. */
	void					CreateFileInfoTip(int iItem,TCHAR *szInfoTip,UINT cchMax);

	/* Control creation. */
	HWND					CreateMainListView(HWND hParent,DWORD Style);
	void					CreateMainControls(void);
	void					CreateFolderControls(void);
	void					CreateMainToolbar();
	void					CreateBookmarksToolbar(void);
	void					CreateDrivesToolbar(void);
	void					CreateApplicationToolbar();
	HWND					CreateTabToolbar(HWND hParent,int idCommand,TCHAR *szTip);
	void					CreateTabBacking(void);

	/* Main toolbars. */
	void					InitializeMainToolbars(void);
	void					AdjustMainToolbarSize(void);

	/* Main toolbar private message handlers. */
	void					OnMainToolbarRClick();

	/* Directory specific settings. */
	void					SaveDirectorySpecificSettings(int iTab);
	void					SetDirectorySpecificSettings(int iTab,LPITEMIDLIST pidlDirectory);

	/* Settings. */
	void					SaveAllSettings(void);
	LONG					SaveSettings();
	LONG					LoadSettings();
	void					ValidateLoadedSettings(void);
	void					ValidateColumns(void);
	void					ValidateSingleColumnSet(int iColumnSet,std::list<Column_t> *pColumnList);
	void					ApplyLoadedSettings(void);
	void					ApplyToolbarSettings(void);
	void					SetDefaultValues(void);
	void					TestConfigFile(void);
	void					SaveTabSettingsToRegistry(void);
	int						LoadTabSettingsFromRegistry(void);
	void					LoadColumnFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns);
	void					SaveColumnToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns);
	void					LoadColumnWidthsFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns);
	void					SaveColumnWidthsToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns);
	void					LoadDefaultColumnsFromRegistry(void);
	void					SaveDefaultColumnsToRegistry(void);
	void					InitializeBookmarks(void);
	void					SaveBookmarksToRegistry(void);
	void					LoadBookmarksFromRegistry(void);
	void					LoadApplicationToolbarFromRegistry();
	void					SaveApplicationToolbarToRegistry();
	void					SaveToolbarInformationToRegistry(void);
	void					LoadToolbarInformationFromRegistry(void);
	void					SaveDialogStatesToRegistry(void);
	void					LoadDialogStatesFromRegistry(void);

	/* Window state update. */
	void					UpdateWindowStates(void);
	void					UpdateMainWindowText(void);
	void					UpdateAddressBarText(void);
	void					UpdateTabText(void);
	void					UpdateTabText(int iTabId);
	void					UpdateTabText(int iTab,int iTabId);
	void					SetTabIcon(void);
	void					SetTabIcon(int iTabId);
	void					SetTabIcon(int iIndex,int iTabId);
	void					SetTabIcon(int iIndex,int iTabId,LPCITEMIDLIST pidlDirectory);
	void					UpdateTreeViewSelection(void);
	void					SetStatusBarParts(int width);
	void					ResizeWindows(void);
	void					SetListViewInitialPosition(HWND hListView);
	void					AdjustFolderPanePosition(void);
	HRESULT					UpdateStatusBarText(void);
	void					ToggleFolders(void);

	/* Windows 7 taskbar thumbnail previews. */
	void					InitializeTaskbarThumbnails();
	ATOM					RegisterTabProxyClass(const TCHAR *szClassName);
	void					CreateTabProxy(int iTabId,BOOL bSwitchToNewTab);
	void					RegisterTab(HWND hTabProxy, const TCHAR *szDisplayName, BOOL bTabActive);
	HBITMAP					CaptureTabScreenshot(int iTabId);
	void					GetTabLivePreviewBitmap(int iTabId,TabPreviewInfo_t *ptpi);
	void					RemoveTabProxy(int iTabId);
	void					InvalidateTaskbarThumbnailBitmap(int iTabId);
	void					UpdateTaskbarThumbnailsForTabSelectionChange(int selectedTabId);
	void					UpdateTaskbarThumbnailTtitle(int tabId, const std::wstring &title);
	void					SetTabProxyIcon(int iTabId, HICON hIcon);

	/* Windows 7 jumplist tasks. */
	void					SetupJumplistTasks();

	/* Languages. */
	void					SetLanguageModule(void);
	BOOL					VerifyLanguageVersion(const TCHAR *szLanguageModule) const;

	/* Default settings. */
	void					SetDefaultTabSettings(TabInfo_t *pTabInfo);

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
	void					OpenAllSelectedItems(BOOL bOpenInNewTab);
	void					OpenListViewItem(int iItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenItem(const TCHAR *szItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenItem(LPCITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenFolderItem(LPCITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenFileItem(LPCITEMIDLIST pidlItem,const TCHAR *szParameters);
	HRESULT					OnListViewCopy(BOOL bCopy);

	/* File context menu. */
	void					AddMenuEntries(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu);
	BOOL					HandleShellMenuItem(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,const TCHAR *szCmd);
	void					HandleCustomMenuItem(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,int iCmd);

	/* Listview selection file tests. */
	void					BuildListViewFileSelectionList(HWND hListView,std::list<std::wstring> *pFileSelectionList);
	HRESULT					TestListViewSelectionAttributes(SFGAOF *pItemAttributes) const;
	HRESULT					TestTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const;
	HRESULT					GetSelectionAttributes(SFGAOF *pItemAttributes) const;
	BOOL					CanCut() const;
	BOOL					CanCopy() const;
	BOOL					CanRename() const;
	BOOL					CanDelete() const;
	BOOL					CanShowFileProperties() const;
	BOOL					CanPaste() const;
	BOOL					TestItemAttributes(SFGAOF attributes) const;

	/* Display window file information. */
	void					UpdateDisplayWindow(void);
	void					UpdateDisplayWindowForZeroFiles(void);
	void					UpdateDisplayWindowForOneFile(void);
	void					UpdateDisplayWindowForMultipleFiles(void);

	/* Columns. */
	void					OnSelectColumns();
	void					CopyColumnInfoToClipboard(void);

	/* Bookmark handling. */
	HRESULT					ExpandAndBrowsePath(const TCHAR *szPath);
	HRESULT					ExpandAndBrowsePath(const TCHAR *szPath, BOOL bOpenInNewTab, BOOL bSwitchToNewTab);

	/* Filtering. */
	void					SetFilterStatus(void);

	/* Options dialog. */
	void					ShowOptions(void);
	void					AddLanguages(HWND hDlg);
	BOOL					AddLanguageToComboBox(HWND hComboBox, const TCHAR *szImageDirectory, const TCHAR *szFileName, WORD *pdwLanguage);
	int						GetLanguageIDFromIndex(HWND hDlg,int iIndex);

	/* Default settings dialog. */
	void					OnDefaultSettingsNewTabDir(HWND hDlg);
	void					DefaultSettingsSetNewTabDir(HWND hEdit,LPITEMIDLIST pidl);
	void					DefaultSettingsSetNewTabDir(HWND hEdit, const TCHAR *szPath);

	/* Files and folders dialog. */
	void					SetInfoTipWindowStates(HWND hDlg);
	void					SetFolderSizeWindowState(HWND hDlg);

	/* XML Settings. */
	void					LoadGenericSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveGenericSettingsToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	int						LoadTabSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveTabSettingsToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	void					SaveTabSettingsToXMLnternal(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pe);
	int						LoadColumnFromXML(IXMLDOMNode *pNode,std::list<Column_t> *pColumns);
	void					SaveColumnToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode, std::list<Column_t> *pColumns, const TCHAR *szColumnSet, int iIndent);
	int						LoadBookmarksFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveBookmarksToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	int						LoadDefaultColumnsFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveDefaultColumnsToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	void					SaveDefaultColumnsToXMLInternal(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pColumnsNode);
	void					SaveWindowPositionToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	void					SaveWindowPositionToXMLInternal(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pWndPosNode);
	void					LoadApplicationToolbarFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveApplicationToolbarToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	void					LoadToolbarInformationFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveToolbarInformationToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	void					SaveToolbarInformationToXMLnternal(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pe);
	void					LoadDialogStatesFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveDialogStatesToXML(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pRoot);
	void					MapAttributeToValue(IXMLDOMNode *pNode,WCHAR *wszName,WCHAR *wszValue);
	void					MapTabAttributeValue(WCHAR *wszName,WCHAR *wszValue,InitialSettings_t *pSettings,TabInfo_t *pTabInfo);

	/* IExplorerplusplus methods. */
	HWND					GetActiveListView() const;
	CShellBrowser			*GetActiveShellBrowser() const;
	HWND					GetTreeView() const;

	/* Helpers. */
	HANDLE					CreateWorkerThread();

	/* Menus. */
	void					InitializeMenus(void);
	void					SetMenuImages();
	void					SetMenuItemImageFromImageList(HMENU menu, UINT menuItemId, HIMAGELIST imageList, int bitmapIndex, std::vector<HBitmapPtr> &menuImages);
	HMENU					BuildViewsMenu();
	void					AddViewModesToMenu(HMENU menu);

	/* Miscellaneous. */
	void					CreateStatusBar(void);
	void					InitializeDisplayWindow(void);
	void					SetGoMenuName(HMENU hMenu,UINT uMenuID,UINT csidl);
	int						CreateDriveFreeSpaceString(const TCHAR *szPath, TCHAR *szBuffer, int nBuffer);
	BOOL					AnyItemsSelected(void);
	void					ShowMainRebarBand(HWND hwnd,BOOL bShow);
	BOOL					OnMouseWheel(MousewheelSource_t MousewheelSource,WPARAM wParam,LPARAM lParam);
	void					CycleViewState(BOOL bCycleForward);
	HMENU					CreateRebarHistoryMenu(BOOL bBack);
	CStatusBar				*GetStatusBar();
	SortMode				GetDefaultSortMode(LPCITEMIDLIST pidlDirectory) const;
	void					FolderSizeCallback(FolderSizeExtraInfo_t *pfsei,int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize);




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
	HWND					m_hFoldersToolbar;
	HWND					m_hTabBacking;
	HWND					m_hBookmarksToolbar;

	IDirectoryMonitor *		m_pDirMon;
	CMyTreeView *			m_pMyTreeView;
	CStatusBar *			m_pStatusBar;
	HANDLE					m_hTreeViewIconThread;

	HMODULE					m_hLanguageModule;

	/** Internal state. **/
	HWND					m_hLastActiveWindow;
	HWND					m_hNextClipboardViewer;
	HMENU					m_hArrangeSubMenu;
	HMENU					m_hGroupBySubMenu;
	HMENU					m_hArrangeSubMenuRClick;
	HMENU					m_hGroupBySubMenuRClick;
	HMENU					m_hTabRightClickMenu;
	HMENU					m_hToolbarRightClickMenu;
	TCHAR					m_CurrentDirectory[MAX_PATH];
	TCHAR					m_DefaultTabDirectoryStatic[MAX_PATH];
	TCHAR					m_DefaultTabDirectory[MAX_PATH];
	TCHAR					m_OldTreeViewFileName[MAX_PATH];
	DWORD					m_Language;
	LONG					m_DisplayWindowHeight;
	BOOL					m_bTreeViewRightClick;
	BOOL					m_bSelectingTreeViewDirectory;
	BOOL					m_bAttemptToolbarRestore;
	BOOL					m_bLanguageLoaded;
	BOOL					m_bTreeViewOpenInNewTab;
	unsigned int			m_TreeViewWidth;
	int						m_selectedTabIndex;
	int						m_selectedTabId;
	int						m_iMaxArrangeMenuItem;
	int						m_iLastSelectedTab;
	ULONG					m_SHChangeNotifyID;
	bool					m_InitializationFinished;

	std::list<ViewMode>		m_ViewModes;

	/* Initialization. */
	BOOL					m_bLoadSettingsFromXML;
	Gdiplus::Color			m_DisplayWindowCentreColor;
	Gdiplus::Color			m_DisplayWindowSurroundColor;
	COLORREF				m_DisplayWindowTextColor;
	HFONT					m_DisplayWindowFont;

	/* Tabs. */
	std::unordered_map<int, TabInfo_t> m_TabInfo;
	int						m_tabIdCounter;

	HWND					m_hActiveListView;
	CShellBrowser *			m_pActiveShellBrowser;

	/* Directory-specific settings. */
	std::list<DirectorySettings_t>	m_DirectorySettingsList;

	/* User options variables. */
	std::shared_ptr<Config>	m_config;
	BOOL					m_bShowTabBar;
	BOOL					m_bSavePreferencesToXMLFile;
	BOOL					m_bLockToolbars;
	BOOL					m_bShowTabBarAtBottom;
	BOOL					m_bShowTaskbarThumbnails;
	BOOL					m_bShowTaskbarThumbnailsProvisional;
	BOOL					m_bSynchronizeTreeview;
	BOOL					m_bTVAutoExpandSelected;
	StartupMode_t			m_StartupMode;
	NDefaultFileManager::ReplaceExplorerModes_t	m_ReplaceExplorerMode;

	/* Infotips (user options). */
	BOOL					m_bShowInfoTips;
	InfoTipType_t			m_InfoTipType;

	/* Global options. */
	ViewMode				m_ViewModeGlobal;
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
	ITaskbarList4			*m_pTaskbarList;
	std::list<TabProxyInfo_t>	m_TabProxyList;
	UINT					m_uTaskbarButtonCreatedMessage;
	BOOL					m_bTaskbarInitialised;

	/* Tabs. */
	CTabContainer			*m_pTabContainer;
	HFONT					m_hTabFont;
	HIMAGELIST				m_hTabCtrlImageList;

	/* Bookmarks. */
	CBookmarkFolder *		m_bfAllBookmarks;
	GUID					m_guidBookmarksToolbar;
	GUID					m_guidBookmarksMenu;
	CBookmarksToolbar		*m_pBookmarksToolbar;

	/* Customize colors. */
	std::vector<NColorRuleHelper::ColorRule_t>	m_ColorRules;

	/* Undo support. */
	CFileActionHandler		m_FileActionHandler;

	/* Toolbars. */
	REBARBANDINFO			m_ToolbarInformation[NUM_MAIN_TOOLBARS];
	MainToolbar				*m_mainToolbar;
	CDrivesToolbar			*m_pDrivesToolbar;
	CApplicationToolbar		*m_pApplicationToolbar;

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
	BOOL					m_bListViewRenaming;

	/* Tab handler data. */
	std::vector<int>		m_TabSelectionHistory;
	int						m_iPreviousTabSelectionId;
	BOOL					m_bTabBeenDragged;
	RECT					m_rcDraggedTab;
	int						m_iTabMenuItem;
	
	/* Cut items data. */
	std::list<std::wstring>	m_CutFileNameList;
	int						m_iCutTabInternal;

	/* Menu images. */
	std::vector<HBitmapPtr>	m_menuImages;

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