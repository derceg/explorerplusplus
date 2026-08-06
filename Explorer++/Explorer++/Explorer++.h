// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorUpdater.h"
#include "ApplicationExecutorImpl.h"
#include "BrowserCommandController.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "CoreInterface.h"
#include "IconFetcherImpl.h"
#include "LayoutDefaults.h"
#include "Literals.h"
#include "MainToolbarStorage.h"
#include "PluginInterface.h"
#include "Plugins/PluginCommandManager.h"
#include "Plugins/PluginMenuManager.h"
#include "RebarView.h"
#include "ShellBrowser/ShellBrowserFactoryImpl.h"
#include "ShellBrowser/SortModes.h"
#include "ShellIconLoaderImpl.h"
#include "Tab.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include "../Helper/ClipboardHelper.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <wil/resource.h>
#include <optional>

/* Sent when a folder size calculation has finished. */
#define WM_APP_FOLDERSIZECOMPLETED WM_APP + 3

class AddressBar;
class App;
class BookmarksMainMenu;
class BookmarksToolbar;
struct Config;
class DisplayWindow;
class DrivesToolbar;
class HistoryMenu;
class HolderWindow;
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
class TabBacking;
class TabContainer;
class TaskbarThumbnails;
class ThemeWindowTracker;
class PreviewPane;
class WindowSubclass;

namespace Applications
{
class ApplicationToolbar;
}

namespace Plugins
{
class PluginManager;
}

class Explorerplusplus : public BrowserWindow, public CoreInterface, public PluginInterface
{
public:
	static constexpr wchar_t WINDOW_CLASS_NAME[] = L"Explorer++";

	static Explorerplusplus *Create(App *app, const WindowStorageData *storageData = nullptr);

	// BrowserWindow
	HWND GetHWND() const override;
	BrowserCommandController *GetCommandController() override;
	BrowserPane *GetActivePane() const override;
	TabContainer *GetActiveTabContainer() override;
	const TabContainer *GetActiveTabContainer() const override;
	void FocusActiveTab() override;
	Tab *CreateTabFromPreservedTab(const PreservedTab *tab) override;
	using BrowserWindow::OpenDefaultItem;
	using BrowserWindow::OpenItem;
	void OpenDefaultItem(OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition) override;
	void OpenFileItem(const std::wstring &itemPath, const std::wstring &parameters) override;
	void OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const std::wstring &parameters) override;

	// Note that there is also GetActiveShellBrowserImpl() declared below. There are some places
	// where this method can be used, as the calling code only uses methods from the ShellBrowser
	// interface. However, there are other places where implementation-only methods are used, in
	// which case GetActiveShellBrowserImpl() is required. That's the reason these two methods
	// exist.
	ShellBrowser *GetActiveShellBrowser() override;

	const ShellBrowser *GetActiveShellBrowser() const override;
	void StartMainToolbarCustomization() override;
	std::optional<std::wstring> RequestMenuHelpText(HMENU menu, UINT id) const override;
	WindowStorageData GetStorageData() const override;
	bool IsActive() const override;
	void Activate() override;
	void TryClose() override;
	void Close() override;

	// MenuHelpTextHost
	void MenuItemSelected(HMENU menu, UINT itemId, UINT flags) override;
	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override;

private:
	static constexpr UINT WM_APP_CLOSE = WM_APP + 1;

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

	static constexpr wchar_t PLUGIN_FOLDER_NAME[] = L"plugins";

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

	struct MainMenuSubMenu
	{
		std::unique_ptr<MainMenuSubMenuView> view;
		std::unique_ptr<MenuBase> menu;
	};

	Explorerplusplus(App *app, const WindowStorageData *storageData);
	~Explorerplusplus();

	static HWND CreateMainWindow(const WindowStorageData *storageData);
	static ATOM RegisterMainWindowClass(HINSTANCE instance);

	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void SetUpControlVisibilityConfigListeners();

	/* Main window message handlers. */
	LRESULT CommandHandler(HWND hwnd, HWND control, UINT id, UINT notificationCode);
	LRESULT HandleMenuOrToolbarButtonOrAccelerator(HWND hwnd, UINT id, UINT notificationCode);
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
	void OnFocusNextWindow(FocusChangeDirection direction);
	void OnAppCommand(UINT cmd);
	void OnDirectoryContentsChanged(const ShellBrowser *shellBrowser);

	/* Main menu handlers. */
	void OnNewTab();
	void OnCloneWindow();
	void OnPaste();
	void OnPasteShortcut();
	void OnResolveLink();
	void OnGroupBy(SortMode groupMode);
	void OnGroupByNone();
	void OnGroupSortDirectionSelected(SortDirection direction);
	void OnShowHiddenFiles();
	void OnSelectColumns();
	void OnDestroyFiles();
	void OnSearch();
	void OnCustomizeColors();
	void OnRunScript();
	void OnShowOptions();
	void OnSearchTabs();

	void OnGoToOffset(int offset);

	int HighlightSimilarFiles(HWND ListView) const;

	void CreateNewWindow(const std::vector<TabStorageData> &tabs = {});

	/* Main toolbar-specific handlers. */
	void OnToolbarViews();

	/* ListView private message handlers. */
	LRESULT OnListViewKeyDown(LPARAM lParam);
	void OnListViewPaste();

	/* Tabs. */
	void InitializeTabs();
	void MaybeUpdateTabBarVisibility();
	void OnTabCreated(const Tab &tab);
	void OnTabSelected(const Tab &tab);
	void OnTabPreRemoval(const Tab &tab, int index);
	void OnTabRemoved(const Tab &tab);
	static concurrencpp::null_result ScheduleFinishShutdown(WeakPtr<Explorerplusplus> self,
		Runtime *runtime);
	void ShowTabBar();
	void HideTabBar();
	void CreateInitialTabs(const WindowStorageData *storageData);
	void CreateTabsFromStorageData(const WindowStorageData &storageData);
	void CreateCommandLineTabs();
	void OnTabListViewSelectionChanged(const ShellBrowser *shellBrowser);

	void OnNavigationCommitted(const NavigationRequest *request);

	/* PluginInterface. */
	TabEvents *GetTabEvents() override;
	TabContainer *GetTabContainer() override;
	Plugins::PluginMenuManager *GetPluginMenuManager() override;
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
	HMENU CreateRebarHistoryMenu(BOOL bBack);
	bool OnToolbarRightClick(const NMMOUSE *mouseInfo);

	/* Settings. */
	void ApplyDisplayWindowPosition();

	/* Window state update. */
	void UpdateWindowStates(const Tab &tab);
	void OnTreeViewHolderResized(int newWidth);

	void OpenShortcutItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition);

	void OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab);

	void OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory);

	/* File selection tests. */
	BOOL CanPaste(PasteType pasteType) const override;
	bool CanPasteLink() const;
	PidlAbsolute MaybeGetFocusedDirectory() const;

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
	HWND GetMainWindow() const override;
	ShellBrowserImpl *GetActiveShellBrowserImpl() const override;
	TabContainer *GetTabContainer() const override;

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
	void OnInitMenu(HMENU menu);
	void OnExitMenuLoop(bool shortcutMenu);
	void OnInitMenuPopup(HMENU menu);
	void OnUninitMenuPopup(HMENU menu);
	bool MaybeHandleMainMenuItemSelection(UINT id);
	boost::signals2::connection AddMainMenuItemMiddleClickedObserver(
		const MainMenuItemMiddleClickedSignal::slot_type &observer) override;
	void OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);
	boost::signals2::connection AddMainMenuItemRightClickedObserver(
		const MainMenuItemRightClickedSignal::slot_type &observer) override;
	void OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt);
	MainMenuSubMenu *MaybeGetMainMenuSubMenuFromId(UINT id);

	/* Miscellaneous. */
	void InitializeDisplayWindow();
	void InitializePreviewPane();
	void UpdatePreviewPane(const Tab &tab);

	static void FolderSizeCallbackStub(int nFolders, int nFiles, PULARGE_INTEGER lTotalFolderSize,
		LPVOID pData);
	void FolderSizeCallback(FolderSizeExtraInfo *pfsei, int nFolders, int nFiles,
		PULARGE_INTEGER lTotalFolderSize);

	bool ConfirmClose();
	void BeginShutdown();
	void FinishShutdown();

	App *const m_app;
	HWND m_hContainer;

	BrowserCommandController m_commandController;

	/** Internal state. **/
	HWND m_lastActiveWindow;
	bool m_bShowTabBar;
	ULONG m_SHChangeNotifyID;

	// Undo support.
	FileActionHandler m_fileActionHandler;

#if DCHECK_IS_ON()
	bool m_performingLayout = false;
#endif

	MainWindow *m_mainWindow = nullptr;
	AddressBar *m_addressBar = nullptr;
	TabBacking *m_tabBacking = nullptr;
	StatusBar *m_statusBar = nullptr;

	DisplayWindow *m_displayWindow = nullptr;
	int m_displayWindowWidth = LayoutDefaults::DEFAULT_DISPLAY_WINDOW_WIDTH;
	int m_displayWindowHeight = LayoutDefaults::DEFAULT_DISPLAY_WINDOW_HEIGHT;

	// Preview pane
	PreviewPane *m_previewPane = nullptr;
	int m_previewPaneWidth = LayoutDefaults::DEFAULT_PREVIEW_PANE_WIDTH;

	wil::com_ptr_nothrow<IImageList> m_mainMenuSystemImageList;
	std::vector<wil::unique_hbitmap> m_mainMenuImages;
	MainMenuPreShowSignal m_mainMenuPreShowSignal;
	MainMenuItemMiddleClickedSignal m_mainMenuItemMiddleClickedSignal;
	MainMenuItemRightClickedSignal m_mainMenuItemRightClickedSignal;
	bool m_mainMenuShowing = false;
	std::vector<MainMenuSubMenu> m_mainMenuSubMenus;

	MenuHelpTextRequestSignal m_menuHelpTextRequestSignal;

	// Treeview
	HolderWindow *m_treeViewHolder = nullptr;
	ShellTreeView *m_shellTreeView = nullptr;
	int m_treeViewWidth = LayoutDefaults::DEFAULT_TREEVIEW_WIDTH;

	std::unique_ptr<BrowserPane> m_browserPane;

	/* Tabs. */
	wil::unique_hbrush m_tabBarBackgroundBrush;

	/* Theming. */
	std::unique_ptr<ThemeWindowTracker> m_themeWindowTracker;

	/* Plugins. */
	std::unique_ptr<Plugins::PluginManager> m_pluginManager;
	Plugins::PluginMenuManager m_pluginMenuManager;
	AcceleratorUpdater m_acceleratorUpdater;
	Plugins::PluginCommandManager m_pluginCommandManager;

	HWND m_hActiveListView;
	ShellBrowserImpl *m_pActiveShellBrowser;
	ShellBrowserFactoryImpl m_shellBrowserFactory;

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

	// Main rebar
	MainRebarView *m_mainRebarView = nullptr;
	std::vector<boost::signals2::scoped_connection> m_rebarConnections;

	/* Toolbars. */
	MainToolbar *m_mainToolbar;
	DrivesToolbar *m_drivesToolbar = nullptr;
	Applications::ApplicationExecutorImpl m_applicationExecutor;
	Applications::ApplicationToolbar *m_applicationToolbar = nullptr;

	/* Display window folder sizes. */
	std::list<DWFolderSize> m_DWFolderSizes;
	int m_iDWFolderSizeUniqueId;

	WeakPtrFactory<Explorerplusplus> m_weakPtrFactory{ this };
};
