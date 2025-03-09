// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "Config.h"
#include "DocumentServiceProvider.h"
#include "FeatureList.h"
#include "HistoryEntry.h"
#include "IconFetcher.h"
#include "ItemData.h"
#include "MainResource.h"
#include "NavigationRequest.h"
#include "RuntimeHelper.h"
#include "ShellEnumeratorImpl.h"
#include "ShellNavigationController.h"
#include "ShellView.h"
#include "ViewModes.h"
#include "WebBrowserApp.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WinRTBaseWrapper.h"
#include "../Helper/WindowHelper.h"
#include <wil/com.h>
#include <propkey.h>
#include <propvarutil.h>
#include <list>

void ShellBrowserImpl::OnNavigationStarted(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(request);

	DCHECK(shellBrowser == this);

	RecalcWindowCursor(m_hListView);
}

void ShellBrowserImpl::ChangeFolders(const PidlAbsolute &directory)
{
	PrepareToChangeFolders();

	bool isVirtualFolder = false;
	SFGAOF attributes = SFGAO_FILESYSTEM;
	HRESULT hr = GetItemAttributes(directory.Raw(), &attributes);
	DCHECK(SUCCEEDED(hr));

	if (SUCCEEDED(hr))
	{
		isVirtualFolder = WI_IsFlagClear(attributes, SFGAO_FILESYSTEM);
	}

	m_directoryState.pidlDirectory = directory;
	m_directoryState.directory = GetDisplayNameWithFallback(directory.Raw(), SHGDN_FORPARSING);
	m_directoryState.virtualFolder = isVirtualFolder;
	m_uniqueFolderId++;

	SetActiveColumnSet();
	VerifySortMode();
	SetViewModeInternal(m_folderSettings.viewMode);

	m_folderVisited = true;
}

void ShellBrowserImpl::PrepareToChangeFolders()
{
	if (m_folderVisited)
	{
		SaveColumnWidths();
	}

	ClearPendingResults();

	m_shellChangeWatcher.StopWatchingAll();

	ListView_DeleteAllItems(m_hListView);

	if (m_folderVisited)
	{
		ResetFolderState();

		// The folder is about to change, so any previous WeakPtrs are no longer needed.
		m_weakPtrFactory.InvalidateWeakPtrs();
	}
}

void ShellBrowserImpl::ClearPendingResults()
{
	m_columnThreadPool.clear_queue();
	m_columnResults.clear();

	m_iconFetcher->ClearQueue();

	m_thumbnailThreadPool.clear_queue();
	m_thumbnailResults.clear();

	m_infoTipsThreadPool.clear_queue();
	m_infoTipResults.clear();
}

void ShellBrowserImpl::StoreCurrentlySelectedItems()
{
	auto *entry = m_navigationController->GetCurrentEntry();
	auto selectedItems = GetSelectedItemPidls();
	entry->SetSelectedItems(selectedItems);
}

void ShellBrowserImpl::ResetFolderState()
{
	ListView_SetImageList(m_hListView, nullptr, LVSIL_SMALL);
	ListView_SetImageList(m_hListView, nullptr, LVSIL_NORMAL);

	ListView_RemoveAllGroups(m_hListView);

	m_directoryState = DirectoryState();

	EnterCriticalSection(&m_csDirectoryAltered);
	m_AlteredList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	m_itemInfoMap.clear();

	m_renamedItemOldPidl.reset();
}

void ShellBrowserImpl::NotifyShellOfNavigation(PCIDLIST_ABSOLUTE pidl)
{
	if (m_config->replaceExplorerMode == +DefaultFileManager::ReplaceExplorerMode::None)
	{
		return;
	}

	HRESULT hr = RegisterShellWindowIfNecessary(pidl);

	if (FAILED(hr))
	{
		return;
	}

	wil::unique_variant pidlVariant;
	hr = InitVariantFromBuffer(pidl, ILGetSize(pidl), &pidlVariant);

	if (FAILED(hr))
	{
		return;
	}

	m_shellWindows->OnNavigate(m_shellWindowCookie.get(), &pidlVariant);
}

HRESULT ShellBrowserImpl::RegisterShellWindowIfNecessary(PCIDLIST_ABSOLUTE pidl)
{
	if (m_shellWindowRegistered)
	{
		return S_OK;
	}

	HRESULT hr = RegisterShellWindow(pidl);

	if (SUCCEEDED(hr))
	{
		m_shellWindowRegistered = true;
	}

	return hr;
}

// When the shell needs to find an existing window (to select a specific item), it will go through
// the following process:
//
// 1. Use IShellWindows::FindWindowSW() to query for a window of type SWC_BROWSER with the necessary
//    pidl.
// 2. Query for the IWebBrowserApp interface for that window (using the IDispatch interface that's
//    registered).
// 3. Call IWebBrowserApp::get_Document() to retrieve the IDispatch interface for the document.
// 4. Call QueryInterface() on the IDispatch interface to request IServiceProvider.
// 5. Call IServiceProvider::QueryService() to request an IID_IFolderView service and IID_IShellView
//    interface.
// 6. Call IShellView::SelectItem() to select the appropriate item.
//
// Note that the shell will also bring the window it finds to the foreground as part of this
// process.
//
// A similar process also occurs when simply searching for an existing shell window. For example, if
// the user double clicks the recycle bin icon on the desktop, any existing recycle bin window will
// be brought to the foreground if present.
HRESULT ShellBrowserImpl::RegisterShellWindow(PCIDLIST_ABSOLUTE pidl)
{
	if (!m_shellWindows)
	{
		return E_FAIL;
	}

	wil::unique_variant pidlVariant;
	RETURN_IF_FAILED(InitVariantFromBuffer(pidl, ILGetSize(pidl), &pidlVariant));

	wil::unique_variant empty;

	m_shellWindowCookie.associate(m_shellWindows.get());

	// Note that while the documentation states that the pidl variant (the second argument) must be
	// "of type VT_VARIANT | VT_BYREF", it appears that's not actually necessary (and would be more
	// complicated, since there would need to be two variants).
	// Also, an empty variant must be supplied for the third argument (in contrast to the
	// documentation, which states that null is allowed).
	// Additionally, the shell window type that's set is SWC_BROWSER, which the documentation states
	// is for "An Internet Explorer (Iexplore.exe) browser window.". While a value like SWC_3RDPARTY
	// or SWC_EXPLORER might make more sense, the shell looks for Explorer windows using the
	// SWC_BROWSER type. So for a window to be found, it needs to use that specific type.
	// Finally, there are two possible ways of supplying a pidl: using
	// IShellWindows::RegisterPending() or IShellWindows::OnNavigate(). The second
	// method is called during each navigation anyway, so it would be simpler to just use that,
	// however calling IShellWindows::RegisterPending() is necessary.
	// If a new instance is launched as part of a SHOpenFolderAndSelectItems() call, then no
	// selection request will be made unless IShellWindows::RegisterPending() is called. In this
	// case, it's redundant, since IShellWindows::OnNavigate() will be called anyway, which will
	// supply a pidl, but it appears that that's not enough.
	// Therefore, that's the only reason this method is called.
	RETURN_IF_FAILED(m_shellWindows->RegisterPending(GetCurrentThreadId(), &pidlVariant, &empty,
		SWC_BROWSER, &m_shellWindowCookie));

	auto document = winrt::make_self<DocumentServiceProvider>();
	auto shellView =
		winrt::make_self<ShellView>(m_weakPtrFactory.GetWeakPtr(), m_tabNavigation, true);
	document->RegisterService(IID_IFolderView, shellView.get());

	auto browserApp = winrt::make_self<WebBrowserApp>(m_hOwner, document.get());

	// Registering the same window multiple times (ultimately with different pidls) is odd, but
	// appears to work fine (since a shell window is uniquely identified through a cookie, rather
	// than a window handle).
	// Registering each listview wouldn't work, as it wouldn't make sense for the shell to bring the
	// listview windows to the foreground or activate them (doing so would break the way tabs are
	// managed).
	// This has implications for the way the appropriate tab is selected. Since the shell will only
	// bring the top-level window to the foreground, the appropriate tab will be selected when the
	// item selection is set (via a call to the IShellView instance set up above).
	// Note that the cast from HWND to long causes warnings, but the warnings can be safely ignored.
	// Only the lower 32 bits of a handle are important (see the discussion at
	// https://stackoverflow.com/q/1822667).
	long registeredCookie;
#pragma warning(push)
#pragma warning(                                                                                   \
	disable : 4311 4302) // 'reinterpret_cast': pointer truncation from 'HWND' to 'long',
						 // 'reinterpret_cast': truncation from 'HWND' to 'long'
	RETURN_IF_FAILED(m_shellWindows->Register(browserApp.get(), reinterpret_cast<long>(m_hOwner),
		SWC_BROWSER, &registeredCookie));
#pragma warning(pop)

	// The call to RegisterPending() above is passed the thread ID. The call to Register() will use
	// that thread ID to link a pending window to the specified window handle. That means the cookie
	// values for the two calls should be the same - since they refer to the same window instance.
	DCHECK(registeredCookie == m_shellWindowCookie.get());

	return S_OK;
}

std::optional<int> ShellBrowserImpl::AddItemInternal(IShellFolder *shellFolder,
	PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild, int itemIndex, BOOL setPosition)
{
	auto itemInfo = GetItemInformation(shellFolder, pidlDirectory, pidlChild);

	if (!itemInfo)
	{
		return std::nullopt;
	}

	return AddItemInternal(itemIndex, *itemInfo, setPosition);
}

int ShellBrowserImpl::AddItemInternal(int itemIndex, const ItemInfo_t &itemInfo, BOOL setPosition)
{
	int itemId = GenerateUniqueItemId();
	m_itemInfoMap.insert({ itemId, itemInfo });

	AwaitingAdd_t awaitingAdd;

	if (itemIndex == -1)
	{
		awaitingAdd.iItem =
			m_directoryState.numItems + static_cast<int>(m_directoryState.awaitingAddList.size());
	}
	else
	{
		awaitingAdd.iItem = itemIndex;
	}

	awaitingAdd.iItemInternal = itemId;
	awaitingAdd.bPosition = setPosition;
	awaitingAdd.iAfter = itemIndex - 1;

	m_directoryState.awaitingAddList.push_back(awaitingAdd);

	return itemId;
}

std::optional<ShellBrowserImpl::ItemInfo_t> ShellBrowserImpl::GetItemInformation(
	IShellFolder *shellFolder, PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD pidlChild)
{
	ItemInfo_t itemInfo;

	itemInfo.pidlComplete.TakeOwnership(ILCombine(pidlDirectory, pidlChild));
	itemInfo.pridl = pidlChild;

	std::wstring parsingName;
	HRESULT hr = GetDisplayName(shellFolder, pidlChild, SHGDN_FORPARSING, parsingName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	itemInfo.parsingName = parsingName;

	ULONG attributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
	PCITEMID_CHILD items[] = { pidlChild };
	hr = shellFolder->GetAttributesOf(1, items, &attributes);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	SHGDNF displayNameFlags = SHGDN_INFOLDER;

	unique_pidl_absolute recycleBinPidl;
	hr = SHGetKnownFolderIDList(FOLDERID_RecycleBinFolder, KF_FLAG_DEFAULT, nullptr,
		wil::out_param(recycleBinPidl));

	bool isRecycleBin = SUCCEEDED(hr) && ArePidlsEquivalent(pidlDirectory, recycleBinPidl.get());

	// SHGDN_INFOLDER | SHGDN_FORPARSING is used to ensure that the name retrieved for a filesystem
	// file contains an extension, even if extensions are hidden in Windows Explorer. When using
	// SHGDN_INFOLDER by itself, the resulting name won't contain an extension if extensions are
	// hidden in Windows Explorer.
	// Note that the recycle bin is excluded here, as the parsing names for the items are completely
	// different to their regular display names.
	if (!isRecycleBin && WI_IsFlagSet(attributes, SFGAO_FILESYSTEM)
		&& WI_IsFlagClear(attributes, SFGAO_FOLDER))
	{
		WI_SetFlag(displayNameFlags, SHGDN_FORPARSING);
	}

	std::wstring displayName;
	hr = GetDisplayName(shellFolder, pidlChild, displayNameFlags, displayName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	itemInfo.displayName = displayName;

	std::wstring editingName;
	hr = GetDisplayName(shellFolder, pidlChild, SHGDN_INFOLDER | SHGDN_FOREDITING, editingName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	itemInfo.editingName = editingName;

	if (PathIsRoot(parsingName.c_str()))
	{
		itemInfo.bDrive = TRUE;
		StringCchCopy(itemInfo.szDrive, std::size(itemInfo.szDrive), parsingName.c_str());
	}
	else
	{
		itemInfo.bDrive = FALSE;
	}

	WIN32_FIND_DATA wfd;
	hr = SHGetDataFromIDList(shellFolder, pidlChild, SHGDFIL_FINDDATA, &wfd, sizeof(wfd));

	if (FAILED(hr))
	{
		hr = ExtractFindDataUsingPropertyStore(shellFolder, pidlChild, wfd);
	}

	if (SUCCEEDED(hr))
	{
		itemInfo.wfd = wfd;
		itemInfo.isFindDataValid = true;
	}
	else
	{
		StringCchCopy(itemInfo.wfd.cFileName, std::size(itemInfo.wfd.cFileName),
			displayName.c_str());

		if (WI_IsFlagSet(attributes, SFGAO_FOLDER))
		{
			WI_SetFlag(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
		}
	}

	return itemInfo;
}

HRESULT ShellBrowserImpl::ExtractFindDataUsingPropertyStore(IShellFolder *shellFolder,
	PCITEMID_CHILD pidlChild, WIN32_FIND_DATA &output)
{
	wil::com_ptr_nothrow<IPropertyStoreFactory> factory;
	HRESULT hr = shellFolder->BindToObject(pidlChild, nullptr, IID_PPV_ARGS(&factory));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IPropertyStore> store;
	PROPERTYKEY keys[] = { PKEY_FindData };
	hr = factory->GetPropertyStoreForKeys(keys, std::size(keys), GPS_FASTPROPERTIESONLY,
		IID_PPV_ARGS(&store));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::unique_prop_variant findDataProp;
	hr = store->GetValue(PKEY_FindData, &findDataProp);

	if (FAILED(hr))
	{
		return hr;
	}

	if (PropVariantGetElementCount(findDataProp) != sizeof(WIN32_FIND_DATA))
	{
		return hr;
	}

	WIN32_FIND_DATA wfd;
	hr = PropVariantToBuffer(findDataProp, &wfd, sizeof(wfd));

	if (FAILED(hr))
	{
		return hr;
	}

	output = wfd;

	return hr;
}

void ShellBrowserImpl::OnNavigationWillCommit(const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(request);

	// The folder is going to change, so update the set of selected items before the current
	// navigation entry changes.
	StoreCurrentlySelectedItems();

	SetNavigationState(NavigationState::WillCommit);
}

void ShellBrowserImpl::OnNavigationComitted(const NavigationRequest *request,
	const std::vector<PidlChild> &items)
{
	ChangeFolders(request->GetNavigateParams().pidl);

	NotifyShellOfNavigation(request->GetNavigateParams().pidl.Raw());

	RecalcWindowCursor(m_hListView);

	AddNavigationItems(request, items);

	SetNavigationState(NavigationState::Committed);
}

void ShellBrowserImpl::AddNavigationItems(const NavigationRequest *request,
	const std::vector<PidlChild> &itemPidls)
{
	auto items = GetItemInformationFromPidls(request, itemPidls);

	for (auto &item : items)
	{
		AddItemInternal(-1, item, FALSE);
	}

	ScopedRedrawDisabler redrawDisabler(m_hListView);

	InsertAwaitingItems();
	SortFolder();

	ListView_EnsureVisible(m_hListView, 0, FALSE);

	/* Set the focus back to the first item. */
	ListView_SetItemState(m_hListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);

	// A history entry should be created when the navigation is committed, so the current entry
	// should always be for the current navigation.
	auto *currentEntry = m_navigationController->GetCurrentEntry();
	DCHECK(currentEntry->GetPidl() == request->GetNavigateParams().pidl);

	SelectItems(currentEntry->GetSelectedItems());

	if (request->GetNavigateParams().navigationType == NavigationType::Up)
	{
		SelectItems({ request->GetNavigateParams().originalPidl });
	}

	if (m_config->shellChangeNotificationType == ShellChangeNotificationType::All
		|| (m_config->shellChangeNotificationType == ShellChangeNotificationType::NonFilesystem
			&& m_directoryState.virtualFolder))
	{
		StartDirectoryMonitoring(m_directoryState.pidlDirectory.Raw());
	}
}

std::vector<ShellBrowserImpl::ItemInfo_t> ShellBrowserImpl::GetItemInformationFromPidls(
	const NavigationRequest *request, const std::vector<PidlChild> &itemPidls)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, request->GetNavigateParams().pidl.Raw(), nullptr,
		IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return {};
	}

	std::vector<ItemInfo_t> items;

	for (const auto &pidl : itemPidls)
	{
		auto item = GetItemInformation(shellFolder.get(), request->GetNavigateParams().pidl.Raw(),
			pidl.Raw());

		if (item)
		{
			items.push_back(*item);
		}
	}

	return items;
}

void ShellBrowserImpl::InsertAwaitingItems()
{
	int nPrevItems = ListView_GetItemCount(m_hListView);

	if (nPrevItems == 0 && m_directoryState.awaitingAddList.empty())
	{
		m_directoryState.numItems = 0;

		return;
	}

	/* Make the listview allocate space (for internal data structures)
	for all the items at once, rather than individually.
	Acts as a speed optimization. */
	ListView_SetItemCount(m_hListView, m_directoryState.awaitingAddList.size() + nPrevItems);

	if (m_folderSettings.autoArrange)
	{
		ListViewHelper::SetAutoArrange(m_hListView, false);
	}

	int nAdded = 0;
	std::optional<int> itemToRename;

	for (const auto &awaitingItem : m_directoryState.awaitingAddList)
	{
		const auto &itemInfo = m_itemInfoMap.at(awaitingItem.iItemInternal);

		if (IsFileFiltered(itemInfo))
		{
			m_directoryState.filteredItemsList.insert(awaitingItem.iItemInternal);
			continue;
		}

		BasicItemInfo_t basicItemInfo = getBasicItemInfo(awaitingItem.iItemInternal);
		std::wstring filename = ProcessItemFileName(basicItemInfo, m_config->globalFolderSettings);

		LVITEM lv;
		lv.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

		if (m_folderSettings.showInGroups)
		{
			int groupId = DetermineItemGroup(awaitingItem.iItemInternal);

			lv.mask |= LVIF_GROUPID;
			lv.iGroupId = groupId;

			EnsureGroupExistsInListView(groupId);
		}

		lv.iItem = awaitingItem.iItem;
		lv.iSubItem = 0;

		auto firstColumn = GetFirstCheckedColumn();

		if ((m_folderSettings.viewMode == +ViewMode::Details)
			&& firstColumn.type != +ColumnType::Name)
		{
			lv.pszText = LPSTR_TEXTCALLBACK;
		}
		else
		{
			lv.pszText = filename.data();
		}

		lv.iImage = I_IMAGECALLBACK;
		lv.lParam = awaitingItem.iItemInternal;

		/* Insert the item into the list view control. */
		int iItemIndex = ListView_InsertItem(m_hListView, &lv);

		if (awaitingItem.bPosition && m_folderSettings.viewMode != +ViewMode::Details)
		{
			POINT ptItem;

			if (awaitingItem.iAfter != -1)
			{
				ListView_GetItemPosition(m_hListView, awaitingItem.iAfter, &ptItem);
			}
			else
			{
				ptItem.x = 0;
				ptItem.y = 0;
			}

			/* The item will end up in the position AFTER iAfter. */
			ListView_SetItemPosition32(m_hListView, iItemIndex, ptItem.x, ptItem.y);
		}

		if (m_folderSettings.viewMode == +ViewMode::Tiles)
		{
			SetTileViewItemInfo(iItemIndex, awaitingItem.iItemInternal);
		}

		if (m_directoryState.queuedRenameItem.HasValue()
			&& ArePidlsEquivalent(itemInfo.pidlComplete.Raw(),
				m_directoryState.queuedRenameItem.Raw()))
		{
			itemToRename = iItemIndex;
		}

		auto selectItr = std::find_if(m_directoryState.filesToSelect.begin(),
			m_directoryState.filesToSelect.end(), [&itemInfo](const auto &pidl)
			{ return ArePidlsEquivalent(pidl.Raw(), itemInfo.pidlComplete.Raw()); });

		if (selectItr != m_directoryState.filesToSelect.end())
		{
			ListViewHelper::SelectItem(m_hListView, iItemIndex, true);

			int selectedCount = ListView_GetSelectedCount(m_hListView);

			if (selectedCount == 1)
			{
				ListViewHelper::FocusItem(m_hListView, iItemIndex, true);
				ListView_EnsureVisible(m_hListView, iItemIndex, FALSE);
			}

			m_directoryState.filesToSelect.erase(selectItr);
		}

		/* If the file is marked as hidden, ghost it out. */
		if (itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			ListView_SetItemState(m_hListView, iItemIndex, LVIS_CUT, LVIS_CUT);
		}

		/* Add the current file's size to the running size of the current directory. */
		/* A folder may or may not have 0 in its high file size member.
		It should either be zeroed, or never counted. */
		ULARGE_INTEGER ulFileSize;
		ulFileSize.LowPart = itemInfo.wfd.nFileSizeLow;
		ulFileSize.HighPart = itemInfo.wfd.nFileSizeHigh;

		m_directoryState.totalDirSize += ulFileSize.QuadPart;

		nAdded++;
	}

	if (m_folderSettings.autoArrange)
	{
		ListViewHelper::SetAutoArrange(m_hListView, true);
	}

	m_directoryState.numItems = nPrevItems + nAdded;

	m_directoryState.awaitingAddList.clear();

	if (itemToRename)
	{
		m_directoryState.queuedRenameItem.Reset();
		ListView_EditLabel(m_hListView, *itemToRename);
	}
}

BOOL ShellBrowserImpl::IsFileFiltered(const ItemInfo_t &itemInfo) const
{
	BOOL bHideSystemFile = FALSE;
	BOOL bFilenameFiltered = FALSE;

	if (m_folderSettings.applyFilter
		&& ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
	{
		bFilenameFiltered = IsFilenameFiltered(itemInfo.displayName.c_str());
	}

	if (m_config->globalFolderSettings.hideSystemFiles)
	{
		bHideSystemFile =
			(itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == FILE_ATTRIBUTE_SYSTEM;
	}

	return bFilenameFiltered || bHideSystemFile;
}

void ShellBrowserImpl::RemoveItem(int iItemInternal)
{
	ULARGE_INTEGER ulFileSize;
	LVFINDINFO lvfi;
	int iItem;
	int nItems;

	if (iItemInternal == -1)
	{
		return;
	}

	/* Take the file size of the removed file away from the total
	directory size. */
	ulFileSize.LowPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow;
	ulFileSize.HighPart = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh;

	m_directoryState.totalDirSize -= ulFileSize.QuadPart;

	/* Locate the item within the listview.
	Could use filename, providing removed
	items are always deleted before new
	items are inserted. */
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = iItemInternal;
	iItem = ListView_FindItem(m_hListView, -1, &lvfi);

	if (iItem != -1)
	{
		if (m_folderSettings.showInGroups)
		{
			auto groupId = GetItemGroupId(iItem);

			if (groupId)
			{
				OnItemRemovedFromGroup(*groupId);
			}
		}

		/* Remove the item from the listview. */
		ListView_DeleteItem(m_hListView, iItem);
	}

	m_directoryState.filteredItemsList.erase(iItemInternal);
	m_itemInfoMap.erase(iItemInternal);

	nItems = ListView_GetItemCount(m_hListView);

	m_directoryState.numItems--;
}

ShellNavigationController *ShellBrowserImpl::GetNavigationController() const
{
	return m_navigationController.get();
}

void ShellBrowserImpl::SetNavigationState(NavigationState navigationState)
{
	if (navigationState == NavigationState::WillCommit)
	{
		CHECK(m_navigationState == NavigationState::NoFolderShown
			|| m_navigationState == NavigationState::Committed);
	}
	else if (navigationState == NavigationState::Committed)
	{
		CHECK(m_navigationState == NavigationState::WillCommit);
	}
	else
	{
		CHECK(false);
	}

	m_navigationState = navigationState;
}
