// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorUpdater.h"
#include "Bookmarks/BookmarkTree.h"
#include "CoreInterface.h"
#include "Navigation.h"
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
class ApplicationToolbar;
class BookmarksMainMenu;
class BookmarksToolbar;
struct ColumnWidth;
struct Config;
class DrivesToolbar;
class IconResourceLoader;
__interface IDirectoryMonitor;
class ILoadSave;
class LoadSaveRegistry;
class LoadSaveXML;
class MainToolbar;
class MainWindow;
class ShellBrowser;
class ShellTreeView;
class TabContainer;
class TabRestorer;
class TabRestorerUI;
struct TabSettings;
class TaskbarThumbnails;
class UiTheming;
class WindowSubclassWrapper;

namespace NColorRuleHelper
{
	struct ColorRule;
}

namespace Plugins
{
	class PluginManager;
}

class Explorerplusplus :
	public IExplorerplusplus,
	public TabNavigationInterface,
	public IFileContextMenuExternal,
	public PluginInterface
{
	friend LoadSaveRegistry;
	friend LoadSaveXML;

public:
	Explorerplusplus(HWND);
	~Explorerplusplus();

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	LRESULT CALLBACK RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT CALLBACK TreeViewHolderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK TreeViewSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/* Directory modification. */
	static void DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction, void *pData);

private:
	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const UINT MINIMUM_DISPLAYWINDOW_WIDTH = 70;
	static const UINT MINIMUM_DISPLAYWINDOW_HEIGHT = 70;

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

	static inline constexpr COLORREF TAB_BAR_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	static inline const int CLOSE_TOOLBAR_WIDTH = 24;
	static inline const int CLOSE_TOOLBAR_HEIGHT = 24;

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

	LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ListViewSubclassProc(HWND ListView, UINT msg, WPARAM wParam, LPARAM lParam);

	/* Main window message handlers. */
	LRESULT CALLBACK CommandHandler(HWND hwnd, WPARAM wParam);
	LRESULT HandleMenuOrAccelerator(HWND hwnd, WPARAM wParam);
	LRESULT HandleControlNotification(HWND hwnd, WPARAM wParam);
	LRESULT CALLBACK NotifyHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnCreate();
	BOOL OnSize(int MainWindowWidth, int MainWindowHeight);
	void OnDpiChanged(const RECT *updatedWindowRect);
	std::optional<LRESULT> OnCtlColorStatic(HWND hwnd, HDC hdc);
	int OnClose();
	int OnDestroy();
	void OnRightClick(NMHDR *nmhdr);
	void OnSetFocus();
	LRESULT OnDeviceChange(WPARAM wParam, LPARAM lParam);
	LRESULT StatusBarMenuSelect(WPARAM wParam, LPARAM lParam);
	void OnStartedBrowsing(int iTabId, const TCHAR *szPath);
	void OnPreviousWindow();
	void OnNextWindow();
	void OnShellNewItemCreated(LPARAM lParam);
	void OnAppCommand(UINT cmd);
	void OnDirectoryModified(int iTabId);
	void OnIdaRClick();
	void OnAssocChanged();
	LRESULT OnCustomDraw(LPARAM lParam);
	void OnSelectTabByIndex(int iTab);

	/* Main menu handlers. */
	HRESULT OnNewTab();
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
	void OnWildcardSelect(BOOL bSelect);
	void OnResolveLink();
	void OnLockToolbars();
	void OnChangeDisplayColors();
	void OnFilterResults();
	void OnSortBy(SortMode sortMode);
	void OnGroupBy(SortMode sortMode);
	void OnSortByAscending(BOOL bSortAscending);
	void OnShowHiddenFiles();
	void OnRefresh();
	void OnSelectColumns();
	void OnAutoSizeColumns();
	void OnCreateNewFolder();
	void OnMergeFiles();
	void OnSplitFile();
	void OnDestroyFiles();
	void OnSearch();
	void OnCustomizeColors();
	void OnRunScript();
	void OnShowOptions();
	void OnShowHelp();
	void OnCheckForUpdates();
	void OnAbout();

	HRESULT OnGoBack();
	HRESULT OnGoForward();
	HRESULT OnGoToOffset(int offset);
	HRESULT OnGoHome();
	HRESULT OnGoToKnownFolder(REFKNOWNFOLDERID knownFolderId);

	int HighlightSimilarFiles(HWND ListView) const;

	/* Main toolbar-specific handlers. */
	void OnToolbarViews();

	/* ListView private message handlers. */
	void OnListViewDoubleClick(NMHDR *nmhdr);
	LRESULT OnListViewKeyDown(LPARAM lParam);
	BOOL OnListViewItemChanging(const NMLISTVIEW *changeData);
	HRESULT OnListViewBeginDrag(LPARAM lParam, DragType dragType);
	BOOL OnListViewBeginLabelEdit(LPARAM lParam);
	BOOL OnListViewEndLabelEdit(LPARAM lParam);
	void OnListViewRClick(POINT *pCursorPos);
	void OnListViewBackgroundRClick(POINT *pCursorPos);
	void OnListViewItemRClick(POINT *pCursorPos);
	void OnListViewCopyItemPath() const;
	void OnListViewCopyUniversalPaths() const;
	void OnListViewSetFileAttributes() const;
	void OnListViewPaste();

	/* TreeView private message handlers. */
	void OnTreeViewRightClick(WPARAM wParam, LPARAM lParam);
	void OnTreeViewSelChanged(LPARAM lParam);
	void OnTreeViewCopyItemPath() const;
	void OnTreeViewSetFileAttributes() const;
	void OnTreeViewCopyUniversalPaths() const;

	/* Holder window private message handlers. */
	std::optional<LRESULT> OnHolderCtlColorStatic(HWND hwnd, HDC hdc);
	LRESULT CALLBACK TreeViewHolderWindowNotifyHandler(
		HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnTreeViewHolderWindowTimer();

	LRESULT CALLBACK FoldersToolbarParentProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
	HRESULT CreateNewTab(PCIDLIST_ABSOLUTE pidlDirectory, bool selected) override;

	void OnNavigationCompleted(const Tab &tab);

	/* PluginInterface. */
	IExplorerplusplus *GetCoreInterface() override;
	TabContainer *GetTabContainer() override;
	Navigation *GetNavigation() override;
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
	HWND CreateTabToolbar(HWND hParent, int idCommand, const std::wstring &tip);

	/* Main toolbars. */
	void InitializeMainToolbars();
	void OnUseLargeToolbarIconsUpdated(BOOL newValue);
	boost::signals2::connection AddToolbarContextMenuObserver(
		const ToolbarContextMenuSignal::slot_type &observer) override;

	/* Main toolbar private message handlers. */
	void OnToolbarRClick(HWND sourceWindow);

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
	void SaveColumnToRegistry(
		HKEY hColumnsKey, const TCHAR *szKeyName, std::vector<Column_t> *pColumns);
	std::vector<ColumnWidth> LoadColumnWidthsFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName);
	void SaveColumnWidthsToRegistry(
		HKEY hColumnsKey, const TCHAR *szKeyName, std::vector<Column_t> *pColumns);
	void LoadDefaultColumnsFromRegistry();
	void SaveDefaultColumnsToRegistry();
	void SaveBookmarksToRegistry();
	void LoadBookmarksFromRegistry();
	void LoadApplicationToolbarFromRegistry();
	void SaveApplicationToolbarToRegistry();
	void SaveToolbarInformationToRegistry();
	void LoadToolbarInformationFromRegistry();
	void SaveDialogStatesToRegistry();
	void LoadDialogStatesFromRegistry();

	/* XML Settings. */
	void LoadGenericSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveGenericSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	int LoadTabSettingsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveTabSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveTabSettingsToXMLnternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	int LoadColumnFromXML(IXMLDOMNode *pNode, std::vector<Column_t> &outputColumns);
	void SaveColumnToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode,
		const std::vector<Column_t> &columns, const TCHAR *szColumnSet, int iIndent);
	void LoadBookmarksFromXML(IXMLDOMDocument *pXMLDom);
	void SaveBookmarksToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void LoadDefaultColumnsFromXML(IXMLDOMDocument *pXMLDom);
	void SaveDefaultColumnsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveDefaultColumnsToXMLInternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pColumnsNode);
	void SaveWindowPositionToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveWindowPositionToXMLInternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pWndPosNode);
	void LoadApplicationToolbarFromXML(IXMLDOMDocument *pXMLDom);
	void SaveApplicationToolbarToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void LoadToolbarInformationFromXML(IXMLDOMDocument *pXMLDom);
	void SaveToolbarInformationToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void SaveToolbarInformationToXMLnternal(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	void LoadDialogStatesFromXML(IXMLDOMDocument *pXMLDom);
	void SaveDialogStatesToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot);
	void MapAttributeToValue(IXMLDOMNode *pNode, WCHAR *wszName, WCHAR *wszValue);
	void MapTabAttributeValue(
		WCHAR *wszName, WCHAR *wszValue, TabSettings &tabSettings, FolderSettings &folderSettings);

	/* Window state update. */
	void UpdateWindowStates(const Tab &tab);
	void UpdateTreeViewSelection();
	void SetStatusBarParts(int width);
	void ResizeWindows();
	void SetListViewInitialPosition(HWND hListView) override;
	void AdjustFolderPanePosition();
	HRESULT UpdateStatusBarText(const Tab &tab);
	void ToggleFolders();
	void UpdateLayout();

	/* Languages. */
	void SetLanguageModule();
	BOOL VerifyLanguageVersion(const TCHAR *szLanguageModule) const;

	/* File operations. */
	void CopyToFolder(bool move);
	void OpenAllSelectedItems(BOOL bOpenInNewTab);
	void OpenListViewItem(int iItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);
	void OpenItem(const TCHAR *szItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow) override;
	void OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);
	void OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const TCHAR *szParameters) override;

	/* File context menu. */
	void AddMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems,
		DWORD_PTR dwData, HMENU hMenu) override;
	BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, int iCmd) override;

	/* File selection tests. */
	BOOL AnyItemsSelected() const;
	bool CanCreate() const override;
	BOOL CanCut() const override;
	BOOL CanCopy() const override;
	BOOL CanRename() const override;
	BOOL CanDelete() const override;
	BOOL CanShowFileProperties() const override;
	BOOL CanPaste() const override;
	BOOL TestItemAttributes(SFGAOF attributes) const;
	HRESULT GetSelectionAttributes(SFGAOF *pItemAttributes) const;

	HRESULT GetTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const;

	/* Display window. */
	void OnDisplayWindowResized(WPARAM wParam);
	void OnDisplayWindowIconRClick(POINT *ptClient);
	void OnDisplayWindowRClick(POINT *ptClient);
	void UpdateDisplayWindow(const Tab &tab);
	void UpdateDisplayWindowForZeroFiles(const Tab &tab);
	void UpdateDisplayWindowForOneFile(const Tab &tab);
	void UpdateDisplayWindowForMultipleFiles(const Tab &tab);

	/* Columns. */
	void CopyColumnInfoToClipboard();

	/* Bookmark handling. */
	HRESULT ExpandAndBrowsePath(const TCHAR *szPath);
	HRESULT ExpandAndBrowsePath(const TCHAR *szPath, BOOL bOpenInNewTab, BOOL bSwitchToNewTab);

	/* IExplorerplusplus methods. */
	const Config *GetConfig() const override;
	HMODULE GetLanguageModule() const override;
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
	void FocusChanged(WindowFocusSource windowFocusSource) override;
	boost::signals2::connection AddFocusChangeObserver(
		const FocusChangedSignal::slot_type &observer) override;
	boost::signals2::connection AddApplicationShuttingDownObserver(
		const ApplicationShuttingDownSignal::slot_type &observer) override;

	/* Menus. */
	void InitializeMainMenu();
	void SetGoMenuName(HMENU hMenu, UINT uMenuID, UINT csidl);
	void SetMainMenuImages();
	boost::signals2::connection AddMainMenuPreShowObserver(
		const MainMenuPreShowSignal::slot_type &observer) override;
	HMENU BuildViewsMenu() override;
	void AddViewModesToMenu(HMENU menu);

	// Dark mode
	void SetUpDarkMode();

	// Rebar
	HMENU CreateRebarHistoryMenu(BOOL bBack);
	std::optional<int> OnRebarCustomDraw(NMHDR *nmhdr);
	bool OnRebarEraseBackground(HDC hdc);

	/* Miscellaneous. */
	void CreateStatusBar();
	void InitializeDisplayWindow();
	int CreateDriveFreeSpaceString(const TCHAR *szPath, TCHAR *szBuffer, int nBuffer);
	void ShowMainRebarBand(HWND hwnd, BOOL bShow);
	BOOL OnMouseWheel(MousewheelSource mousewheelSource, WPARAM wParam, LPARAM lParam) override;
	StatusBar *GetStatusBar() override;
	void HandleDirectoryMonitoring(int iTabId);
	int DetermineListViewObjectIndex(HWND hListView);

	static void FolderSizeCallbackStub(
		int nFolders, int nFiles, PULARGE_INTEGER lTotalFolderSize, LPVOID pData);
	void FolderSizeCallback(
		FolderSizeExtraInfo *pfsei, int nFolders, int nFiles, PULARGE_INTEGER lTotalFolderSize);

	HWND m_hContainer;
	HWND m_hStatusBar;
	HWND m_hMainRebar;
	HWND m_hDisplayWindow;
	HWND m_hTabWindowToolbar;
	HWND m_hHolder;
	HWND m_foldersToolbarParent;
	HWND m_hFoldersToolbar;
	HWND m_hTabBacking;
	HWND m_hBookmarksToolbar;

	IDirectoryMonitor *m_pDirMon;
	ShellTreeView *m_shellTreeView;
	StatusBar *m_pStatusBar;

	HMODULE m_hLanguageModule;

	/** Internal state. **/
	HWND m_hLastActiveWindow;
	std::wstring m_CurrentDirectory;
	bool m_bTreeViewRightClick;
	bool m_bSelectingTreeViewDirectory;
	bool m_bAttemptToolbarRestore;
	bool m_bLanguageLoaded;
	bool m_bTreeViewOpenInNewTab;
	bool m_bShowTabBar;
	int m_iLastSelectedTab;
	ULONG m_SHChangeNotifyID;
	ValueWrapper<bool> m_InitializationFinished;

	/* Initialization. */
	BOOL m_bLoadSettingsFromXML;

	MainWindow *m_mainWindow;
	AddressBar *m_addressBar;

	std::unique_ptr<Navigation> m_navigation;

	std::unique_ptr<IconResourceLoader> m_iconResourceLoader;

	CachedIcons m_cachedIcons;

	MainMenuPreShowSignal m_mainMenuPreShowSignal;
	FocusChangedSignal m_focusChangedSignal;
	ApplicationShuttingDownSignal m_applicationShuttingDownSignal;

	/* Tabs. */
	TabContainer *m_tabContainer;
	wil::unique_hbrush m_tabBarBackgroundBrush;
	std::unique_ptr<TabRestorer> m_tabRestorer;
	std::unique_ptr<TabRestorerUI> m_tabRestorerUI;
	TabsInitializedSignal m_tabsInitializedSignal;

	ToolbarContextMenuSignal m_toolbarContextMenuSignal;

	/* Theming. */
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

	TaskbarThumbnails *m_taskbarThumbnails;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	/* Bookmarks. */
	BookmarkTree m_bookmarkTree;
	std::unique_ptr<BookmarksMainMenu> m_bookmarksMainMenu;
	BookmarksToolbar *m_pBookmarksToolbar;

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

	/* Customize colors. */
	std::vector<NColorRuleHelper::ColorRule> m_ColorRules;

	/* Undo support. */
	FileActionHandler m_FileActionHandler;

	/* Toolbars. */
	REBARBANDINFO m_ToolbarInformation[NUM_MAIN_TOOLBARS];
	MainToolbar *m_mainToolbar;
	DrivesToolbar *m_pDrivesToolbar;
	ApplicationToolbar *m_pApplicationToolbar;

	/* Display window folder sizes. */
	std::list<DWFolderSize> m_DWFolderSizes;
	int m_iDWFolderSizeUniqueId;

	/* Drag and drop. */
	bool m_bDragging;
	bool m_bDragCancelled;
	bool m_bDragAllowed;

	/* Rename support. */
	bool m_bListViewRenaming;

	/* Menu images. */
	std::vector<wil::unique_hbitmap> m_menuImages;

	/* Mousewheel. */
	int m_zDeltaTotal;

	bool m_blockNextListViewSelection;
};