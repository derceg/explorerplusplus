// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainer.h"
#include "Bookmarks/BookmarkHelper.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainTabView.h"
#include "PopupMenuView.h"
#include "PreservedTab.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/PreservedHistoryEntry.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowser/ShellBrowserFactory.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainerBackgroundContextMenu.h"
#include "TabContextMenu.h"
#include "TabEvents.h"
#include "TabStorage.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WeakPtrFactory.h"
#include "../Helper/WindowHelper.h"
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>
#include <ranges>

using namespace std::chrono_literals;

namespace
{

class MainTabViewItem : public TabViewItem
{
public:
	MainTabViewItem(Tab *tab, TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
		NavigationEvents *navigationEvents, IconFetcher *iconFetcher, CachedIcons *cachedIcons,
		MainTabViewImageListManager *imageListManager) :
		m_tab(tab),
		m_iconFetcher(iconFetcher),
		m_cachedIcons(cachedIcons),
		m_imageListManager(imageListManager)
	{
		m_connections.push_back(
			tabEvents->AddUpdatedObserver(std::bind_front(&MainTabViewItem::OnTabUpdated, this),
				TabEventScope::ForBrowser(*tab->GetBrowser())));

		m_connections.push_back(shellBrowserEvents->AddDirectoryPropertiesChangedObserver(
			std::bind(&MainTabViewItem::OnDisplayPropertiesUpdated, this),
			ShellBrowserEventScope::ForShellBrowser(*tab->GetShellBrowser())));

		m_connections.push_back(navigationEvents->AddCommittedObserver(
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

		auto cachedIconIndex = MaybeGetCachedIconIndex();

		if (cachedIconIndex)
		{
			return m_imageListManager->AddIconFromSystemImageList(*cachedIconIndex);
		}

		return m_imageListManager->GetDefaultFolderIconIndex();
	}

	std::optional<int> MaybeGetCachedIconIndex() const
	{
		const auto &pidl = m_tab->GetShellBrowser()->GetDirectory();

		std::wstring parsingPath;
		HRESULT hr = GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, parsingPath);

		if (FAILED(hr))
		{
			return std::nullopt;
		}

		return m_cachedIcons->MaybeGetIconIndex(parsingPath);
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

TabContainer *TabContainer::Create(MainTabView *view, BrowserWindow *browser,
	ShellBrowserFactory *shellBrowserFactory, TabEvents *tabEvents,
	ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
	TabRestorer *tabRestorer, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
	const AcceleratorManager *acceleratorManager, const Config *config,
	const ResourceLoader *resourceLoader, PlatformContext *platformContext)
{
	return new TabContainer(view, browser, shellBrowserFactory, tabEvents, shellBrowserEvents,
		navigationEvents, tabRestorer, cachedIcons, bookmarkTree, acceleratorManager, config,
		resourceLoader, platformContext);
}

TabContainer::TabContainer(MainTabView *view, BrowserWindow *browser,
	ShellBrowserFactory *shellBrowserFactory, TabEvents *tabEvents,
	ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
	TabRestorer *tabRestorer, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
	const AcceleratorManager *acceleratorManager, const Config *config,
	const ResourceLoader *resourceLoader, PlatformContext *platformContext) :
	ShellDropTargetWindow(view->GetHWND()),
	m_view(view),
	m_browser(browser),
	m_shellBrowserFactory(shellBrowserFactory),
	m_tabEvents(tabEvents),
	m_shellBrowserEvents(shellBrowserEvents),
	m_navigationEvents(navigationEvents),
	m_tabRestorer(tabRestorer),
	m_timerManager(m_hwnd),
	m_iconFetcher(m_hwnd, cachedIcons),
	m_cachedIcons(cachedIcons),
	m_bookmarkTree(bookmarkTree),
	m_acceleratorManager(acceleratorManager),
	m_config(config),
	m_resourceLoader(resourceLoader),
	m_platformContext(platformContext),
	m_iPreviousTabSelectionId(-1)
{
	Initialize(GetParent(m_view->GetHWND()));
}

void TabContainer::Initialize(HWND parent)
{
	m_view->SetDelegate(this);
	m_view->windowDestroyedSignal.AddObserver(
		std::bind_front(&TabContainer::OnWindowDestroyed, this));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(parent,
		std::bind_front(&TabContainer::ParentWndProc, this)));
}

void TabContainer::OnTabDoubleClicked(Tab *tab, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	if (m_config->doubleClickTabClose)
	{
		CloseTab(*tab);
	}
}

void TabContainer::OnTabMiddleClicked(Tab *tab, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	CloseTab(*tab);
}

void TabContainer::OnTabRightClicked(Tab *tab, const MouseEvent &event)
{
	POINT ptScreen = event.ptClient;
	BOOL res = ClientToScreen(m_hwnd, &ptScreen);
	CHECK(res);

	PopupMenuView popupMenu(m_browser);
	TabContextMenu menu(&popupMenu, m_acceleratorManager, tab, this, m_tabEvents, m_resourceLoader);
	popupMenu.Show(m_hwnd, ptScreen);
}

LRESULT TabContainer::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDBLCLK:
		CreateNewTabInDefaultDirectory({ .selected = true });
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

void TabContainer::ShowBackgroundContextMenu(const POINT &ptClient)
{
	POINT ptScreen = ptClient;
	ClientToScreen(m_hwnd, &ptScreen);

	PopupMenuView popupMenu(m_browser);
	TabContainerBackgroundContextMenu menu(&popupMenu, m_acceleratorManager, this, m_tabRestorer,
		m_bookmarkTree, m_browser, m_resourceLoader, m_platformContext);
	popupMenu.Show(m_hwnd, ptScreen);
}

void TabContainer::OnTabSelected(const Tab &tab)
{
	if (m_iPreviousTabSelectionId != -1)
	{
		m_tabSelectionHistory.push_back(m_iPreviousTabSelectionId);
	}

	m_iPreviousTabSelectionId = tab.GetId();

	m_tabEvents->NotifySelected(tab);
}

MainTabView *TabContainer::GetView()
{
	return m_view;
}

void TabContainer::CreateNewTabInDefaultDirectory(const TabSettings &tabSettings)
{
	CreateNewTab(m_config->defaultTabDirectory, tabSettings);
}

// Note that although it's guaranteed that the tab will be created, it's not guaranteed that it will
// be navigated to the directory that's provided. For example, the directory may not exist, or the
// user may not have access to it, or the path may be temporarily unavailable (e.g. because a
// network location has been disconnected). Therefore, it's not safe to make any assumptions about
// the actual directory the tab is in once it's been created.
Tab &TabContainer::CreateNewTab(const std::wstring &directory, const TabSettings &tabSettings,
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

Tab &TabContainer::CreateNewTab(const PreservedTab &preservedTab)
{
	const auto &preservedShellBrowser = preservedTab.preservedShellBrowser;
	auto shellBrowser = m_shellBrowserFactory->CreateFromPreserved(preservedShellBrowser);
	Tab::InitialData initialTabData{ .useCustomName = preservedTab.useCustomName,
		.customName = preservedTab.customName,
		.lockState = preservedTab.lockState };
	auto tab = std::make_unique<Tab>(std::move(shellBrowser), m_browser, this, m_tabEvents,
		initialTabData);
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

	TabSettings tabSettings{ .index = finalIndex, .selected = true };

	const auto *entry = preservedShellBrowser.history[preservedShellBrowser.currentEntry].get();
	auto navigateParams =
		NavigateParams::Normal(entry->GetPidl().Raw(), HistoryEntryType::ReplaceCurrentEntry);
	return SetUpNewTab(*rawTab, navigateParams, tabSettings);
}

Tab &TabContainer::CreateNewTab(NavigateParams &navigateParams, const TabSettings &tabSettings,
	const FolderSettings *folderSettings, const FolderColumns *initialColumns)
{
	FolderSettings folderSettingsFinal;

	if (folderSettings)
	{
		folderSettingsFinal = *folderSettings;
	}
	else
	{
		folderSettingsFinal = m_config->defaultFolderSettings;
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

	auto tab = std::make_unique<Tab>(std::move(shellBrowser), m_browser, this, m_tabEvents,
		initialTabData);
	auto *rawTab = tab.get();
	m_tabs.insert({ tab->GetId(), std::move(tab) });

	return SetUpNewTab(*rawTab, navigateParams, tabSettings);
}

Tab &TabContainer::SetUpNewTab(Tab &tab, NavigateParams &navigateParams,
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
		if (m_config->openNewTabNextToCurrent && m_view->MaybeGetSelectedIndex())
		{
			index = GetSelectedTabIndex() + 1;
		}
		else
		{
			index = m_view->GetNumTabs();
		}
	}

	auto tabItem = std::make_unique<MainTabViewItem>(&tab, m_tabEvents, m_shellBrowserEvents,
		m_navigationEvents, &m_iconFetcher, m_cachedIcons, m_view->GetImageListManager());
	tabItem->SetDoubleClickedCallback(
		std::bind_front(&TabContainer::OnTabDoubleClicked, this, &tab));
	tabItem->SetMiddleClickedCallback(
		std::bind_front(&TabContainer::OnTabMiddleClicked, this, &tab));
	tabItem->SetRightClickedCallback(std::bind_front(&TabContainer::OnTabRightClicked, this, &tab));

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	index = m_view->AddTab(std::move(tabItem), index);

	m_tabEvents->NotifyCreated(tab);

	bool selected = false;

	if (tabSettings.selected)
	{
		selected = *tabSettings.selected;
	}

	if (selected)
	{
		SelectTabAtIndex(index);
	}

	if (m_tabs.size() == 1)
	{
		// The tab control will implicitly select the first tab that's created, so a selection event
		// needs to be synthesized here.
		CHECK_EQ(m_view->GetSelectedIndex(), index);
		OnTabSelected(tab);
	}

	tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);

	return tab;
}

void TabContainer::CloseAllTabs()
{
	int numTabs = GetNumTabs();

	for (int i = numTabs - 1; i >= 0; i--)
	{
		bool closed = CloseTab(GetTabByIndex(i), CloseMode::Force);
		DCHECK(closed);
	}
}

bool TabContainer::CloseTab(const Tab &tab)
{
	return CloseTab(tab, CloseMode::Normal);
}

bool TabContainer::CloseTab(const Tab &tab, CloseMode closeMode)
{
	if ((tab.GetLockState() == Tab::LockState::Locked
			|| tab.GetLockState() == Tab::LockState::AddressLocked)
		&& closeMode == CloseMode::Normal)
	{
		return false;
	}

	m_tabEvents->NotifyPreRemoval(tab, GetTabIndex(tab));

	RemoveTabFromControl(tab);

	// Taking ownership of the tab here will ensure it's still live when the observers are notified
	// below.
	auto itr = m_tabs.find(tab.GetId());
	CHECK(itr != m_tabs.end());
	auto ownedTab = std::move(*itr);

	m_tabs.erase(itr);

	m_tabEvents->NotifyRemoved(tab);

	return true;
}

void TabContainer::RemoveTabFromControl(const Tab &tab)
{
	m_tabSelectionHistory.erase(
		std::remove(m_tabSelectionHistory.begin(), m_tabSelectionHistory.end(), tab.GetId()),
		m_tabSelectionHistory.end());

	const int index = GetTabIndex(tab);

	if (IsTabSelected(tab) && m_tabs.size() > 1)
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

Tab &TabContainer::GetTab(int tabId) const
{
	auto *tab = MaybeGetTab(tabId);
	CHECK(tab);
	return *tab;
}

Tab *TabContainer::MaybeGetTab(int tabId) const
{
	auto itr = m_tabs.find(tabId);

	if (itr == m_tabs.end())
	{
		return nullptr;
	}

	return itr->second.get();
}

void TabContainer::SelectTab(const Tab &tab)
{
	int index = GetTabIndex(tab);
	SelectTabAtIndex(index);
}

void TabContainer::SelectAdjacentTab(SelectionDirection selectionDirection)
{
	int numTabs = GetNumTabs();
	CHECK(numTabs > 0);

	int selectedIndex = GetSelectedTabIndex();
	int step = (selectionDirection == SelectionDirection::Next) ? 1 : -1;
	int newIndex = (selectedIndex + step + numTabs) % numTabs;
	SelectTabAtIndex(newIndex);
}

void TabContainer::SelectTabAtIndex(int index)
{
	m_view->SelectTabAtIndex(index);
}

Tab &TabContainer::GetSelectedTab() const
{
	int index = GetSelectedTabIndex();
	return GetTabByIndex(index);
}

int TabContainer::GetSelectedTabIndex() const
{
	return m_view->GetSelectedIndex();
}

bool TabContainer::IsTabSelected(const Tab &tab) const
{
	const Tab &selectedTab = GetSelectedTab();
	return tab.GetId() == selectedTab.GetId();
}

Tab &TabContainer::GetTabByIndex(int index) const
{
	auto *tabViewItem = static_cast<MainTabViewItem *>(m_view->GetTabAtIndex(index));
	return *tabViewItem->GetTab();
}

int TabContainer::GetTabIndex(const Tab &tab) const
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

int TabContainer::GetNumTabs() const
{
	return static_cast<int>(m_tabs.size());
}

int TabContainer::MoveTab(const Tab &tab, int newIndex)
{
	int index = GetTabIndex(tab);
	return m_view->MoveTab(index, newIndex);
}

const std::unordered_map<int, std::unique_ptr<Tab>> &TabContainer::GetAllTabs() const
{
	return m_tabs;
}

std::vector<Tab *> TabContainer::GetAllTabsInOrder() const
{
	std::vector<Tab *> sortedTabs;

	for (const auto &tab : m_tabs | std::views::values)
	{
		sortedTabs.push_back(tab.get());
	}

	// The Tab class is non-copyable, so there are essentially two ways of retrieving a sorted list
	// of tabs, as far as I can tell:
	//
	// 1. The first is to maintain a sorted list of tabs while the program is running. I generally
	//    don't think that's a good idea, since it would be redundant (the tab control already
	//    stores that information) and it risks possible issues (if the two sets get out of sync).
	//
	// 2. The second is to sort the tabs when needed. Because they're non-copyable, that can't be
	//    done directly, so pointers are used instead.
	std::sort(sortedTabs.begin(), sortedTabs.end(), [this](const auto *tab1, const auto *tab2)
		{ return GetTabIndex(*tab1) < GetTabIndex(*tab2); });

	return sortedTabs;
}

Tab &TabContainer::DuplicateTab(const Tab &tab)
{
	PreservedTab preservedTab(tab, GetTabIndex(tab) + 1);
	return CreateNewTab(preservedTab);
}

void TabContainer::OnTabMoved(int fromIndex, int toIndex)
{
	const Tab &tab = GetTabByIndex(toIndex);
	m_tabEvents->NotifyMoved(tab, fromIndex, toIndex);
}

bool TabContainer::ShouldRemoveIcon(int iconIndex)
{
	return !m_view->GetImageListManager()->IsDefaultIcon(iconIndex);
}

void TabContainer::OnSelectionChanged()
{
	OnTabSelected(GetSelectedTab());
}

int TabContainer::GetDropTargetItem(const POINT &pt)
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

unique_pidl_absolute TabContainer::GetPidlForTargetItem(int targetItem)
{
	if (targetItem == -1)
	{
		return nullptr;
	}

	const auto &tab = GetTabByIndex(targetItem);
	return tab.GetShellBrowserImpl()->GetDirectoryIdl();
}

IUnknown *TabContainer::GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl)
{
	UNREFERENCED_PARAMETER(targetItemPidl);

	return nullptr;
}

bool TabContainer::IsTargetSourceOfDrop(int targetItem)
{
	UNREFERENCED_PARAMETER(targetItem);

	return false;
}

void TabContainer::UpdateUiForDrop(int targetItem, const POINT &pt)
{
	UpdateUiForTargetItem(targetItem);
	ScrollTabControlForDrop(pt);
}

void TabContainer::UpdateUiForTargetItem(int targetItem)
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
			std::bind(&TabContainer::OnDropSwitchTabTimer, this));
	}

	m_dropTargetContext->targetIndex = targetItem;
}

void TabContainer::ScrollTabControlForDrop(const POINT &pt)
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
			std::bind(&TabContainer::OnDropScrollTimer, this));
	}
}

void TabContainer::ResetDropUiState()
{
	m_dropTargetContext.reset();
}

void TabContainer::OnDropSwitchTabTimer()
{
	CHECK(m_dropTargetContext);
	CHECK_NE(m_dropTargetContext->targetIndex, -1);

	if (m_dropTargetContext->targetIndex != GetSelectedTabIndex())
	{
		SelectTabAtIndex(m_dropTargetContext->targetIndex);
	}
}

void TabContainer::OnDropScrollTimer()
{
	CHECK(m_dropTargetContext);
	CHECK(m_dropTargetContext->scrollDirection);
	m_view->Scroll(*m_dropTargetContext->scrollDirection);

	m_dropTargetContext->scrollDirection.reset();
}

std::vector<TabStorageData> TabContainer::GetStorageData() const
{
	std::vector<TabStorageData> tabListStorageData;

	for (const auto *tab : GetAllTabsInOrder())
	{
		tabListStorageData.push_back(tab->GetStorageData());
	}

	return tabListStorageData;
}

void TabContainer::OnWindowDestroyed()
{
	delete this;
}
