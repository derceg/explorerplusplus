// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorUpdater.h"
#include "BrowserCommandController.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "CommandLine.h"
#include "CoreInterface.h"
#include "IconFetcherImpl.h"
#include "Literals.h"
#include "MainToolbarStorage.h"
#include "PluginInterface.h"
#include "Plugins/PluginCommandManager.h"
#include "Plugins/PluginMenuManager.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowserEmbedder.h"
#include "ShellBrowser/SortModes.h"
#include "Tab.h"
#include "TabNavigationInterface.h"
#include "ValueWrapper.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <optional>

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED WM_APP + 3

// Forward declarations.
class AddressBar;
class BookmarksMainMenu;
class BookmarksToolbar;
struct Config;
class DrivesToolbar;
class HolderWindow;
class IconResourceLoader;
__interface IDirectoryMonitor;
class ILoadSave;
class LoadSaveRegistry;
class LoadSaveXML;
class MainFontSetter;
class MainToolbar;
class MainWindow;
struct NavigateParams;
struct RebarBandStorageInfo;
class ShellBrowserImpl;
class ShellTreeView;
class TabContainer;
class TabRestorer;
class TabRestorerMenu;
struct TabSettings;
struct TabStorageData;
class TaskbarThumbnails;
class ThemeWindowTracker;
class UiTheming;
class WindowSubclassWrapper;

namespace Applications
{
class ApplicationToolbar;
}

namespace Plugins
{
class PluginManager;
}

class Explorerplusplus :
	public BrowserWindow,
	public CoreInterface,
	private FileContextMenuHandler,
	public PluginInterface,
	private ShellBrowserEmbedder,
	public TabNavigationInterface
{
	friend LoadSaveRegistry;
	friend LoadSaveXML;

public:
	Explorerplusplus(HWND hwnd, CommandLine::Settings *commandLineSettings);
	~Explorerplusplus();

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/* Directory modification. */
	static void DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction, void *pData);

	// BrowserWindow
	BrowserPane *GetActivePane() const override;
	BrowserCommandController *GetCommandController() override;
	void FocusActiveTab() override;

	// Note that there is also GetActiveShellBrowserImpl() declared below. There are some places
	// where this method can be used, as the calling code only uses methods from the ShellBrowser
	// interface. However, there are other places where implementation-only methods are used, in
	// which case GetActiveShellBrowserImpl() is required. That's the reason these two methods
	// exist.
	ShellBrowser *GetActiveShellBrowser() override;

private:
	static constexpr UINT WM_APP_CLOSE = WM_APP + 1;

	static const int OPEN_IN_NEW_TAB_MENU_ITEM_ID = FileContextMenuManager::MAX_SHELL_MENU_ID + 1;

	static const int PREVIOUS_BACKGROUND_CONTEXT_MENU_MIN_ID = 0;
	static const int PREVIOUS_BACKGROUND_CONTEXT_MENU_MAX_ID = 1000;

	static const UINT DISPLAY_WINDOW_MINIMUM_WIDTH = 70;
	static const UINT DISPLAY_WINDOW_MINIMUM_HEIGHT = 70;

	static constexpr auto TREEVIEW_MINIMUM_WIDTH = 70_px;
	static constexpr double TREEVIEW_MAXIMUM_WIDTH_PERCENTAGE = 0.8;

	// Main rebar band IDs. These are used to load and save data, so the values shouldn't be
	// changed.
	static const UINT REBAR_BAND_ID_MAIN_TOOLBAR = 0;
	static const UINT REBAR_BAND_ID_ADDRESS_BAR = 1;
	static const UINT REBAR_BAND_ID_BOOKMARKS_TOOLBAR = 2;
	static const UINT REBAR_BAND_ID_DRIVES_TOOLBAR = 3;
	static const UINT REBAR_BAND_ID_APPLICATIONS_TOOLBAR = 4;

	static const std::vector<std::wstring> BLACKLISTED_BACKGROUND_MENU_CLSID_ENTRIES;

	static const UINT_PTR AUTOSAVE_TIMER_ID = 100000;
	static const UINT AUTOSAVE_TIMEOUT = 30000;

	static const UINT_PTR LISTVIEW_ITEM_CHANGED_TIMER_ID = 100001;
	static const UINT LISTVIEW_ITEM_CHANGED_TIMEOUT = 50;

	// Represents the maximum number of icons that can be cached. This cache is
	// shared between various components in the application.
	static const int MAX_CACHED_ICONS = 1000;

	static inline constexpr COLORREF TAB_BAR_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	// When changing the font size, it will be decreased/increased by this amount.
	static const int FONT_SIZE_CHANGE_DELTA = 1_pt;

	struct DirectoryAltered
	{
		int iIndex;
		int iFolderIndex;
		void *pData;
	};

	struct DWFolderSizeCompletion
	{
		ULARGE_INTEGER liFolderSize;
		int uId;
		int iTabId;
	};

	struct DWFolderSize
	{
		int uId;
		int iTabId;
		BOOL bValid;
	};

	struct FolderSizeExtraInfo
	{
		void *pContainer;
		int uId;
	};

	struct InternalRebarBandInfo
	{
		UINT id;
		HWND child;
		UINT height;
		bool newLine;
		bool useChevron;
		bool showBand;
		UINT length;
		std::optional<UINT> idealLength;
	};

	enum class PasteType
	{
		Normal,
		Shortcut
	};

	enum class FocusChangeDirection
	{
		Previous,
		Next
	};

	enum class FontSizeType
	{
		Decrease,
		Increase
	};

	LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ListViewSubclassProc(HWND ListView, UINT msg, WPARAM wParam, LPARAM lParam);

	/* Main window message handlers. */
	LRESULT CALLBACK CommandHandler(HWND hwnd, HWND control, int id, UINT notificationCode);
	LRESULT HandleMenuOrToolbarButtonOrAccelerator(HWND hwnd, int id, UINT notificationCode);
	LRESULT HandleControlNotification(HWND hwnd, UINT notificationCode);
	LRESULT CALLBACK NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnCreate();
	bool OnActivate(int activationState, bool minimized);
	void OnSize(HWND hwnd, UINT state, int mainWindowWidth, int mainWindowHeight);
	void OnDpiChanged(const RECT *updatedWindowRect);
	std::optional<LRESULT> OnCtlColorStatic(HWND hwnd, HDC hdc);
	void OnSettingChange(const WCHAR *systemParameter);
	void RequestCloseApplication() override;
	int OnDestroy();
	void OnDeviceChange(WPARAM wParam, LPARAM lParam);
	void OnFocusNextWindow(FocusChangeDirection direction);
	void OnAppCommand(UINT cmd);
	void OnDirectoryModified(const Tab &tab);
	void OnAssocChanged();
	void OnSelectTabByIndex(int iTab);
	void OnChangeMainFontSize(FontSizeType sizeType);
	void OnResetMainFontSize();

	/* Main menu handlers. */
	void OnNewTab();
	bool OnCloseTab();
	void OnSaveDirectoryListing() const;
	void OnCloneWindow();
	void OnCopyItemPath() const;
	void OnCopyUniversalPaths() const;
	void OnSetFileAttributes() const;
	void OnFileDelete(bool permanent);
	void OnFileRename();
	void OnShowFileProperties() const;
	void OnCopy(BOOL bCopy);
	void OnPaste();
	void OnPasteShortcut();
	void OnWildcardSelect(BOOL bSelect);
	void OnResolveLink();
	void OnLockToolbars();
	void OnChangeDisplayColors();
	void OnFilterResults();
	void OnSortBy(SortMode sortMode);
	void OnGroupBy(SortMode groupMode);
	void OnGroupByNone();
	void OnSortDirectionSelected(SortDirection direction);
	void OnGroupSortDirectionSelected(SortDirection direction);
	void OnShowHiddenFiles();
	void OnRefresh();
	void OnSelectColumns();
	void OnCreateNewFolder();
	void OnMergeFiles();
	void OnSplitFile();
	void OnDestroyFiles();
	void OnSearch();
	void OnCustomizeColors();
	void OnRunScript();
	void OnShowOptions();
	void OnSearchTabs();
	void OnOpenOnlineDocumentation();
	void OnCheckForUpdates();
	void OnAbout();

	HRESULT OnGoToOffset(int offset);
	HRESULT OnGoHome();

	int HighlightSimilarFiles(HWND ListView) const;

	/* Main toolbar-specific handlers. */
	void OnToolbarViews();

	/* ListView private message handlers. */
	void OnListViewDoubleClick(const NMITEMACTIVATE *eventInfo);
	LRESULT OnListViewKeyDown(LPARAM lParam);
	void OnShowListViewContextMenu(const POINT &ptScreen);
	void OnListViewBackgroundRClick(POINT *pCursorPos);
	void OnListViewBackgroundRClickWindows8OrGreater(POINT *pCursorPos);
	void OnListViewBackgroundRClickWindows7(POINT *pCursorPos);
	void OnListViewItemRClick(POINT *pCursorPos);
	void OnListViewCopyItemPath() const;
	void OnListViewCopyUniversalPaths() const;
	void OnListViewSetFileAttributes() const;
	void OnListViewPaste();

	/* TreeView private message handlers. */
	void OnTreeViewCopyItemPath() const;
	void OnTreeViewSetFileAttributes() const;
	void OnTreeViewCopyUniversalPaths() const;

	/* Tab backing. */
	void CreateTabBacking();
	void OnTabsInitialized();
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateTabToolbar();

	/* Tabs. */
	void InitializeTabs();
	boost::signals2::connection AddTabsInitializedObserver(
		const TabsInitializedSignal::slot_type &observer) override;
	void OnTabCreated(int tabId, BOOL switchToNewTab);
	void OnTabSelected(const Tab &tab);
	void ShowTabBar() override;
	void HideTabBar() override;
	HRESULT CreateInitialTabs();
	void RestorePreviousTabs();
	void CreateCommandLineTabs();
	void OnTabListViewSelectionChanged(const Tab &tab);

	/* TabNavigationInterface methods. */
	void CreateNewTab(NavigateParams &navigateParams, bool selected) override;
	void SelectTabById(int tabId) override;

	void OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams);

	// ShellBrowserEmbedder
	void OnShellBrowserCreated(ShellBrowser *shellBrowser) override;

	/* PluginInterface. */
	CoreInterface *GetCoreInterface() override;
	TabContainer *GetTabContainer() override;
	Plugins::PluginMenuManager *GetPluginMenuManager() override;
	UiTheming *GetUiTheming() override;
	AcceleratorUpdater *GetAccleratorUpdater() override;
	Plugins::PluginCommandManager *GetPluginCommandManager() override;

	/* Plugins. */
	void InitializePlugins();

	/* Menus. */
	wil::unique_hmenu InitializeRightClickMenu();
	void SetProgramMenuItemStates(HMENU hProgramMenu);

	// Main rebar
	void CreateMainRebarAndChildren();
	std::vector<InternalRebarBandInfo> InitializeMainRebarBands();
	InternalRebarBandInfo InitializeToolbarBand(UINT id, HWND toolbar, bool showBand);
	InternalRebarBandInfo InitializeNonToolbarBand(UINT id, HWND child, bool showBand);
	void UpdateMainRebarBandsFromLoadedInfo(std::vector<InternalRebarBandInfo> &mainRebarBands);
	void UpdateMainRebarBandFromLoadedInfo(InternalRebarBandInfo &internalBandInfo);
	void InsertMainRebarBand(const InternalRebarBandInfo &internalBandInfo);
	LRESULT RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void CreateFolderControls();
	void CreateAddressBar();
	void CreateMainToolbar();
	void CreateBookmarksToolbar();
	void CreateDrivesToolbar();
	void CreateApplicationToolbar();
	void OnAddressBarSizeUpdated();
	void OnRebarToolbarSizeUpdated(HWND toolbar);
	boost::signals2::connection AddToolbarContextMenuObserver(
		const ToolbarContextMenuSignal::slot_type &observer) override;
	boost::signals2::connection AddToolbarContextMenuSelectedObserver(
		const ToolbarContextMenuSelectedSignal::slot_type &observer) override;
	HMENU CreateRebarHistoryMenu(BOOL bBack);
	std::vector<RebarBandStorageInfo> GetMainRebarStorageInfo();

	/* Main toolbar private message handlers. */
	void OnToolbarRClick(HWND sourceWindow);
	void OnToolbarMenuItemSelected(HWND sourceWindow, int menuItemId);
	void OnToggleAddressBar();
	void OnToggleMainToolbar();
	void OnToggleBookmarksToolbar();
	void OnToggleDrivesToolbar();
	void OnToggleApplicationToolbar();
	void OnToggleToolbar(HWND toolbar, bool show);
	void OnCustomizeMainToolbar();

	/* Settings. */
	void SaveAllSettings() override;
	void LoadAllSettings();
	void ValidateLoadedSettings();
	void ApplyDisplayWindowPosition();
	void TestConfigFile();

	/* Registry settings. */
	LONG LoadGenericSettingsFromRegistry();
	LONG SaveGenericSettingsToRegistry();
	void SaveTabSettingsToRegistry();
	std::vector<TabStorageData> GetTabListStorageData();
	void LoadTabSettingsFromRegistry();
	void LoadDefaultColumnsFromRegistry();
	void SaveDefaultColumnsToRegistry();
	void LoadMainRebarInformationFromRegistry(HKEY mainKey);
	void SaveMainRebarInformationToRegistry(HKEY mainKey);

	/* XML Settings. */
	void LoadGenericSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveGenericSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void LoadTabSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveTabSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void LoadDefaultColumnsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveDefaultColumnsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveWindowPositionToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveWindowPositionToXMLInternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pWndPosNode);
	void LoadMainRebarInformationFromXML(IXMLDOMDocument *pXMLDom);
	void SaveMainRebarInformationToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void MapAttributeToValue(IXMLDOMNode *pNode, WCHAR *wszName, WCHAR *wszValue);

	/* Window state update. */
	void UpdateWindowStates(const Tab &tab);
	void SetListViewInitialPosition(HWND hListView) override;
	void ToggleFolders();
	void UpdateLayout();
	void OnTreeViewHolderResized(int newWidth);
	void ToggleDualPane();

	// Status bar
	void CreateStatusBar();
	void SetStatusBarParts(int width);
	void UpdateStatusBarMinHeight();
	LRESULT StatusBarMenuSelect(WPARAM wParam, LPARAM lParam);
	void OnNavigationStartedStatusBar(const Tab &tab, const NavigateParams &navigateParams);
	void SetStatusBarLoadingText(PCIDLIST_ABSOLUTE pidl);
	void OnNavigationCompletedStatusBar(const Tab &tab, const NavigateParams &navigateParams);
	void OnNavigationFailedStatusBar(const Tab &tab, const NavigateParams &navigateParams);
	HRESULT UpdateStatusBarText(const Tab &tab);
	std::wstring CreateDriveFreeSpaceString(const std::wstring &path);

	/* Languages. */
	void SetLanguageModule();
	BOOL VerifyLanguageVersion(const TCHAR *szLanguageModule) const;

	/* File operations. */
	void CopyToFolder(bool move);
	void OpenAllSelectedItems(
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);
	void OpenListViewItem(int index,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);

	// Navigator
	using Navigator::OpenItem;
	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition) override;

	void OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);
	void OpenFileItem(const std::wstring &itemPath, const std::wstring &parameters) override;
	void OpenFileItem(PCIDLIST_ABSOLUTE pidl, const std::wstring &parameters) override;

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);

	// FileContextMenuHandler
	void UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu) override;
	bool HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		int cmd) override;

	void UpdateBackgroundContextMenu(HMENU menu, IContextMenu *contextMenu);
	void UpdateItemContextMenu(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems);

	/* File selection tests. */
	BOOL AnyItemsSelected() const;
	bool CanCreate() const override;
	BOOL CanCut() const override;
	BOOL CanCopy() const override;
	BOOL CanRename() const override;
	BOOL CanDelete() const override;
	BOOL CanShowFileProperties() const override;
	BOOL CanPaste() const override;
	BOOL CanPasteShortcut() const;
	BOOL CanPasteShellData(PasteType pastType) const;
	BOOL CanPasteCustomData() const;
	BOOL TestItemAttributes(SFGAOF attributes) const;
	HRESULT GetSelectionAttributes(SFGAOF *pItemAttributes) const;

	HRESULT GetTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const;

	/* Display window. */
	void OnDisplayWindowResized(WPARAM wParam);
	void OnDisplayWindowRClick(POINT *ptClient);
	void UpdateDisplayWindow(const Tab &tab);
	void UpdateDisplayWindowForZeroFiles(const Tab &tab);
	void UpdateDisplayWindowForOneFile(const Tab &tab);
	void UpdateDisplayWindowForMultipleFiles(const Tab &tab);

	/* Columns. */
	void CopyColumnInfoToClipboard();

	/* CoreInterface methods. */
	const Config *GetConfig() const override;
	HINSTANCE GetResourceInstance() const override;
	HACCEL *GetAcceleratorTable() const override;
	HWND GetMainWindow() const override;
	ShellBrowserImpl *GetActiveShellBrowserImpl() const override;
	TabContainer *GetTabContainer() const override;
	TabRestorer *GetTabRestorer() const override;
	HWND GetTreeView() const override;
	IDirectoryMonitor *GetDirectoryMonitor() const override;
	IconResourceLoader *GetIconResourceLoader() const override;
	CachedIcons *GetCachedIcons() override;
	BOOL GetSavePreferencesToXmlFile() const override;
	void SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile) override;
	void FocusChanged() override;
	boost::signals2::connection AddApplicationInitializatedObserver(
		const ApplicationInitializedSignal::slot_type &observer) override;
	boost::signals2::connection AddFocusChangeObserver(
		const FocusChangedSignal::slot_type &observer) override;
	boost::signals2::connection AddDeviceChangeObserver(
		const DeviceChangeSignal::slot_type &observer) override;
	boost::signals2::connection AddApplicationShuttingDownObserver(
		const ApplicationShuttingDownSignal::slot_type &observer) override;

	/* Menus. */
	void InitializeMainMenu();
	void SetMainMenuImages();
	void InitializeGoMenu(HMENU mainMenu);
	void AddGoMenuItem(HMENU goMenu, UINT id, const KNOWNFOLDERID &folderId);
	void AddGoMenuItem(HMENU goMenu, UINT id, const std::wstring &path);
	void AddGoMenuItem(HMENU goMenu, UINT id, PCIDLIST_ABSOLUTE pidl);
	boost::signals2::connection AddMainMenuPreShowObserver(
		const MainMenuPreShowSignal::slot_type &observer) override;
	wil::unique_hmenu BuildViewsMenu() override;
	void AddViewModesToMenu(HMENU menu, UINT startPosition, BOOL byPosition);
	void OnInitMenu(HMENU menu);
	void OnExitMenuLoop(bool shortcutMenu);
	boost::signals2::connection AddMainMenuItemMiddleClickedObserver(
		const MainMenuItemMiddleClickedSignal::slot_type &observer) override;
	void OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);
	boost::signals2::connection AddMainMenuItemRightClickedObserver(
		const MainMenuItemRightClickedSignal::slot_type &observer) override;
	void OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt);
	boost::signals2::connection AddGetMenuItemHelperTextObserver(
		const GetMenuItemHelperTextSignal::slot_type &observer) override;

	// Dark mode
	static bool ShouldEnableDarkMode(Theme theme);
	void OnThemeUpdated(Theme theme);

	// Customize colors
	void InitializeDefaultColorRules();

	/* Miscellaneous. */
	void InitializeDisplayWindow();
	void ShowMainRebarBand(HWND hwnd, BOOL bShow);
	StatusBar *GetStatusBar() override;
	void StartDirectoryMonitoringForTab(const Tab &tab);
	void StopDirectoryMonitoringForTab(const Tab &tab);
	int DetermineListViewObjectIndex(HWND hListView);

	static void FolderSizeCallbackStub(int nFolders, int nFiles, PULARGE_INTEGER lTotalFolderSize,
		LPVOID pData);
	void FolderSizeCallback(FolderSizeExtraInfo *pfsei, int nFolders, int nFiles,
		PULARGE_INTEGER lTotalFolderSize);

	CommandLine::Settings m_commandLineSettings;

	BrowserCommandController m_commandController;

	HWND m_hContainer;
	HWND m_hStatusBar;
	HWND m_hMainRebar;
	HWND m_hDisplayWindow;
	HWND m_hTabBacking;

	HWND m_hTabWindowToolbar;
	wil::unique_himagelist m_tabWindowToolbarImageList;

	IDirectoryMonitor *m_pDirMon;

	HINSTANCE m_resourceInstance;

	/** Internal state. **/
	HWND m_lastActiveWindow;
	bool m_bLanguageLoaded;
	bool m_bShowTabBar;
	int m_iLastSelectedTab = 0;
	ULONG m_SHChangeNotifyID;

	/* Initialization. */
	BOOL m_bLoadSettingsFromXML;
	bool m_applicationInitialized = false;
	ApplicationInitializedSignal m_applicationInitializedSignal;

	MainWindow *m_mainWindow;
	AddressBar *m_addressBar;

	std::unique_ptr<IconResourceLoader> m_iconResourceLoader;

	CachedIcons m_cachedIcons;

	wil::com_ptr_nothrow<IImageList> m_mainMenuSystemImageList;
	std::vector<wil::unique_hbitmap> m_mainMenuImages;
	MainMenuPreShowSignal m_mainMenuPreShowSignal;
	MainMenuItemMiddleClickedSignal m_mainMenuItemMiddleClickedSignal;
	MainMenuItemRightClickedSignal m_mainMenuItemRightClickedSignal;
	bool m_mainMenuShowing = false;
	GetMenuItemHelperTextSignal m_getMenuItemHelperTextSignal;
	FocusChangedSignal m_focusChangedSignal;
	ApplicationShuttingDownSignal m_applicationShuttingDownSignal;
	bool m_applicationShuttingDown = false;

	// Treeview
	HolderWindow *m_treeViewHolder = nullptr;
	ShellTreeView *m_shellTreeView = nullptr;

	std::unique_ptr<BrowserPane> m_browserPane;

	/* Tabs. */
	std::vector<TabStorageData> m_loadedTabs;
	std::unique_ptr<MainFontSetter> m_tabToolbarTooltipFontSetter;
	wil::unique_hbrush m_tabBarBackgroundBrush;
	std::unique_ptr<TabRestorer> m_tabRestorer;
	std::unique_ptr<TabRestorerMenu> m_tabRestorerMenu;
	TabsInitializedSignal m_tabsInitializedSignal;

	ToolbarContextMenuSignal m_toolbarContextMenuSignal;
	ToolbarContextMenuSelectedSignal m_toolbarContextMenuSelectedSignal;

	/* Theming. */
	std::unique_ptr<ThemeWindowTracker> m_themeWindowTracker;
	std::unique_ptr<UiTheming> m_uiTheming;

	/* Plugins. */
	std::unique_ptr<Plugins::PluginManager> m_pluginManager;
	Plugins::PluginMenuManager m_pluginMenuManager;
	AcceleratorUpdater m_acceleratorUpdater;
	Plugins::PluginCommandManager m_pluginCommandManager;

	HWND m_hActiveListView;
	ShellBrowserImpl *m_pActiveShellBrowser;

	/* User options variables. */
	std::shared_ptr<Config> m_config;
	BOOL m_bSavePreferencesToXMLFile;

	bool m_themeValueLoadedFromXml = false;
	bool m_groupSortDirectionGlobalLoadedFromXml = false;

	TaskbarThumbnails *m_taskbarThumbnails;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	/* Bookmarks. */
	std::unique_ptr<BookmarksMainMenu> m_bookmarksMainMenu;
	BookmarksToolbar *m_bookmarksToolbar;

	// IconFetcher retrieves file icons in a background thread. A queue of requests is maintained
	// and that queue is cleared when the instance is destroyed. However, any current request that's
	// running in the background thread will continue to run and the main thread will wait for it to
	// finish.
	// This, however, could cause problems if the IconFetcher instance is held by an object that
	// only exists for a specific period of time. For example, if the instance were being managed by
	// the manage bookmark dialog, and the dialog was closed, the instance would be destroyed and
	// the main thread would wait for any operation in the background thread to finish. That could
	// then cause the application to appear to hang.
	// Since this class exists for the entire lifetime of the application, it means that the
	// IconFetcher instance will only be destroyed at application shutdown and that's the only point
	// at which the main thread will wait. While the application is running, the most that would
	// happen is that the process of retrieving other icons would be delayed.
	// Ideally, it would be better to cancel operations that are running in the background thread,
	// but as far as I'm aware, it's not possible to cancel SHGetFileInfo (which is what's
	// ultimately used to retrieve the icons).
	IconFetcherImpl m_iconFetcher;

	/* Undo support. */
	FileActionHandler m_FileActionHandler;

	// Main rebar
	std::vector<RebarBandStorageInfo> m_loadedRebarStorageInfo;

	/* Toolbars. */
	MainToolbar *m_mainToolbar;
	std::optional<MainToolbarStorage::MainToolbarButtons> m_loadedMainToolbarButtons;
	DrivesToolbar *m_drivesToolbar = nullptr;
	Applications::ApplicationToolbar *m_applicationToolbar = nullptr;

	/* Display window folder sizes. */
	std::list<DWFolderSize> m_DWFolderSizes;
	int m_iDWFolderSizeUniqueId;

	// WM_DEVICECHANGE notifications
	DeviceChangeSignal m_deviceChangeSignal;

	// Status bar
	StatusBar *m_pStatusBar = nullptr;
	std::unique_ptr<MainFontSetter> m_statusBarFontSetter;
};
