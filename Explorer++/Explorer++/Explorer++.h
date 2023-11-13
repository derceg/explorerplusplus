// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorUpdater.h"
#include "BrowserPane.h"
#include "CommandLine.h"
#include "CoreInterface.h"
#include "Literals.h"
#include "Navigator.h"
#include "PluginInterface.h"
#include "Plugins/PluginCommandManager.h"
#include "Plugins/PluginMenuManager.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/SortModes.h"
#include "Tab.h"
#include "TabNavigationInterface.h"
#include "ValueWrapper.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/IconFetcher.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <optional>

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED WM_APP + 3

/* Private definitions. */
#define FROM_LISTVIEW 0
#define FROM_TREEVIEW 1

// Forward declarations.
class AddressBar;
class BookmarksMainMenu;
class BookmarksToolbar;
struct ColumnWidth;
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
class ShellBrowser;
class ShellTreeView;
class TabContainer;
class TabRestorer;
class TabRestorerUI;
struct TabSettings;
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
	public CoreInterface,
	private FileContextMenuHandler,
	public Navigator,
	public PluginInterface,
	public TabNavigationInterface
{
	friend LoadSaveRegistry;
	friend LoadSaveXML;

public:
	Explorerplusplus(HWND hwnd, CommandLine::Settings *commandLineSettings);
	~Explorerplusplus();

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT CALLBACK RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT CALLBACK TreeViewHolderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK TreeViewSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/* Directory modification. */
	static void DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction, void *pData);

private:
	static constexpr UINT WM_APP_CLOSE = WM_APP + 1;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const UINT DISPLAY_WINDOW_MINIMUM_WIDTH = 70;
	static const UINT DISPLAY_WINDOW_MINIMUM_HEIGHT = 70;

	static constexpr auto TREEVIEW_MINIMUM_WIDTH = 70_px;
	static constexpr double TREEVIEW_MAXIMUM_WIDTH_PERCENTAGE = 0.8;

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

	static const UINT_PTR TREEVIEW_SELECTION_CHANGED_TIMER_ID = 0;
	static const UINT TREEVIEW_SELECTION_CHANGED_TIMEOUT = 500;

	static const UINT_PTR LISTVIEW_ITEM_CHANGED_TIMER_ID = 100001;
	static const UINT LISTVIEW_ITEM_CHANGED_TIMEOUT = 50;

	// Represents the maximum number of icons that can be cached. This cache is
	// shared between various components in the application.
	static const int MAX_CACHED_ICONS = 1000;

	static inline constexpr COLORREF TAB_BAR_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	// When changing the font size, it will be decreased/increased by this amount.
	static const int FONT_SIZE_CHANGE_DELTA = 1_pt;

	struct FileContextMenuInfo
	{
		UINT uFrom;
	};

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

	HRESULT OnGoBack();
	HRESULT OnGoForward();
	HRESULT OnGoToOffset(int offset);
	HRESULT OnGoHome();
	HRESULT OnGoToKnownFolder(REFKNOWNFOLDERID knownFolderId);
	HRESULT OnGoToPath(const std::wstring &path);
	HRESULT GoToPidl(PCIDLIST_ABSOLUTE pidl);

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
	void OnShowTreeViewContextMenu(const POINT &ptScreen);
	void OnTreeViewSelectionChanged(const NMTREEVIEW *eventInfo);
	void HandleTreeViewSelectionChanged(const NMTREEVIEW *eventInfo);
	void OnTreeViewCopyItemPath() const;
	void OnTreeViewSetFileAttributes() const;
	void OnTreeViewCopyUniversalPaths() const;

	/* Holder window private message handlers. */
	LRESULT CALLBACK TreeViewHolderWindowNotifyHandler(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam);
	void OnTreeViewSelectionChangedTimer();

	BrowserPane *GetActivePane() const;

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
	HRESULT RestoreTabs(ILoadSave *pLoadSave);
	void OnTabListViewSelectionChanged(const Tab &tab);

	/* TabNavigationInterface methods. */
	void CreateNewTab(NavigateParams &navigateParams, bool selected) override;
	void SelectTabById(int tabId) override;

	void OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams);

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

	/* Control creation. */
	void CreateMainControls();
	void CreateFolderControls();
	void CreateAddressBar();
	void CreateMainToolbar();
	void CreateBookmarksToolbar();
	void CreateDrivesToolbar();
	void CreateApplicationToolbar();

	// Rebar bands
	void InitializeMainToolbars();
	void OnAddressBarSizeUpdated();
	void OnRebarToolbarSizeUpdated(HWND toolbar);
	boost::signals2::connection AddToolbarContextMenuObserver(
		const ToolbarContextMenuSignal::slot_type &observer) override;
	boost::signals2::connection AddToolbarContextMenuSelectedObserver(
		const ToolbarContextMenuSelectedSignal::slot_type &observer) override;

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
	void LoadAllSettings(ILoadSave **pLoadSave);
	void ValidateLoadedSettings();
	void ValidateColumns(FolderColumns &folderColumns);
	void ValidateSingleColumnSet(int iColumnSet, std::vector<Column_t> &columns);
	void ApplyDisplayWindowPosition();
	void ApplyToolbarSettings();
	void TestConfigFile();

	/* Registry settings. */
	LONG LoadGenericSettingsFromRegistry();
	LONG SaveGenericSettingsToRegistry();
	void SaveTabSettingsToRegistry();
	int LoadTabSettingsFromRegistry();
	std::vector<Column_t> LoadColumnFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName);
	void SaveColumnToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName,
		std::vector<Column_t> *pColumns);
	std::vector<ColumnWidth> LoadColumnWidthsFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName);
	void SaveColumnWidthsToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName,
		std::vector<Column_t> *pColumns);
	void LoadDefaultColumnsFromRegistry();
	void SaveDefaultColumnsToRegistry();
	void SaveToolbarInformationToRegistry();
	void LoadToolbarInformationFromRegistry();

	/* XML Settings. */
	void LoadGenericSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveGenericSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	int LoadTabSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveTabSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveTabSettingsToXMLnternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	int LoadColumnFromXML(IXMLDOMNode *pNode, std::vector<Column_t> &outputColumns);
	void SaveColumnToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode,
		const std::vector<Column_t> &columns, const TCHAR *szColumnSet, int iIndent);
	void LoadDefaultColumnsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveDefaultColumnsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveDefaultColumnsToXMLInternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode);
	void SaveWindowPositionToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveWindowPositionToXMLInternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pWndPosNode);
	void LoadToolbarInformationFromXML(IXMLDOMDocument *pXMLDom);
	void SaveToolbarInformationToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveToolbarInformationToXMLnternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	void MapAttributeToValue(IXMLDOMNode *pNode, WCHAR *wszName, WCHAR *wszValue);
	void MapTabAttributeValue(WCHAR *wszName, WCHAR *wszValue, TabSettings &tabSettings,
		FolderSettings &folderSettings, bool &groupModeLoaded, bool &groupSortDirectionLoaded);

	/* Window state update. */
	void UpdateWindowStates(const Tab &tab);
	void UpdateTreeViewSelection();
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
	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab) override;
	OpenFolderDisposition DetermineOpenDisposition(bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown) override;

	void OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);
	void OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const TCHAR *szParameters) override;

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);
	HRESULT OnNavigateUp();

	// FileContextMenuHandler
	void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, IContextMenu *contextMenu,
		HMENU hMenu) override;
	BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, int iCmd) override;

	void UpdateBackgroundContextMenu(IContextMenu *contextMenu, HMENU menu);
	void UpdateItemContextMenu(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR data, HMENU menu);

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
	HWND GetActiveListView() const override;
	ShellBrowser *GetActiveShellBrowser() const override;
	TabContainer *GetTabContainer() const override;
	TabRestorer *GetTabRestorer() const override;
	HWND GetTreeView() const override;
	IDirectoryMonitor *GetDirectoryMonitor() const override;
	IconResourceLoader *GetIconResourceLoader() const override;
	CachedIcons *GetCachedIcons() override;
	BOOL GetSavePreferencesToXmlFile() const override;
	void SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile) override;
	void FocusChanged() override;
	boost::signals2::connection AddFocusChangeObserver(
		const FocusChangedSignal::slot_type &observer) override;
	boost::signals2::connection AddDeviceChangeObserver(
		const DeviceChangeSignal::slot_type &observer) override;
	boost::signals2::connection AddApplicationShuttingDownObserver(
		const ApplicationShuttingDownSignal::slot_type &observer) override;
	void FocusActiveTab() override;

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

	// Rebar
	HMENU CreateRebarHistoryMenu(BOOL bBack);

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
	bool m_bAttemptToolbarRestore;
	bool m_bLanguageLoaded;
	bool m_bShowTabBar;
	int m_iLastSelectedTab = 0;
	ULONG m_SHChangeNotifyID;
	ValueWrapper<bool> m_InitializationFinished;

	/* Initialization. */
	BOOL m_bLoadSettingsFromXML;

	MainWindow *m_mainWindow;
	AddressBar *m_addressBar;

	std::unique_ptr<IconResourceLoader> m_iconResourceLoader;

	CachedIcons m_cachedIcons;

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
	std::optional<NMTREEVIEW> m_treeViewSelectionChangedEventInfo;
	bool m_treeViewInitialized = false;

	std::unique_ptr<BrowserPane> m_browserPane;

	/* Tabs. */
	std::unique_ptr<MainFontSetter> m_tabToolbarTooltipFontSetter;
	wil::unique_hbrush m_tabBarBackgroundBrush;
	std::unique_ptr<TabRestorer> m_tabRestorer;
	std::unique_ptr<TabRestorerUI> m_tabRestorerUI;
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
	ShellBrowser *m_pActiveShellBrowser;

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
	IconFetcher m_bookmarkIconFetcher;

	/* Undo support. */
	FileActionHandler m_FileActionHandler;

	/* Toolbars. */
	REBARBANDINFO m_ToolbarInformation[NUM_MAIN_TOOLBARS];
	MainToolbar *m_mainToolbar;
	DrivesToolbar *m_drivesToolbar = nullptr;
	Applications::ApplicationToolbar *m_applicationToolbar = nullptr;

	/* Display window folder sizes. */
	std::list<DWFolderSize> m_DWFolderSizes;
	int m_iDWFolderSizeUniqueId;

	/* Menu images. */
	std::vector<wil::unique_hbitmap> m_menuImages;

	// WM_DEVICECHANGE notifications
	DeviceChangeSignal m_deviceChangeSignal;

	// Status bar
	StatusBar *m_pStatusBar = nullptr;
	std::unique_ptr<MainFontSetter> m_statusBarFontSetter;
};
