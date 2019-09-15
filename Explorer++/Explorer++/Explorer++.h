// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "PluginCommandManager.h"
#include "PluginInterface.h"
#include "PluginMenuManager.h"
#include "ShellBrowser/iShellView.h"
#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "TabContainerInterface.h"
#include "TabInterface.h"
#include "UiTheming.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/ImageWrappers.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <unordered_map>

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED	WM_APP + 3

/* Private definitions. */
#define FROM_LISTVIEW				0
#define FROM_TREEVIEW				1

// Forward declarations.
class AddressBar;
class CApplicationToolbar;
class CBookmarksToolbar;

namespace NColorRuleHelper
{
	struct ColorRule_t;
}

class CDrivesToolbar;
struct Config;
struct ColumnWidth_t;
class ILoadSave;
class CLoadSaveRegistry;
class CLoadSaveXML;
class MainToolbar;
class MainWindow;
class Navigation;

namespace Plugins
{
	class PluginManager;
}

class CShellBrowser;
class TabContainer;
class TaskbarThumbnails;

class CBookmarkFolder;

__interface IDirectoryMonitor;

class CMyTreeView;

class Explorerplusplus : public IExplorerplusplus, public TabContainerInterface, public TabInterface,
	public IFileContextMenuExternal, public PluginInterface
{
	friend CLoadSaveRegistry;
	friend CLoadSaveXML;

	friend LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	friend void FolderSizeCallbackStub(int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize,LPVOID pData);

public:

	Explorerplusplus(HWND);
	~Explorerplusplus();

	LRESULT CALLBACK	ListViewSubclassProc(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);

	LRESULT CALLBACK	RebarSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

	LRESULT CALLBACK	TreeViewHolderProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TreeViewSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	/* Options dialog. */
	INT_PTR CALLBACK	GeneralSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	FilesFoldersProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	WindowProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	DefaultSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR CALLBACK	TabSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	/* Directory modification. */
	static void			DirectoryAlteredCallback(const TCHAR *szFileName,DWORD dwAction,void *pData);

private:

	static const int		MIN_SHELL_MENU_ID = 1;
	static const int		MAX_SHELL_MENU_ID = 1000;

	static const UINT		MINIMUM_DISPLAYWINDOW_HEIGHT = 70;

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

	static const UINT_PTR AUTOSAVE_TIMER_ID = 100000;
	static const UINT AUTOSAVE_TIMEOUT = 30000;

	struct ArrangeMenuItem_t
	{
		UINT SortById;
		UINT GroupById;
	};

	struct FileContextMenuInfo_t
	{
		UINT uFrom;
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

	LRESULT CALLBACK		WindowProcedure(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	/* Internal private functions. */
	void					UpdateArrangeMenuItems(void);

	/* <----Private message handlers.----> */

	/* Main window private message handlers. */
	LRESULT CALLBACK		CommandHandler(HWND hwnd, WPARAM wParam);
	LRESULT					HandleMenuOrAccelerator(HWND hwnd, WPARAM wParam);
	LRESULT					HandleControlNotification(HWND hwnd, WPARAM wParam);
	LRESULT CALLBACK		NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	BOOL					OnSize(int MainWindowWidth,int MainWindowHeight);
	int						OnClose(void);
	int						OnDestroy(void);
	void					OnCopy(BOOL bCopy);
	void					OnRightClick(NMHDR *nmhdr);
	void					OnCreate(void);
	void					OnDrawClipboard(void);
	void					OnChangeCBChain(WPARAM wParam,LPARAM lParam);
	void					OnSetFocus(void);
	LRESULT					OnDeviceChange(WPARAM wParam,LPARAM lParam);
	LRESULT					StatusBarMenuSelect(WPARAM wParam,LPARAM lParam);
	void					HandleDirectoryMonitoring(int iTabId);
	void					OnDisplayWindowResized(WPARAM wParam);
	void					OnStartedBrowsing(int iTabId, const TCHAR *szPath);
	void					OnAutoSizeColumns(void);
	void					OnToolbarViews(void);
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
	void					OnSelectTabByIndex(int iTab);

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
	void					OnFileDelete(bool permanent);
	void					OnFileRename(void);
	void					OnShowFileProperties(void) const;
	int						HighlightSimilarFiles(HWND ListView) const;
	void					OnShowHiddenFiles(void);

	/* ListView private message handlers. */
	void					OnListViewMButtonDown(POINT *pt);
	void					OnListViewMButtonUp(POINT *pt);
	void					OnListViewLButtonDown(WPARAM wParam,LPARAM lParam);
	void					OnListViewDoubleClick(NMHDR *nmhdr);
	void					OnListViewFileRename();
	void					OnListViewFileRenameSingle();
	void					OnListViewFileRenameMultiple();
	LRESULT					OnListViewKeyDown(LPARAM lParam);
	void					OnListViewItemChanged(LPARAM lParam);
	HRESULT					OnListViewBeginDrag(LPARAM lParam,DragTypes_t DragType);
	BOOL					OnListViewBeginLabelEdit(LPARAM lParam);
	BOOL					OnListViewEndLabelEdit(LPARAM lParam);
	void					OnListViewFileDelete(bool permanent);
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

	/* Holder window private message handlers. */
	LRESULT CALLBACK		TreeViewHolderWindowCommandHandler(WPARAM wParam);
	LRESULT CALLBACK		TreeViewHolderWindowNotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void					OnTreeViewHolderWindowTimer(void);

	/* Tab backing. */
	void					CreateTabBacking();
	void					OnTabsInitialized();
	void					OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void					UpdateTabToolbar();

	/* Tabs. */
	void					InitializeTabs();
	boost::signals2::connection	AddTabsInitializedObserver(const TabsInitializedSignal::slot_type &observer);
	void					OnTabSelected(const Tab &tab);
	void					ShowTabBar();
	void					HideTabBar();
	bool					OnCloseTab();
	HRESULT					RestoreTabs(ILoadSave *pLoadSave);
	HRESULT					RefreshTab(const Tab &tab);

	void					OnNavigationCompleted(const Tab &tab);

	/* PluginInterface. */
	TabContainer			*GetTabContainer();
	TabInterface			*GetTabInterface();
	Navigation				*GetNavigation();
	Plugins::PluginMenuManager	*GetPluginMenuManager();
	UiTheming				*GetUiTheming();
	AcceleratorUpdater		*GetAccleratorUpdater();
	Plugins::PluginCommandManager	*GetPluginCommandManager();

	/* Plugins. */
	void					InitializePlugins();

	/* Clone Window. */
	void					OnCloneWindow(void);

	/* Menus. */
	HMENU					InitializeRightClickMenu(void);
	void					SetProgramMenuItemStates(HMENU hProgramMenu);
	void					SetArrangeMenuItemStates();

	/* Control creation. */
	HWND					CreateMainListView(HWND hParent);
	void					CreateMainControls(void);
	void					CreateFolderControls(void);
	void					CreateAddressBar();
	void					CreateMainToolbar();
	void					CreateBookmarksToolbar(void);
	void					CreateDrivesToolbar(void);
	void					CreateApplicationToolbar();
	HWND					CreateTabToolbar(HWND hParent,int idCommand,TCHAR *szTip);

	/* Main toolbars. */
	void					InitializeMainToolbars(void);
	void					AdjustMainToolbarSize(void);
	boost::signals2::connection	AddToolbarContextMenuObserver(const ToolbarContextMenuSignal::slot_type &observer);

	/* Main toolbar private message handlers. */
	void					OnToolbarRClick(HWND sourceWindow);

	/* Settings. */
	void					SaveAllSettings();
	LONG					SaveSettings();
	LONG					LoadSettings();
	void					ValidateLoadedSettings();
	void					ValidateColumns(FolderColumns &folderColumns);
	void					ValidateSingleColumnSet(int iColumnSet, std::vector<Column_t> &columns);
	void					ApplyLoadedSettings(void);
	void					ApplyToolbarSettings(void);
	void					TestConfigFile(void);
	void					SaveTabSettingsToRegistry(void);
	int						LoadTabSettingsFromRegistry();
	std::vector<Column_t>	LoadColumnFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName);
	void					SaveColumnToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::vector<Column_t> *pColumns);
	std::vector<ColumnWidth_t>	LoadColumnWidthsFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName);
	void					SaveColumnWidthsToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::vector<Column_t> *pColumns);
	void					LoadDefaultColumnsFromRegistry();
	void					SaveDefaultColumnsToRegistry();
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
	void					UpdateTreeViewSelection(void);
	void					SetStatusBarParts(int width);
	void					ResizeWindows(void);
	void					SetListViewInitialPosition(HWND hListView);
	void					AdjustFolderPanePosition(void);
	HRESULT					UpdateStatusBarText(void);
	void					ToggleFolders(void);
	void					UpdateLayout();

	/* Languages. */
	void					SetLanguageModule(void);
	BOOL					VerifyLanguageVersion(const TCHAR *szLanguageModule) const;

	/* Arrange menu. */
	void					InitializeArrangeMenuItems(void);
	void					SetActiveArrangeMenuItems(void);
	int						InsertArrangeMenuItems(HMENU hMenu);
	void					DeletePreviousArrangeMenuItems(void);

	/* File operations. */
	void					CopyToFolder(bool move);
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
	HRESULT					TestListViewItemAttributes(int item, SFGAOF attributes) const;
	HRESULT					GetListViewSelectionAttributes(SFGAOF *pItemAttributes) const;
	HRESULT					GetListViewItemAttributes(int item, SFGAOF *pItemAttributes) const;
	HRESULT					GetTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const;
	HRESULT					GetSelectionAttributes(SFGAOF *pItemAttributes) const;
	bool					CanCreate() const;
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
	void					ToggleFilterStatus();

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
	int						LoadColumnFromXML(IXMLDOMNode *pNode, std::vector<Column_t> &outputColumns);
	void					SaveColumnToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode, const std::vector<Column_t> &columns, const TCHAR *szColumnSet, int iIndent);
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
	void					MapTabAttributeValue(WCHAR *wszName, WCHAR *wszValue, TabSettings &tabSettings, FolderSettings &folderSettings);

	/* IExplorerplusplus methods. */
	HWND					GetMainWindow() const;
	HWND					GetActiveListView() const;
	CShellBrowser			*GetActiveShellBrowser() const;
	TabContainer			*GetTabContainer() const;
	HWND					GetTreeView() const;
	IDirectoryMonitor		*GetDirectoryMonitor() const;

	/* Helpers. */
	HANDLE					CreateWorkerThread();

	/* Menus. */
	void					InitializeMenus(void);
	void					SetMainMenuImages();
	HMENU					BuildViewsMenu();
	void					AddViewModesToMenu(HMENU menu);

	/* Miscellaneous. */
	void					CreateStatusBar(void);
	void					InitializeDisplayWindow();
	void					SetGoMenuName(HMENU hMenu,UINT uMenuID,UINT csidl);
	int						CreateDriveFreeSpaceString(const TCHAR *szPath, TCHAR *szBuffer, int nBuffer);
	BOOL					AnyItemsSelected(void);
	void					ShowMainRebarBand(HWND hwnd,BOOL bShow);
	BOOL					OnMouseWheel(MousewheelSource_t MousewheelSource,WPARAM wParam,LPARAM lParam);
	void					CycleViewState(BOOL bCycleForward);
	HMENU					CreateRebarHistoryMenu(BOOL bBack);
	CStatusBar				*GetStatusBar();
	void					FolderSizeCallback(FolderSizeExtraInfo_t *pfsei,int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize);

	/* ------ Internal state. ------ */

	HWND					m_hContainer;
	HWND					m_hStatusBar;
	HWND					m_hMainRebar;
	HWND					m_hDisplayWindow;
	HWND					m_hTabWindowToolbar;
	HWND					m_hTreeView;
	HWND					m_hHolder;
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
	TCHAR					m_CurrentDirectory[MAX_PATH];
	TCHAR					m_OldTreeViewFileName[MAX_PATH];
	DWORD					m_Language;
	BOOL					m_bTreeViewRightClick;
	BOOL					m_bSelectingTreeViewDirectory;
	BOOL					m_bAttemptToolbarRestore;
	BOOL					m_bLanguageLoaded;
	BOOL					m_bTreeViewOpenInNewTab;
	BOOL					m_bShowTabBar;
	int						m_iMaxArrangeMenuItem;
	int						m_iLastSelectedTab;
	ULONG					m_SHChangeNotifyID;
	bool					m_InitializationFinished;

	static const std::array<ViewMode, 8>	m_viewModes;

	/* Initialization. */
	BOOL					m_bLoadSettingsFromXML;

	MainWindow				*m_mainWindow;
	AddressBar				*m_addressBar;

	Navigation				*m_navigation;

	/* Tabs. */
	TabContainer			*m_tabContainer;
	TabsInitializedSignal	m_tabsInitializedSignal;

	ToolbarContextMenuSignal	m_toolbarContextMenuSignal;

	/* Theming. */
	std::unique_ptr<UiTheming>	m_uiTheming;

	/* Plugins. */
	std::unique_ptr<Plugins::PluginManager>	m_pluginManager;
	Plugins::PluginMenuManager	m_pluginMenuManager;
	AcceleratorUpdater		m_acceleratorUpdater;
	Plugins::PluginCommandManager	m_pluginCommandManager;

	HWND					m_hActiveListView;
	CShellBrowser *			m_pActiveShellBrowser;

	/* User options variables. */
	std::shared_ptr<Config>	m_config;	
	BOOL					m_bSavePreferencesToXMLFile;

	TaskbarThumbnails		*m_taskbarThumbnails;

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