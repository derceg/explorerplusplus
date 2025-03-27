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
#include "LayoutDefaults.h"
#include "Literals.h"
#include "MainToolbarStorage.h"
#include "PluginInterface.h"
#include "Plugins/PluginCommandManager.h"
#include "Plugins/PluginMenuManager.h"
#include "RebarView.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/SortModes.h"
#include "ShellIconLoaderImpl.h"
#include "Tab.h"
#include "TabNavigationInterface.h"
#include "TabStorage.h"
#include "Theme.h"
#include "ValueWrapper.h"
#include "WindowStorage.h"
#include "../Helper/ClipboardHelper.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/ShellContextMenu.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <wil/resource.h>
#include <optional>

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED WM_APP + 3

// Forward declarations.
class AcceleratorManager;
class AddressBar;
class App;
class BookmarksMainMenu;
class BookmarksToolbar;
class BrowserTracker;
class CachedIcons;
struct Config;
class DisplayWindow;
class DrivesToolbar;
class FrequentLocationsMenu;
class HistoryMenu;
class HolderWindow;
__interface IDirectoryMonitor;
class ILoadSave;
class LoadSaveRegistry;
class LoadSaveXML;
class MainFontSetter;
class MainMenuSubMenuView;
class MainRebarView;
class MainToolbar;
class MainWindow;
class MenuBase;
class MenuView;
struct NavigateParams;
class NavigationRequest;
struct RebarBandStorageInfo;
class Runtime;
class ShellBrowserImpl;
class ShellTreeView;
class StatusBar;
class TabContainerImpl;
class TabRestorerMenu;
struct TabSettings;
class TaskbarThumbnails;
class ThemeWindowTracker;
class UiTheming;
class WindowSubclass;

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
	public PluginInterface,
	private ShellContextMenuHandler,
	public TabNavigationInterface
{
	friend LoadSaveRegistry;
	friend LoadSaveXML;

public:
	static constexpr wchar_t WINDOW_CLASS_NAME[] = L"Explorer++";

	static Explorerplusplus *Create(App *app, const WindowStorageData *storageData = nullptr);

	~Explorerplusplus();

	/* Directory modification. */
	static void DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction, void *pData);

	// BrowserWindow
	HWND GetHWND() const override;
	boost::signals2::connection AddBrowserInitializedObserver(
		const BrowserInitializedSignal::slot_type &observer) override;
	BrowserCommandController *GetCommandController() override;
	BrowserPane *GetActivePane() const override;
	void FocusActiveTab() override;
	void CreateTabFromPreservedTab(const PreservedTab *tab) override;

	// Note that there is also GetActiveShellBrowserImpl() declared below. There are some places
	// where this method can be used, as the calling code only uses methods from the ShellBrowser
	// interface. However, there are other places where implementation-only methods are used, in
	// which case GetActiveShellBrowserImpl() is required. That's the reason these two methods
	// exist.
	ShellBrowser *GetActiveShellBrowser() override;

	const ShellBrowser *GetActiveShellBrowser() const override;
	std::optional<std::wstring> RequestMenuHelpText(HMENU menu, UINT id) const override;
	WindowStorageData GetStorageData() const override;
	bool IsActive() const override;
	void Activate() override;
	void FocusChanged() override;
	void TryClose() override;
	void Close() override;

	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override;

private:
	static constexpr UINT WM_APP_CLOSE = WM_APP + 1;
	static constexpr UINT WM_APP_ASSOC_CHANGED = WM_APP + 2;

	static const int OPEN_IN_NEW_TAB_MENU_ITEM_ID = ShellContextMenu::MAX_SHELL_MENU_ID + 1;

	static const int DISPLAY_WINDOW_MINIMUM_WIDTH = 70_px;
	static const int DISPLAY_WINDOW_MINIMUM_HEIGHT = 70_px;

	static constexpr int TREEVIEW_MINIMUM_WIDTH = 70_px;
	static constexpr double TREEVIEW_MAXIMUM_WIDTH_PERCENTAGE = 0.8;

	// Main rebar band IDs. These are used to load and save data, so the values shouldn't be
	// changed.
	static const UINT REBAR_BAND_ID_MAIN_TOOLBAR = 0;
	static const UINT REBAR_BAND_ID_ADDRESS_BAR = 1;
	static const UINT REBAR_BAND_ID_BOOKMARKS_TOOLBAR = 2;
	static const UINT REBAR_BAND_ID_DRIVES_TOOLBAR = 3;
	static const UINT REBAR_BAND_ID_APPLICATIONS_TOOLBAR = 4;

	static const UINT_PTR LISTVIEW_ITEM_CHANGED_TIMER_ID = 100001;
	static const UINT LISTVIEW_ITEM_CHANGED_TIMEOUT = 50;

	static inline constexpr COLORREF TAB_BAR_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	// When changing the font size, it will be decreased/increased by this amount.
	static const int FONT_SIZE_CHANGE_DELTA = 1_pt;

	static constexpr wchar_t PLUGIN_FOLDER_NAME[] = L"plugins";

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

	struct MainMenuSubMenu
	{
		std::unique_ptr<MainMenuSubMenuView> view;
		std::unique_ptr<MenuBase> menu;
	};

	Explorerplusplus(App *app, const WindowStorageData *storageData);

	static HWND CreateMainWindow(const WindowStorageData *storageData);
	static ATOM RegisterMainWindowClass(HINSTANCE instance);

	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ListViewSubclassProc(HWND ListView, UINT msg, WPARAM wParam, LPARAM lParam);

	/* Main window message handlers. */
	LRESULT CALLBACK CommandHandler(HWND hwnd, HWND control, int id, UINT notificationCode);
	LRESULT HandleMenuOrToolbarButtonOrAccelerator(HWND hwnd, int id, UINT notificationCode);
	LRESULT HandleControlNotification(HWND hwnd, UINT notificationCode);
	LRESULT CALLBACK NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void Initialize(const WindowStorageData *storageData);
	bool OnActivate(int activationState, bool minimized);
	void OnSize(UINT state);
	static concurrencpp::null_result ScheduleUpdateLayout(WeakPtr<Explorerplusplus> self,
		Runtime *runtime);
	void UpdateLayout();
	void OnDpiChanged(const RECT *updatedWindowRect);
	std::optional<LRESULT> OnCtlColorStatic(HWND hwnd, HDC hdc);
	int OnDestroy();
	void OnDeviceChange(WPARAM wParam, LPARAM lParam);
	void OnFocusNextWindow(FocusChangeDirection direction);
	void OnAppCommand(UINT cmd);
	void OnDirectoryContentsChanged(const ShellBrowser *shellBrowser);
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
	void OnToggleLockToolbars();
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

	void OnGoToOffset(int offset);
	void OnGoHome();

	int HighlightSimilarFiles(HWND ListView) const;

	void CreateNewWindow(const std::vector<TabStorageData> &tabs = {});

	/* Main toolbar-specific handlers. */
	void OnToolbarViews();

	/* ListView private message handlers. */
	void OnListViewClick(const NMITEMACTIVATE *eventInfo);
	void OnListViewDoubleClick(const NMITEMACTIVATE *eventInfo);
	LRESULT OnListViewKeyDown(LPARAM lParam);
	void OnShowListViewContextMenu(const POINT &ptScreen);
	void OnListViewBackgroundRClick(POINT *pCursorPos);
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
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateTabToolbar();

	/* Tabs. */
	void InitializeTabs();
	void OnTabCreated(const Tab &tab, bool selected);
	void OnTabSelected(const Tab &tab);
	void ShowTabBar() override;
	void HideTabBar() override;
	void CreateInitialTabs(const WindowStorageData *storageData);
	void CreateTabsFromStorageData(const WindowStorageData &storageData);
	void CreateCommandLineTabs();
	void OnTabListViewSelectionChanged(const ShellBrowser *shellBrowser);

	/* TabNavigationInterface methods. */
	void CreateNewTab(NavigateParams &navigateParams, bool selected) override;
	void SelectTabById(int tabId) override;

	void OnNavigationCommitted(const NavigationRequest *request);

	/* PluginInterface. */
	CoreInterface *GetCoreInterface() override;
	TabEvents *GetTabEvents() override;
	TabContainerImpl *GetTabContainerImpl() override;
	Plugins::PluginMenuManager *GetPluginMenuManager() override;
	UiTheming *GetUiTheming() override;
	AcceleratorUpdater *GetAccleratorUpdater() override;
	Plugins::PluginCommandManager *GetPluginCommandManager() override;

	/* Plugins. */
	void InitializePlugins();

	/* Menus. */
	void SetProgramMenuItemStates(HMENU hProgramMenu);

	// Main rebar
	void CreateMainRebarAndChildren(const WindowStorageData *storageData);
	std::vector<RebarView::Band> InitializeMainRebarBands(const WindowStorageData *storageData);
	RebarView::Band InitializeToolbarBand(UINT id, HWND toolbar, bool showBand);
	RebarView::Band InitializeNonToolbarBand(UINT id, HWND child, bool showBand);
	void UpdateMainRebarBandsFromLoadedInfo(std::vector<RebarView::Band> &mainRebarBands,
		const std::vector<RebarBandStorageInfo> &rebarStorageInfo);
	void UpdateMainRebarBandFromLoadedInfo(RebarView::Band &band,
		const std::vector<RebarBandStorageInfo> &rebarStorageInfo);
	LRESULT RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void CreateFolderControls();
	void CreateAddressBar();
	void CreateMainToolbar(
		const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons);
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

	/* Main toolbar private message handlers. */
	void OnToolbarRClick(HWND sourceWindow);
	void OnToolbarMenuItemSelected(HWND sourceWindow, int menuItemId);
	void OnToggleAddressBar();
	void OnToggleMainToolbar();
	void OnToggleBookmarksToolbar();
	void OnToggleDrivesToolbar();
	void OnToggleApplicationToolbar();
	void OnCustomizeMainToolbar();

	/* Settings. */
	void ApplyDisplayWindowPosition();

	/* Window state update. */
	void UpdateWindowStates(const Tab &tab);
	void ToggleFolders();
	void OnTreeViewHolderResized(int newWidth);
	void ToggleDualPane();

	/* File operations. */
	void CopyToFolder(bool move);
	void OpenAllSelectedItems(
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);
	void OpenListViewItem(int index,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);

	// Navigator
	using Navigator::OpenDefaultItem;
	using Navigator::OpenItem;
	void OpenDefaultItem(OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition) override;

	void OpenShortcutItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition);
	bool ShouldOpenContainerFile(PCIDLIST_ABSOLUTE pidlItem);

	void OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);
	void OpenFileItem(const std::wstring &itemPath, const std::wstring &parameters) override;
	void OpenFileItem(PCIDLIST_ABSOLUTE pidl, const std::wstring &parameters) override;

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);

	// FileContextMenuHandler
	void UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu) override;
	std::wstring GetHelpTextForItem(UINT menuItemId) override;
	bool HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		UINT menuItemId) override;

	void UpdateBackgroundContextMenu(HMENU menu, PCIDLIST_ABSOLUTE folderPidl,
		IContextMenu *contextMenu);
	void RemoveNonFunctionalItemsFromBackgroundContextMenu(HMENU menu, IContextMenu *contextMenu);
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
	BOOL CanPaste(PasteType pasteType) const override;
	bool CanPasteLink() const;
	BOOL TestItemAttributes(SFGAOF attributes) const;
	HRESULT GetSelectionAttributes(SFGAOF *pItemAttributes) const;
	PidlAbsolute MaybeGetFocusedDirectory() const;

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
	HWND GetMainWindow() const override;
	ShellBrowserImpl *GetActiveShellBrowserImpl() const override;
	TabContainerImpl *GetTabContainerImpl() const override;
	HWND GetTreeView() const override;
	IDirectoryMonitor *GetDirectoryMonitor() const override;
	CachedIcons *GetCachedIcons() override;
	boost::signals2::connection AddFocusChangeObserver(
		const FocusChangedSignal::slot_type &observer) override;
	boost::signals2::connection AddDeviceChangeObserver(
		const DeviceChangeSignal::slot_type &observer) override;

	/* Menus. */
	void InitializeMainMenu();
	void SetMainMenuImages();
	void SetPasteSymLinkElevationIcon();
	void InitializeGoMenu(HMENU mainMenu);
	void AddGoMenuItem(HMENU goMenu, UINT id, const KNOWNFOLDERID &folderId);
	void AddGoMenuItem(HMENU goMenu, UINT id, const std::wstring &path);
	void AddGoMenuItem(HMENU goMenu, UINT id, PCIDLIST_ABSOLUTE pidl);
	void AddMainMenuSubmenu(HMENU mainMenu, UINT subMenuItemId,
		std::function<std::unique_ptr<MenuBase>(MenuView *menuView)> menuCreator);
	boost::signals2::connection AddMainMenuPreShowObserver(
		const MainMenuPreShowSignal::slot_type &observer) override;
	wil::unique_hmenu BuildViewsMenu() override;
	void AddViewModesToMenu(HMENU menu, UINT startPosition, BOOL byPosition);
	void OnInitMenu(HMENU menu);
	void OnExitMenuLoop(bool shortcutMenu);
	bool MaybeHandleMainMenuItemSelection(UINT id);
	boost::signals2::connection AddMainMenuItemMiddleClickedObserver(
		const MainMenuItemMiddleClickedSignal::slot_type &observer) override;
	void OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);
	boost::signals2::connection AddMainMenuItemRightClickedObserver(
		const MainMenuItemRightClickedSignal::slot_type &observer) override;
	void OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt);
	std::optional<std::wstring> MaybeGetMenuHelpText(HMENU menu, int id);
	MainMenuSubMenu *MaybeGetMainMenuSubMenuFromId(UINT id);

	/* Miscellaneous. */
	void InitializeDisplayWindow();
	void StartDirectoryMonitoringForTab(const Tab &tab);
	void StopDirectoryMonitoringForTab(const Tab &tab);
	int DetermineListViewObjectIndex(HWND hListView);

	static void FolderSizeCallbackStub(int nFolders, int nFiles, PULARGE_INTEGER lTotalFolderSize,
		LPVOID pData);
	void FolderSizeCallback(FolderSizeExtraInfo *pfsei, int nFolders, int nFiles,
		PULARGE_INTEGER lTotalFolderSize);

	bool ConfirmClose();

	App *const m_app;
	HWND m_hContainer;
	std::unique_ptr<BrowserTracker> m_browserTracker;

	BrowserCommandController m_commandController;

	HWND m_hTabBacking;

	HWND m_hTabWindowToolbar;
	wil::unique_himagelist m_tabWindowToolbarImageList;

	IDirectoryMonitor *m_pDirMon;

	/** Internal state. **/
	HWND m_lastActiveWindow;
	bool m_bShowTabBar;
	ULONG m_SHChangeNotifyID;

#if DCHECK_IS_ON()
	bool m_performingLayout = false;
#endif

	/* Initialization. */
	bool m_browserInitialized = false;
	BrowserInitializedSignal m_browserInitializedSignal;

	MainWindow *m_mainWindow = nullptr;
	AddressBar *m_addressBar = nullptr;
	StatusBar *m_statusBar = nullptr;

	DisplayWindow *m_displayWindow = nullptr;
	int m_displayWindowWidth = LayoutDefaults::DEFAULT_DISPLAY_WINDOW_WIDTH;
	int m_displayWindowHeight = LayoutDefaults::DEFAULT_DISPLAY_WINDOW_HEIGHT;

	wil::com_ptr_nothrow<IImageList> m_mainMenuSystemImageList;
	std::vector<wil::unique_hbitmap> m_mainMenuImages;
	MainMenuPreShowSignal m_mainMenuPreShowSignal;
	MainMenuItemMiddleClickedSignal m_mainMenuItemMiddleClickedSignal;
	MainMenuItemRightClickedSignal m_mainMenuItemRightClickedSignal;
	bool m_mainMenuShowing = false;
	std::vector<MainMenuSubMenu> m_mainMenuSubMenus;

	MenuHelpTextRequestSignal m_menuHelpTextRequestSignal;

	FocusChangedSignal m_focusChangedSignal;
	bool m_browserClosing = false;

	// Treeview
	HolderWindow *m_treeViewHolder = nullptr;
	ShellTreeView *m_shellTreeView = nullptr;
	int m_treeViewWidth = LayoutDefaults::DEFAULT_TREEVIEW_WIDTH;

	std::unique_ptr<BrowserPane> m_browserPane;

	/* Tabs. */
	std::unique_ptr<MainFontSetter> m_tabToolbarTooltipFontSetter;
	wil::unique_hbrush m_tabBarBackgroundBrush;

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
	Config *const m_config;

	std::unique_ptr<TaskbarThumbnails> m_taskbarThumbnails;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
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
	ShellIconLoaderImpl m_shellIconLoader;

	/* Undo support. */
	FileActionHandler m_FileActionHandler;

	// Main rebar
	MainRebarView *m_mainRebarView = nullptr;
	std::vector<boost::signals2::scoped_connection> m_rebarConnections;

	/* Toolbars. */
	MainToolbar *m_mainToolbar;
	DrivesToolbar *m_drivesToolbar = nullptr;
	Applications::ApplicationToolbar *m_applicationToolbar = nullptr;

	/* Display window folder sizes. */
	std::list<DWFolderSize> m_DWFolderSizes;
	int m_iDWFolderSizeUniqueId;

	// WM_DEVICECHANGE notifications
	DeviceChangeSignal m_deviceChangeSignal;

	WeakPtrFactory<Explorerplusplus> m_weakPtrFactory;
};
