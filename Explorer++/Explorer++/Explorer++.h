// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarksMainMenu.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "IconResourceLoader.h"
#include "PluginCommandManager.h"
#include "PluginInterface.h"
#include "PluginMenuManager.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "TabNavigationInterface.h"
#include "TabRestorer.h"
#include "TabRestorerUI.h"
#include "UiTheming.h"
#include "ValueWrapper.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <unordered_map>

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED	WM_APP + 3

/* Private definitions. */
#define FROM_LISTVIEW				0
#define FROM_TREEVIEW				1

// Forward declarations.
class AddressBar;
class ApplicationToolbar;
class BookmarksToolbar;

namespace NColorRuleHelper
{
	struct ColorRule_t;
}

class DrivesToolbar;
struct Config;
struct ColumnWidth_t;
class ILoadSave;
class LoadSaveRegistry;
class LoadSaveXML;
class MainToolbar;
class MainWindow;
class Navigation;

namespace Plugins
{
	class PluginManager;
}

class ShellBrowser;
class TabContainer;
class TaskbarThumbnails;

__interface IDirectoryMonitor;

class MyTreeView;

class Explorerplusplus : public IExplorerplusplus, public TabNavigationInterface,
	public IFileContextMenuExternal, public PluginInterface
{
	friend LoadSaveRegistry;
	friend LoadSaveXML;

	friend LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

public:

	Explorerplusplus(HWND);
	~Explorerplusplus();

	LRESULT CALLBACK	RebarSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

	LRESULT CALLBACK	TreeViewHolderProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK	TreeViewSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	/* Directory modification. */
	static void			DirectoryAlteredCallback(const TCHAR *szFileName,DWORD dwAction,void *pData);

private:

	static const int		MIN_SHELL_MENU_ID = 1;
	static const int		MAX_SHELL_MENU_ID = 1000;

	static const UINT		MINIMUM_DISPLAYWINDOW_HEIGHT = 70;

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

	static const UINT_PTR LISTVIEW_ITEM_CHANGED_TIMER_ID = 100001;
	static const UINT LISTVIEW_ITEM_CHANGED_TIMEOUT = 50;

	// Represents the maximum number of icons that can be cached. This cache is
	// shared between various components in the application.
	static const int MAX_CACHED_ICONS = 1000;

	struct SortMenuItem
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

	static LRESULT CALLBACK	ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK		ListViewSubclassProc(HWND ListView, UINT msg, WPARAM wParam, LPARAM lParam);

	/* Main window message handlers. */
	LRESULT CALLBACK		CommandHandler(HWND hwnd, WPARAM wParam);
	LRESULT					HandleMenuOrAccelerator(HWND hwnd, WPARAM wParam);
	LRESULT					HandleControlNotification(HWND hwnd, WPARAM wParam);
	LRESULT CALLBACK		NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void					OnCreate();
	BOOL					OnSize(int MainWindowWidth,int MainWindowHeight);
	void					OnDpiChanged(const RECT *updatedWindowRect);
	int						OnClose();
	int						OnDestroy();
	void					OnRightClick(NMHDR *nmhdr);
	void					OnDrawClipboard();
	void					OnChangeCBChain(WPARAM wParam,LPARAM lParam);
	void					OnSetFocus();
	LRESULT					OnDeviceChange(WPARAM wParam,LPARAM lParam);
	LRESULT					StatusBarMenuSelect(WPARAM wParam,LPARAM lParam);
	void					OnDisplayWindowResized(WPARAM wParam);
	void					OnStartedBrowsing(int iTabId, const TCHAR *szPath);
	void					OnPreviousWindow();
	void					OnNextWindow();
	void					OnShellNewItemCreated(LPARAM lParam);
	void					OnAppCommand(UINT cmd);
	void					OnDirectoryModified(int iTabId);
	void					OnIdaRClick();
	void					OnAssocChanged();
	void					OnNdwRClick(POINT *pt);
	void					OnNdwIconRClick(POINT *pt);
	LRESULT					OnCustomDraw(LPARAM lParam);
	void					OnSelectTabByIndex(int iTab);

	/* Main menu handlers. */
	HRESULT					OnNewTab();
	bool					OnCloseTab();
	void					OnSaveDirectoryListing() const;
	void					OnCloneWindow();
	void					OnCopyItemPath() const;
	void					OnCopyUniversalPaths() const;
	void					OnSetFileAttributes() const;
	void					OnFileDelete(bool permanent);
	void					OnFileRename();
	void					OnShowFileProperties() const;
	void					OnCopy(BOOL bCopy);
	void					OnPaste();
	void					OnWildcardSelect(BOOL bSelect);
	void					OnResolveLink();
	void					OnLockToolbars();
	void					OnChangeDisplayColors();
	void					OnFilterResults();
	void					OnSortBy(SortMode sortMode);
	void					OnGroupBy(SortMode sortMode);
	void					OnSortByAscending(BOOL bSortAscending);
	void					OnShowHiddenFiles();
	void					OnRefresh();
	void					OnSelectColumns();
	void					OnAutoSizeColumns();
	void					OnCreateNewFolder();
	void					OnMergeFiles();
	void					OnSplitFile();
	void					OnDestroyFiles();
	void					OnSearch();
	void					OnCustomizeColors();
	void					OnRunScript();
	void					OnShowOptions();
	void					OnShowHelp();
	void					OnCheckForUpdates();
	void					OnAbout();

	HRESULT					OnGoBack();
	HRESULT					OnGoForward();
	HRESULT					OnGoToOffset(int offset);
	HRESULT					OnGoHome();
	HRESULT					OnGoToKnownFolder(REFKNOWNFOLDERID knownFolderId);

	int						HighlightSimilarFiles(HWND ListView) const;

	/* Main toolbar-specific handlers. */
	void					OnToolbarViews();

	/* ListView private message handlers. */
	void					OnListViewDoubleClick(NMHDR *nmhdr);
	void					OnListViewFileRename();
	void					OnListViewFileRenameSingle();
	void					OnListViewFileRenameMultiple();
	LRESULT					OnListViewKeyDown(LPARAM lParam);
	BOOL					OnListViewItemChanging(const NMLISTVIEW *changeData);
	HRESULT					OnListViewBeginDrag(LPARAM lParam,DragType dragType);
	BOOL					OnListViewBeginLabelEdit(LPARAM lParam);
	BOOL					OnListViewEndLabelEdit(LPARAM lParam);
	void					OnListViewFileDelete(bool permanent);
	void					OnListViewRClick(POINT *pCursorPos);
	void					OnListViewBackgroundRClick(POINT *pCursorPos);
	void					OnListViewItemRClick(POINT *pCursorPos);
	void					OnListViewCopyItemPath(void) const;
	void					OnListViewCopyUniversalPaths(void) const;
	void					OnListViewSetFileAttributes() const;
	void					OnListViewPaste(void);

	/* TreeView private message handlers. */
	void					OnTreeViewFileRename(void);
	void					OnTreeViewFileDelete(BOOL bPermanent);
	void					OnTreeViewRightClick(WPARAM wParam,LPARAM lParam);
	void					OnTreeViewShowFileProperties(void) const;
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
	void					OnTabCreated(int tabId, BOOL switchToNewTab);
	void					OnTabSelected(const Tab &tab);
	void					ShowTabBar();
	void					HideTabBar();
	HRESULT					RestoreTabs(ILoadSave *pLoadSave);
	void					OnTabListViewSelectionChanged(const Tab &tab);

	/* TabNavigationInterface methods. */
	HRESULT					CreateNewTab(PCIDLIST_ABSOLUTE pidlDirectory, bool selected);

	void					OnNavigationCompleted(const Tab &tab);

	/* PluginInterface. */
	IExplorerplusplus		*GetCoreInterface();
	TabContainer			*GetTabContainer();
	Navigation				*GetNavigation();
	Plugins::PluginMenuManager	*GetPluginMenuManager();
	UiTheming				*GetUiTheming();
	AcceleratorUpdater		*GetAccleratorUpdater();
	Plugins::PluginCommandManager	*GetPluginCommandManager();

	/* Plugins. */
	void					InitializePlugins();

	/* Menus. */
	HMENU					InitializeRightClickMenu(void);
	void					SetProgramMenuItemStates(HMENU hProgramMenu);

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
	void					OnUseLargeToolbarIconsUpdated(BOOL newValue);
	boost::signals2::connection	AddToolbarContextMenuObserver(const ToolbarContextMenuSignal::slot_type &observer);

	/* Main toolbar private message handlers. */
	void					OnToolbarRClick(HWND sourceWindow);

	/* Settings. */
	void					SaveAllSettings();
	void					LoadAllSettings(ILoadSave **pLoadSave);
	void					ValidateLoadedSettings();
	void					ValidateColumns(FolderColumns &folderColumns);
	void					ValidateSingleColumnSet(int iColumnSet, std::vector<Column_t> &columns);
	void					ApplyToolbarSettings(void);
	void					TestConfigFile(void);

	/* Registry settings. */
	LONG					LoadGenericSettingsFromRegistry();
	LONG					SaveGenericSettingsToRegistry();
	void					SaveTabSettingsToRegistry(void);
	int						LoadTabSettingsFromRegistry();
	std::vector<Column_t>	LoadColumnFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName);
	void					SaveColumnToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::vector<Column_t> *pColumns);
	std::vector<ColumnWidth_t>	LoadColumnWidthsFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName);
	void					SaveColumnWidthsToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::vector<Column_t> *pColumns);
	void					LoadDefaultColumnsFromRegistry();
	void					SaveDefaultColumnsToRegistry();
	void					SaveBookmarksToRegistry();
	void					LoadBookmarksFromRegistry();
	void					LoadApplicationToolbarFromRegistry();
	void					SaveApplicationToolbarToRegistry();
	void					SaveToolbarInformationToRegistry(void);
	void					LoadToolbarInformationFromRegistry(void);
	void					SaveDialogStatesToRegistry(void);
	void					LoadDialogStatesFromRegistry(void);

	/* XML Settings. */
	void					LoadGenericSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveGenericSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	int						LoadTabSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveTabSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void					SaveTabSettingsToXMLnternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	int						LoadColumnFromXML(IXMLDOMNode *pNode, std::vector<Column_t> &outputColumns);
	void					SaveColumnToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode, const std::vector<Column_t> &columns, const TCHAR *szColumnSet, int iIndent);
	int						LoadBookmarksFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveBookmarksToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	int						LoadDefaultColumnsFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveDefaultColumnsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void					SaveDefaultColumnsToXMLInternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode);
	void					SaveWindowPositionToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void					SaveWindowPositionToXMLInternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pWndPosNode);
	void					LoadApplicationToolbarFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveApplicationToolbarToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void					LoadToolbarInformationFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveToolbarInformationToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void					SaveToolbarInformationToXMLnternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	void					LoadDialogStatesFromXML(IXMLDOMDocument *pXMLDom);
	void					SaveDialogStatesToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void					MapAttributeToValue(IXMLDOMNode *pNode, WCHAR *wszName, WCHAR *wszValue);
	void					MapTabAttributeValue(WCHAR *wszName, WCHAR *wszValue, TabSettings &tabSettings, FolderSettings &folderSettings);

	/* Window state update. */
	void					UpdateWindowStates(const Tab &tab);
	void					UpdateTreeViewSelection();
	void					SetStatusBarParts(int width);
	void					ResizeWindows(void);
	void					SetListViewInitialPosition(HWND hListView);
	void					AdjustFolderPanePosition(void);
	HRESULT					UpdateStatusBarText(const Tab &tab);
	void					ToggleFolders(void);
	void					UpdateLayout();

	/* Languages. */
	void					SetLanguageModule();
	BOOL					VerifyLanguageVersion(const TCHAR *szLanguageModule) const;

	/* Sort menu. */
	void					UpdateSortMenuItems(const Tab &tab);
	void					InsertSortMenuItems();
	void					DeleteSortMenuItems();
	void					SetSortMenuItemStates(const Tab &tab);

	/* File operations. */
	void					CopyToFolder(bool move);
	void					OpenAllSelectedItems(BOOL bOpenInNewTab);
	void					OpenListViewItem(int iItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenItem(const TCHAR *szItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);
	void					OpenItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);
	void					OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);
	void					OpenFileItem(PCIDLIST_ABSOLUTE pidlItem,const TCHAR *szParameters);
	HRESULT					OnListViewCopy(BOOL bCopy);

	/* File context menu. */
	void					AddMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, HMENU hMenu);
	BOOL					HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd);
	void					HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, int iCmd);

	/* File selection tests. */
	BOOL					AnyItemsSelected() const;
	bool					CanCreate() const;
	BOOL					CanCut() const;
	BOOL					CanCopy() const;
	BOOL					CanRename() const;
	BOOL					CanDelete() const;
	BOOL					CanShowFileProperties() const;
	BOOL					CanPaste() const;
	BOOL					TestItemAttributes(SFGAOF attributes) const;
	HRESULT					GetSelectionAttributes(SFGAOF *pItemAttributes) const;

	void					BuildListViewFileSelectionList(HWND hListView,std::list<std::wstring> *pFileSelectionList);
	HRESULT					TestListViewItemAttributes(int item, SFGAOF attributes) const;
	HRESULT					GetListViewSelectionAttributes(SFGAOF *pItemAttributes) const;
	HRESULT					GetListViewItemAttributes(const Tab &tab, int item, SFGAOF *pItemAttributes) const;

	HRESULT					GetTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const;

	/* Display window file information. */
	void					UpdateDisplayWindow(const Tab &tab);
	void					UpdateDisplayWindowForZeroFiles(const Tab &tab);
	void					UpdateDisplayWindowForOneFile(const Tab &tab);
	void					UpdateDisplayWindowForMultipleFiles(const Tab &tab);

	/* Columns. */
	void					CopyColumnInfoToClipboard(void);

	/* Bookmark handling. */
	HRESULT					ExpandAndBrowsePath(const TCHAR *szPath);
	HRESULT					ExpandAndBrowsePath(const TCHAR *szPath, BOOL bOpenInNewTab, BOOL bSwitchToNewTab);

	/* Filtering. */
	void					ToggleFilterStatus();

	/* IExplorerplusplus methods. */
	Config					*GetConfig() const;
	HMODULE					GetLanguageModule() const;
	HWND					GetMainWindow() const;
	HWND					GetActiveListView() const;
	ShellBrowser			*GetActiveShellBrowser() const;
	TabContainer			*GetTabContainer() const;
	HWND					GetTreeView() const;
	IDirectoryMonitor		*GetDirectoryMonitor() const;
	IconResourceLoader		*GetIconResourceLoader() const;
	CachedIcons				*GetCachedIcons();
	BOOL					GetSavePreferencesToXmlFile() const;
	void					SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile);

	/* Menus. */
	void					InitializeMainMenu();
	void					SetGoMenuName(HMENU hMenu, UINT uMenuID, UINT csidl);
	void					SetMainMenuImages();
	boost::signals2::connection	AddMainMenuPreShowObserver(const MainMenuPreShowSignal::slot_type &observer);
	HMENU					BuildViewsMenu();
	void					AddViewModesToMenu(HMENU menu);

	/* Miscellaneous. */
	void					CreateStatusBar(void);
	void					InitializeDisplayWindow();
	int						CreateDriveFreeSpaceString(const TCHAR *szPath, TCHAR *szBuffer, int nBuffer);
	void					ShowMainRebarBand(HWND hwnd,BOOL bShow);
	BOOL					OnMouseWheel(MousewheelSource_t MousewheelSource,WPARAM wParam,LPARAM lParam);
	void					CycleViewState(BOOL bCycleForward);
	HMENU					CreateRebarHistoryMenu(BOOL bBack);
	StatusBar				*GetStatusBar();
	void					HandleDirectoryMonitoring(int iTabId);
	int						DetermineListViewObjectIndex(HWND hListView);

	static void				FolderSizeCallbackStub(int nFolders, int nFiles, PULARGE_INTEGER lTotalFolderSize, LPVOID pData);
	void					FolderSizeCallback(FolderSizeExtraInfo_t *pfsei,int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize);

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
	MyTreeView *			m_pMyTreeView;
	StatusBar *			m_pStatusBar;

	HMODULE					m_hLanguageModule;

	/** Internal state. **/
	HWND					m_hLastActiveWindow;
	HWND					m_hNextClipboardViewer;
	std::wstring			m_CurrentDirectory;
	TCHAR					m_OldTreeViewFileName[MAX_PATH];
	BOOL					m_bTreeViewRightClick;
	BOOL					m_bSelectingTreeViewDirectory;
	BOOL					m_bAttemptToolbarRestore;
	BOOL					m_bLanguageLoaded;
	BOOL					m_bTreeViewOpenInNewTab;
	BOOL					m_bShowTabBar;
	int						m_iLastSelectedTab;
	ULONG					m_SHChangeNotifyID;
	ValueWrapper<bool>		m_InitializationFinished;

	/* Initialization. */
	BOOL					m_bLoadSettingsFromXML;

	MainWindow				*m_mainWindow;
	AddressBar				*m_addressBar;

	std::unique_ptr<Navigation>	m_navigation;

	std::unique_ptr<IconResourceLoader>	m_iconResourceLoader;
	DpiCompatibility		m_dpiCompat;

	CachedIcons				m_cachedIcons;

	MainMenuPreShowSignal	m_mainMenuPreShowSignal;

	/* Tabs. */
	TabContainer			*m_tabContainer;
	std::unique_ptr<TabRestorer>	m_tabRestorer;
	std::unique_ptr<TabRestorerUI>	m_tabRestorerUI;
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
	ShellBrowser *			m_pActiveShellBrowser;

	/* User options variables. */
	std::shared_ptr<Config>	m_config;	
	BOOL					m_bSavePreferencesToXMLFile;

	TaskbarThumbnails		*m_taskbarThumbnails;

	std::vector<boost::signals2::scoped_connection>	m_connections;

	/* Bookmarks. */
	BookmarkTree			m_bookmarkTree;
	std::unique_ptr<BookmarksMainMenu> m_bookmarksMainMenu;
	BookmarksToolbar		*m_pBookmarksToolbar;

	/* Customize colors. */
	std::vector<NColorRuleHelper::ColorRule_t>	m_ColorRules;

	/* Undo support. */
	FileActionHandler		m_FileActionHandler;

	/* Toolbars. */
	REBARBANDINFO			m_ToolbarInformation[NUM_MAIN_TOOLBARS];
	MainToolbar				*m_mainToolbar;
	DrivesToolbar			*m_pDrivesToolbar;
	ApplicationToolbar		*m_pApplicationToolbar;

	/* Display window folder sizes. */
	std::list<DWFolderSize_t>	m_DWFolderSizes;
	int						m_iDWFolderSizeUniqueId;

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
	std::vector<wil::unique_hbitmap>	m_menuImages;

	/* Sort menu. */
	HMENU					m_hSortSubMenu;
	HMENU					m_hGroupBySubMenu;
	std::vector<SortMenuItem>	m_sortMenuItems;

	/* Mousewheel. */
	int						m_zDeltaTotal;

	/* TreeView middle click. */
	HTREEITEM				m_hTVMButtonItem;

	BOOL					m_blockNextListViewSelection;
};