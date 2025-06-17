// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainerImpl.h"
#include "App.h"
#include "Bookmarks/BookmarkHelper.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "MainTabView.h"
#include "PopupMenuView.h"
#include "PreservedTab.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/PreservedHistoryEntry.h"
#include "ShellBrowser/ShellBrowserFactory.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainerBackgroundContextMenu.h"
#include "TabContextMenu.h"
#include "TabStorage.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TabHelper.h"
#include "../Helper/WeakPtrFactory.h"
#include "../Helper/WindowHelper.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <glog/logging.h>

using namespace std::chrono_literals;

namespace
{

class MainTabViewItem : public TabViewItem
{
public:
	MainTabViewItem(Tab *tab, App *app, IconFetcher *iconFetcher, CachedIcons *cachedIcons,
		MainTabViewImageListManager *imageListManager) :
		m_tab(tab),
		m_iconFetcher(iconFetcher),
		m_cachedIcons(cachedIcons),
		m_imageListManager(imageListManager)
	{
		m_connections.push_back(app->GetTabEvents()->AddUpdatedObserver(
			std::bind_front(&MainTabViewItem::OnTabUpdated, this),
			TabEventScope::ForBrowser(*tab->GetBrowser())));

		m_connections.push_back(app->GetShellBrowserEvents()->AddDirectoryPropertiesChangedObserver(
			std::bind(&MainTabViewItem::OnDisplayPropertiesUpdated, this),
			ShellBrowserEventScope::ForShellBrowser(*tab->GetShellBrowser())));

		m_connections.push_back(app->GetNavigationEvents()->AddCommittedObserver(
			std::bind(&MainTabViewItem::OnDisplayPropertiesUpdated, this),
			NavigationEventScope::ForShellBrowser(*tab->GetShellBrowser())));
	}

	std::wstring GetText() const override
	{
		std::wstring name = m_tab->GetName();
		boost::replace_all(name, L"&", L"&&");
		return name;
	}

	std::wstring GetTooltipText() const override
	{
		const auto &pidlDirectory = m_tab->GetShellBrowser()->GetDirectory();
		return GetFolderPathForDisplayWithFallback(pidlDirectory.Raw());
	}

	std::optional<int> GetIconIndex() const override
	{
		if (m_tab->GetLockState() == Tab::LockState::Locked
			|| m_tab->GetLockState() == Tab::LockState::AddressLocked)
		{
			return m_imageListManager->GetLockIconIndex();
		}

		if (!m_iconIndex)
		{
			m_iconIndex = DetermineIconIndex();
		}

		return *m_iconIndex;
	}

	Tab *GetTab()
	{
		return m_tab;
	}

private:
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
	{
		UNREFERENCED_PARAMETER(propertyType);

		if (&tab == m_tab)
		{
			OnDisplayPropertiesUpdated();
		}
	}

	void OnDisplayPropertiesUpdated()
	{
		m_weakPtrFactory.InvalidateWeakPtrs();
		m_iconIndex.reset();

		NotifyParentOfUpdate();
	}

	int DetermineIconIndex() const
	{
		FetchUpdatedIcon();

		auto cachedIconIndex =
			m_cachedIcons->MaybeGetIconIndex(m_tab->GetShellBrowserImpl()->GetDirectoryPath());

		if (cachedIconIndex)
		{
			return m_imageListManager->AddIconFromSystemImageList(*cachedIconIndex);
		}

		return m_imageListManager->GetDefaultFolderIconIndex();
	}

	void FetchUpdatedIcon() const
	{
		const auto &pidlDirectory = m_tab->GetShellBrowser()->GetDirectory();

		m_iconFetcher->QueueIconTask(pidlDirectory.Raw(),
			[weakSelf = m_weakPtrFactory.GetWeakPtr()](int iconIndex, int overlayIndex)
			{
				UNREFERENCED_PARAMETER(overlayIndex);

				if (!weakSelf)
				{
					return;
				}

				weakSelf->OnIconLoaded(iconIndex);
			});
	}

	void OnIconLoaded(int iconIndex) const
	{
		m_iconIndex = m_imageListManager->AddIconFromSystemImageList(iconIndex);

		NotifyParentOfUpdate();
	}

	Tab *const m_tab;
	IconFetcher *const m_iconFetcher;
	CachedIcons *const m_cachedIcons;
	MainTabViewImageListManager *const m_imageListManager;
	mutable std::optional<int> m_iconIndex;
	std::vector<boost::signals2::scoped_connection> m_connections;

	WeakPtrFactory<MainTabViewItem> m_weakPtrFactory{ this };
};

}

TabContainerImpl *TabContainerImpl::Create(MainTabView *view, BrowserWindow *browser, App *app,
	CoreInterface *coreInterface, ShellBrowserFactory *shellBrowserFactory,
	CachedIcons *cachedIcons, BookmarkTree *bookmarkTree, const Config *config)
{
	return new TabContainerImpl(view, browser, app, coreInterface, shellBrowserFactory, cachedIcons,
		bookmarkTree, config);
}

TabContainerImpl::TabContainerImpl(MainTabView *view, BrowserWindow *browser, App *app,
	CoreInterface *coreInterface, ShellBrowserFactory *shellBrowserFactory,
	CachedIcons *cachedIcons, BookmarkTree *bookmarkTree, const Config *config) :
	ShellDropTargetWindow(view->GetHWND()),
	m_view(view),
	m_browser(browser),
	m_app(app),
	m_coreInterface(coreInterface),
	m_shellBrowserFactory(shellBrowserFactory),
	m_timerManager(m_hwnd),
	m_iconFetcher(m_hwnd, cachedIcons),
	m_cachedIcons(cachedIcons),
	m_bookmarkTree(bookmarkTree),
	m_config(config),
	m_iPreviousTabSelectionId(-1)
{
	Initialize(GetParent(m_view->GetHWND()));
}

void TabContainerImpl::Initialize(HWND parent)
{
	m_view->SetDelegate(this);
	m_view->windowDestroyedSignal.AddObserver(
		std::bind_front(&TabContainerImpl::OnWindowDestroyed, this));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&TabContainerImpl::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(parent,
		std::bind_front(&TabContainerImpl::ParentWndProc, this)));
}

LRESULT TabContainerImpl::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MENUSELECT:
		/* Forward the message to the main window so it can
		handle menu help. */
		SendMessage(m_coreInterface->GetMainWindow(), WM_MENUSELECT, wParam, lParam);
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void TabContainerImpl::OnTabDoubleClicked(Tab *tab, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	if (m_config->doubleClickTabClose)
	{
		CloseTab(*tab);
	}
}

void TabContainerImpl::OnTabMiddleClicked(Tab *tab, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	CloseTab(*tab);
}

void TabContainerImpl::OnTabRightClicked(Tab *tab, const MouseEvent &event)
{
	POINT ptScreen = event.ptClient;
	BOOL res = ClientToScreen(m_hwnd, &ptScreen);
	CHECK(res);

	PopupMenuView popupMenu(m_browser);
	TabContextMenu menu(&popupMenu, m_app->GetAcceleratorManager(), tab, this,
		m_app->GetTabEvents(), m_app->GetResourceLoader());
	popupMenu.Show(m_hwnd, ptScreen);
}

LRESULT TabContainerImpl::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDBLCLK:
		CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
		break;

	case WM_RBUTTONUP:
	{
		POINT ptClient;
		POINTSTOPOINT(ptClient, MAKEPOINTS(lParam));
		ShowBackgroundContextMenu(ptClient);
	}
	break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void TabContainerImpl::ShowBackgroundContextMenu(const POINT &ptClient)
{
	POINT ptScreen = ptClient;
	ClientToScreen(m_hwnd, &ptScreen);

	PopupMenuView popupMenu(m_browser);
	TabContainerBackgroundContextMenu menu(&popupMenu, m_app->GetAcceleratorManager(), this,
		m_app->GetTabRestorer(), m_bookmarkTree, m_browser, m_coreInterface,
		m_app->GetResourceLoader());
	popupMenu.Show(m_hwnd, ptScreen);
}

void TabContainerImpl::OnTabSelected(const Tab &tab)
{
	if (m_iPreviousTabSelectionId != -1)
	{
		m_tabSelectionHistory.push_back(m_iPreviousTabSelectionId);
	}

	m_iPreviousTabSelectionId = tab.GetId();

	m_app->GetTabEvents()->NotifySelected(tab);
}

void TabContainerImpl::CreateNewTabInDefaultDirectory(const TabSettings &tabSettings)
{
	CreateNewTab(m_config->defaultTabDirectory, tabSettings);
}

// Note that although it's guaranteed that the tab will be created, it's not guaranteed that it will
// be navigated to the directory that's provided. For example, the directory may not exist, or the
// user may not have access to it, or the path may be temporarily unavailable (e.g. because a
// network location has been disconnected). Therefore, it's not safe to make any assumptions about
// the actual directory the tab is in once it's been created.
Tab &TabContainerImpl::CreateNewTab(const std::wstring &directory, const TabSettings &tabSettings,
	const FolderSettings *folderSettings, const FolderColumns *initialColumns)
{
	unique_pidl_absolute pidl;
	HRESULT hr = ParseDisplayNameForNavigation(directory.c_str(), pidl);

	if (FAILED(hr))
	{
		hr = ParseDisplayNameForNavigation(m_config->defaultTabDirectory.c_str(), pidl);

		if (FAILED(hr))
		{
			hr = ParseDisplayNameForNavigation(m_config->defaultTabDirectoryStatic.c_str(), pidl);
		}
	}

	auto navigateParams = NavigateParams::Normal(pidl.get());
	return CreateNewTab(navigateParams, tabSettings, folderSettings, initialColumns);
}

Tab &TabContainerImpl::CreateNewTab(const PreservedTab &preservedTab)
{
	auto shellBrowser = m_shellBrowserFactory->CreateFromPreserved(preservedTab.history,
		preservedTab.currentEntry, preservedTab.preservedFolderState);
	Tab::InitialData initialTabData{ .useCustomName = preservedTab.useCustomName,
		.customName = preservedTab.customName,
		.lockState = preservedTab.lockState };
	auto tab = std::make_unique<Tab>(std::move(shellBrowser), m_browser, this,
		m_app->GetTabEvents(), initialTabData);
	auto *rawTab = tab.get();
	m_tabs.insert({ tab->GetId(), std::move(tab) });

	int finalIndex;

	if (preservedTab.browserId == m_browser->GetId())
	{
		finalIndex = preservedTab.index;
	}
	else
	{
		// This tab is being restored from a different browser, so its index isn't relevant and it
		// should simply be added to the end of the current set of tabs.
		finalIndex = static_cast<int>(m_tabs.size()) - 1;
	}

	TabSettings tabSettings(_index = finalIndex, _selected = true);

	PreservedHistoryEntry *entry = preservedTab.history.at(preservedTab.currentEntry).get();
	auto navigateParams =
		NavigateParams::Normal(entry->GetPidl().Raw(), HistoryEntryType::ReplaceCurrentEntry);
	return SetUpNewTab(*rawTab, navigateParams, tabSettings);
}

Tab &TabContainerImpl::CreateNewTab(NavigateParams &navigateParams, const TabSettings &tabSettings,
	const FolderSettings *folderSettings, const FolderColumns *initialColumns)
{
	FolderSettings folderSettingsFinal;

	if (folderSettings)
	{
		folderSettingsFinal = *folderSettings;
	}
	else
	{
		folderSettingsFinal = m_app->GetConfig()->defaultFolderSettings;
	}

	auto shellBrowser =
		m_shellBrowserFactory->Create(navigateParams.pidl, folderSettingsFinal, initialColumns);

	Tab::InitialData initialTabData;

	if (tabSettings.lockState)
	{
		initialTabData.lockState = *tabSettings.lockState;
	}

	if (tabSettings.name)
	{
		initialTabData.useCustomName = true;
		initialTabData.customName = *tabSettings.name;
	}

	auto tab = std::make_unique<Tab>(std::move(shellBrowser), m_browser, this,
		m_app->GetTabEvents(), initialTabData);
	auto *rawTab = tab.get();
	m_tabs.insert({ tab->GetId(), std::move(tab) });

	return SetUpNewTab(*rawTab, navigateParams, tabSettings);
}

Tab &TabContainerImpl::SetUpNewTab(Tab &tab, NavigateParams &navigateParams,
	const TabSettings &tabSettings)
{
	int index;

	if (tabSettings.index)
	{
		index = *tabSettings.index;
	}
	else
	{
		// When the application is first started, the number of tabs will be 0 initially, so there
		// won't be any selected tab. In that case, the openNewTabNextToCurrent setting should
		// effectively be ignored.
		if (m_config->openNewTabNextToCurrent && GetSelectedTabIndexOptional())
		{
			index = GetSelectedTabIndex() + 1;
		}
		else
		{
			index = TabCtrl_GetItemCount(m_hwnd);
		}
	}

	auto tabItem = std::make_unique<MainTabViewItem>(&tab, m_app, &m_iconFetcher, m_cachedIcons,
		m_view->GetImageListManager());
	tabItem->SetDoubleClickedCallback(
		std::bind_front(&TabContainerImpl::OnTabDoubleClicked, this, &tab));
	tabItem->SetMiddleClickedCallback(
		std::bind_front(&TabContainerImpl::OnTabMiddleClicked, this, &tab));
	tabItem->SetRightClickedCallback(
		std::bind_front(&TabContainerImpl::OnTabRightClicked, this, &tab));

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	index = m_view->AddTab(std::move(tabItem), index);

	bool selected = false;

	if (tabSettings.selected)
	{
		selected = *tabSettings.selected;
	}

	if (m_tabs.size() == 1)
	{
		// This is the first tab being inserted, so it should be selected (to ensure there's always
		// a selected tab), regardless of what the caller passes in.
		selected = true;
	}

	m_app->GetTabEvents()->NotifyCreated(tab);

	if (selected)
	{
		SelectTabAtIndex(index);
	}

	tab.GetShellBrowserImpl()->GetNavigationController()->Navigate(navigateParams);

	return tab;
}

bool TabContainerImpl::CloseTab(const Tab &tab)
{
	const int nTabs = GetNumTabs();

	if (nTabs == 1)
	{
		if (m_config->closeMainWindowOnTabClose)
		{
			m_browser->Close();
			return true;
		}
		else
		{
			return false;
		}
	}

	/* The tab is locked. Don't close it. */
	if (tab.GetLockState() == Tab::LockState::Locked
		|| tab.GetLockState() == Tab::LockState::AddressLocked)
	{
		return false;
	}

	m_app->GetTabEvents()->NotifyPreRemoval(tab, GetTabIndex(tab));

	RemoveTabFromControl(tab);

	// Taking ownership of the tab here will ensure it's still live when the observers are notified
	// below.
	auto itr = m_tabs.find(tab.GetId());
	CHECK(itr != m_tabs.end());
	auto ownedTab = std::move(*itr);

	m_tabs.erase(itr);

	m_app->GetTabEvents()->NotifyRemoved(tab);

	return true;
}

void TabContainerImpl::RemoveTabFromControl(const Tab &tab)
{
	m_tabSelectionHistory.erase(
		std::remove(m_tabSelectionHistory.begin(), m_tabSelectionHistory.end(), tab.GetId()),
		m_tabSelectionHistory.end());

	const int index = GetTabIndex(tab);

	if (IsTabSelected(tab))
	{
		int newIndex;

		/* If there was a previously active tab, the focus
		should be switched back to it. */
		if (!m_tabSelectionHistory.empty())
		{
			const int lastTabId = m_tabSelectionHistory.back();
			m_tabSelectionHistory.pop_back();

			const Tab &lastTab = GetTab(lastTabId);
			newIndex = GetTabIndex(lastTab);
		}
		else
		{
			// If the last tab in the control is what's being closed,
			// the tab before it will be selected.
			if (index == (GetNumTabs() - 1))
			{
				newIndex = index - 1;
			}
			else
			{
				newIndex = index + 1;
			}
		}

		SelectTabAtIndex(newIndex);

		// This is somewhat hacky. Switching the tab will cause the
		// previously selected tab (i.e. the tab that's about to be
		// closed) to be added to the history list. That's not
		// desirable, so the last entry will be removed here.
		m_tabSelectionHistory.pop_back();
	}

	m_view->RemoveTab(index);
}

Tab &TabContainerImpl::GetTab(int tabId) const
{
	auto *tab = GetTabOptional(tabId);
	CHECK(tab);
	return *tab;
}

Tab *TabContainerImpl::GetTabOptional(int tabId) const
{
	auto itr = m_tabs.find(tabId);

	if (itr == m_tabs.end())
	{
		return nullptr;
	}

	return itr->second.get();
}

void TabContainerImpl::SelectTab(const Tab &tab)
{
	int index = GetTabIndex(tab);
	SelectTabAtIndex(index);
}

void TabContainerImpl::SelectAdjacentTab(BOOL bNextTab)
{
	int nTabs = GetNumTabs();
	int newIndex = GetSelectedTabIndex();

	if (bNextTab)
	{
		/* If this is the last tab in the order,
		wrap the selection back to the start. */
		if (newIndex == (nTabs - 1))
		{
			newIndex = 0;
		}
		else
		{
			newIndex++;
		}
	}
	else
	{
		/* If this is the first tab in the order,
		wrap the selection back to the end. */
		if (newIndex == 0)
		{
			newIndex = nTabs - 1;
		}
		else
		{
			newIndex--;
		}
	}

	SelectTabAtIndex(newIndex);
}

void TabContainerImpl::SelectTabAtIndex(int index)
{
	m_view->SelectTabAtIndex(index);
}

Tab &TabContainerImpl::GetSelectedTab() const
{
	int index = GetSelectedTabIndex();
	return GetTabByIndex(index);
}

int TabContainerImpl::GetSelectedTabIndex() const
{
	int index = TabCtrl_GetCurSel(m_hwnd);
	CHECK_NE(index, -1) << "No selected tab";
	return index;
}

std::optional<int> TabContainerImpl::GetSelectedTabIndexOptional() const
{
	int index = TabCtrl_GetCurSel(m_hwnd);

	if (index == -1)
	{
		return std::nullopt;
	}

	return index;
}

bool TabContainerImpl::IsTabSelected(const Tab &tab) const
{
	const Tab &selectedTab = GetSelectedTab();
	return tab.GetId() == selectedTab.GetId();
}

Tab &TabContainerImpl::GetTabByIndex(int index) const
{
	auto *tabViewItem = static_cast<MainTabViewItem *>(m_view->GetTabAtIndex(index));
	return *tabViewItem->GetTab();
}

int TabContainerImpl::GetTabIndex(const Tab &tab) const
{
	int numTabs = m_view->GetNumTabs();

	for (int i = 0; i < numTabs; i++)
	{
		const auto &currentTab = GetTabByIndex(i);

		if (currentTab.GetId() == tab.GetId())
		{
			return i;
		}
	}

	// All internal tab objects should have an index.
	LOG(FATAL) << "Couldn't determine index for tab";
}

int TabContainerImpl::GetNumTabs() const
{
	return static_cast<int>(m_tabs.size());
}

int TabContainerImpl::MoveTab(const Tab &tab, int newIndex)
{
	int index = GetTabIndex(tab);
	return TabHelper::MoveItem(m_hwnd, index, newIndex);
}

std::unordered_map<int, std::unique_ptr<Tab>> &TabContainerImpl::GetTabs()
{
	return m_tabs;
}

const std::unordered_map<int, std::unique_ptr<Tab>> &TabContainerImpl::GetAllTabs() const
{
	return m_tabs;
}

std::vector<std::reference_wrapper<const Tab>> TabContainerImpl::GetAllTabsInOrder() const
{
	std::vector<std::reference_wrapper<const Tab>> sortedTabs;

	for (const auto &tab : m_tabs | boost::adaptors::map_values)
	{
		sortedTabs.emplace_back(*tab);
	}

	// The Tab class is non-copyable, so there are essentially two ways of
	// retrieving a sorted list of tabs, as far as I can tell:
	//
	// 1. The first is to maintain a sorted list of tabs while the program is
	// running. I generally don't think that's a good idea, since it would be
	// redundant (the tab control already stores that information) and it risks
	// possible issues (if the two sets get out of sync).
	//
	// 2. The second is to sort the tabs when needed. Because they're
	// non-copyable, that can't be done directly. std::reference_wrapper allows
	// it to be done relatively easily, though. Sorting a set of pointers would
	// accomplish the same thing.
	std::sort(sortedTabs.begin(), sortedTabs.end(), [this](const auto &tab1, const auto &tab2)
		{ return GetTabIndex(tab1.get()) < GetTabIndex(tab2.get()); });

	return sortedTabs;
}

void TabContainerImpl::DuplicateTab(const Tab &tab)
{
	auto folderSettings = tab.GetShellBrowserImpl()->GetFolderSettings();
	auto folderColumns = tab.GetShellBrowserImpl()->ExportAllColumns();
	auto navigateParams =
		NavigateParams::Normal(tab.GetShellBrowserImpl()->GetDirectoryIdl().get());
	CreateNewTab(navigateParams, {}, &folderSettings, &folderColumns);
}

void TabContainerImpl::OnTabMoved(int fromIndex, int toIndex)
{
	const Tab &tab = GetTabByIndex(toIndex);
	m_app->GetTabEvents()->NotifyMoved(tab, fromIndex, toIndex);
}

bool TabContainerImpl::ShouldRemoveIcon(int iconIndex)
{
	return !m_view->GetImageListManager()->IsDefaultIcon(iconIndex);
}

void TabContainerImpl::OnSelectionChanged()
{
	OnTabSelected(GetSelectedTab());
}

int TabContainerImpl::GetDropTargetItem(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_hwnd, &ptClient);

	if (!res)
	{
		return -1;
	}

	TCHITTESTINFO hitTestInfo;
	hitTestInfo.pt = ptClient;
	return TabCtrl_HitTest(m_hwnd, &hitTestInfo);
}

unique_pidl_absolute TabContainerImpl::GetPidlForTargetItem(int targetItem)
{
	if (targetItem == -1)
	{
		return nullptr;
	}

	const auto &tab = GetTabByIndex(targetItem);
	return tab.GetShellBrowserImpl()->GetDirectoryIdl();
}

IUnknown *TabContainerImpl::GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl)
{
	UNREFERENCED_PARAMETER(targetItemPidl);

	return nullptr;
}

bool TabContainerImpl::IsTargetSourceOfDrop(int targetItem, IDataObject *dataObject)
{
	UNREFERENCED_PARAMETER(targetItem);
	UNREFERENCED_PARAMETER(dataObject);

	return false;
}

void TabContainerImpl::UpdateUiForDrop(int targetItem, const POINT &pt)
{
	UpdateUiForTargetItem(targetItem);
	ScrollTabControlForDrop(pt);
}

void TabContainerImpl::UpdateUiForTargetItem(int targetItem)
{
	if (!m_dropTargetContext)
	{
		m_dropTargetContext = DropTargetContext(&m_timerManager);
	}

	if (targetItem == -1)
	{
		m_dropTargetContext->switchTabTimer.Stop();
	}
	else if (m_dropTargetContext->targetIndex != targetItem)
	{
		m_dropTargetContext->switchTabTimer.Start(500ms,
			std::bind(&TabContainerImpl::OnDropSwitchTabTimer, this));
	}

	m_dropTargetContext->targetIndex = targetItem;
}

void TabContainerImpl::ScrollTabControlForDrop(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_hwnd, &ptClient);

	if (!res)
	{
		return;
	}

	RECT rc;
	res = GetClientRect(m_hwnd, &rc);

	if (!res)
	{
		return;
	}

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);
	std::optional<TabView::ScrollDirection> scrollDirection;

	if (ptClient.x < MulDiv(DROP_SCROLL_MARGIN_X_96DPI, dpi, USER_DEFAULT_SCREEN_DPI))
	{
		scrollDirection = TabView::ScrollDirection::Left;
	}
	else if (ptClient.x
		> (rc.right - MulDiv(DROP_SCROLL_MARGIN_X_96DPI, dpi, USER_DEFAULT_SCREEN_DPI)))
	{
		scrollDirection = TabView::ScrollDirection::Right;
	}

	if (!scrollDirection)
	{
		m_dropTargetContext->scrollTimer.Stop();
		return;
	}

	if (!m_dropTargetContext->scrollDirection
		|| scrollDirection != *m_dropTargetContext->scrollDirection)
	{
		m_dropTargetContext->scrollDirection = scrollDirection;
		m_dropTargetContext->scrollTimer.Start(1s,
			std::bind(&TabContainerImpl::OnDropScrollTimer, this));
	}
}

void TabContainerImpl::ResetDropUiState()
{
	m_dropTargetContext.reset();
}

void TabContainerImpl::OnDropSwitchTabTimer()
{
	CHECK(m_dropTargetContext);
	CHECK_NE(m_dropTargetContext->targetIndex, -1);

	if (m_dropTargetContext->targetIndex != GetSelectedTabIndex())
	{
		SelectTabAtIndex(m_dropTargetContext->targetIndex);
	}
}

void TabContainerImpl::OnDropScrollTimer()
{
	CHECK(m_dropTargetContext);
	CHECK(m_dropTargetContext->scrollDirection);
	m_view->Scroll(*m_dropTargetContext->scrollDirection);

	m_dropTargetContext->scrollDirection.reset();
}

std::vector<TabStorageData> TabContainerImpl::GetStorageData() const
{
	std::vector<TabStorageData> tabListStorageData;

	for (auto tabRef : GetAllTabsInOrder())
	{
		const auto &tab = tabRef.get();
		tabListStorageData.push_back(tab.GetStorageData());
	}

	return tabListStorageData;
}

void TabContainerImpl::OnWindowDestroyed()
{
	delete this;
}
