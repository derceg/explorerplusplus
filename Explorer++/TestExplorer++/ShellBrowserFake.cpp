// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFake.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellEnumeratorFake.h"
#include "ShellTestHelper.h"

ShellBrowserFake::ShellBrowserFake(NavigationEvents *navigationEvents,
	TabNavigationInterface *tabNavigation,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry,
	std::shared_ptr<concurrencpp::executor> enumerationExecutor,
	std::shared_ptr<concurrencpp::executor> originalExecutor) :
	ShellBrowserFake(navigationEvents, tabNavigation, enumerationExecutor, originalExecutor)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, &m_navigationManager,
		navigationEvents, tabNavigation, preservedEntries, currentEntry);
}

ShellBrowserFake::ShellBrowserFake(NavigationEvents *navigationEvents,
	TabNavigationInterface *tabNavigation,
	std::shared_ptr<concurrencpp::executor> enumerationExecutor,
	std::shared_ptr<concurrencpp::executor> originalExecutor) :
	m_shellEnumerator(std::make_unique<ShellEnumeratorFake>()),
	m_inlineExecutor(std::make_shared<concurrencpp::inline_executor>()),
	m_navigationManager(this, navigationEvents, m_shellEnumerator,
		enumerationExecutor ? enumerationExecutor : m_inlineExecutor,
		originalExecutor ? originalExecutor : m_inlineExecutor),
	m_navigationController(std::make_unique<ShellNavigationController>(this, &m_navigationManager,
		navigationEvents, tabNavigation, CreateSimplePidlForTest(L"c:\\initial_path")))
{
}

ShellBrowserFake::~ShellBrowserFake()
{
	m_destroyedSignal();

	m_inlineExecutor->shutdown();
}

// Although the ShellNavigationController can navigate to a path (by transforming it into a pidl),
// it requires that the path exist. This function will transform the path into a simple pidl, which
// doesn't require the path to exist.
void ShellBrowserFake::NavigateToPath(const std::wstring &path, HistoryEntryType addHistoryType,
	PidlAbsolute *outputPidl)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw(), addHistoryType);
	m_navigationController->Navigate(navigateParams);

	if (outputPidl)
	{
		*outputPidl = pidl;
	}
}

NavigationManager *ShellBrowserFake::GetNavigationManager()
{
	return &m_navigationManager;
}

const NavigationManager *ShellBrowserFake::GetNavigationManager() const
{
	return &m_navigationManager;
}

FolderSettings ShellBrowserFake::GetFolderSettings() const
{
	return {};
}

ShellNavigationController *ShellBrowserFake::GetNavigationController() const
{
	return m_navigationController.get();
}

ViewMode ShellBrowserFake::GetViewMode() const
{
	return m_folderSettings.viewMode;
}

void ShellBrowserFake::SetViewMode(ViewMode viewMode)
{
	m_folderSettings.viewMode = viewMode;
}

SortMode ShellBrowserFake::GetSortMode() const
{
	return m_folderSettings.sortMode;
}

void ShellBrowserFake::SetSortMode(SortMode sortMode)
{
	m_folderSettings.sortMode = sortMode;
}

SortDirection ShellBrowserFake::GetSortDirection() const
{
	return m_folderSettings.sortDirection;
}

void ShellBrowserFake::SetSortDirection(SortDirection direction)
{
	m_folderSettings.sortDirection = direction;
}

bool ShellBrowserFake::IsAutoArrangeEnabled() const
{
	return m_folderSettings.autoArrangeEnabled;
}

void ShellBrowserFake::SetAutoArrangeEnabled(bool enabled)
{
	m_folderSettings.autoArrangeEnabled = enabled;
}

bool ShellBrowserFake::CanAutoSizeColumns() const
{
	return false;
}

void ShellBrowserFake::AutoSizeColumns()
{
}

bool ShellBrowserFake::CanCreateNewFolder() const
{
	return false;
}

void ShellBrowserFake::CreateNewFolder()
{
}

bool ShellBrowserFake::CanSplitFile() const
{
	return false;
}

void ShellBrowserFake::SplitFile()
{
}

bool ShellBrowserFake::CanMergeFiles() const
{
	return false;
}

void ShellBrowserFake::MergeFiles()
{
}

void ShellBrowserFake::SelectAllItems()
{
}

void ShellBrowserFake::InvertSelection()
{
}

bool ShellBrowserFake::CanStartWildcardSelection(SelectionType selectionType) const
{
	UNREFERENCED_PARAMETER(selectionType);

	return false;
}

void ShellBrowserFake::StartWildcardSelection(SelectionType selectionType)
{
	UNREFERENCED_PARAMETER(selectionType);
}

void ShellBrowserFake::SelectItemsMatchingPattern(const std::wstring &pattern,
	SelectionType selectionType)
{
	UNREFERENCED_PARAMETER(pattern);
	UNREFERENCED_PARAMETER(selectionType);
}

void ShellBrowserFake::ClearSelection()
{
}

std::wstring ShellBrowserFake::GetFilterText() const
{
	return m_folderSettings.filter;
}

void ShellBrowserFake::SetFilterText(const std::wstring &filter)
{
	m_folderSettings.filter = filter;
}

bool ShellBrowserFake::IsFilterCaseSensitive() const
{
	return m_folderSettings.filterCaseSensitive;
}

void ShellBrowserFake::SetFilterCaseSensitive(bool caseSensitive)
{
	m_folderSettings.filterCaseSensitive = caseSensitive;
}

bool ShellBrowserFake::IsFilterEnabled() const
{
	return m_folderSettings.filterEnabled;
}

void ShellBrowserFake::SetFilterEnabled(bool enabled)
{
	m_folderSettings.filterEnabled = enabled;
}

void ShellBrowserFake::EditFilterSettings()
{
}

bool ShellBrowserFake::CanSaveDirectoryListing() const
{
	return false;
}

void ShellBrowserFake::SaveDirectoryListing()
{
}

boost::signals2::connection ShellBrowserFake::AddDestroyedObserver(
	const DestroyedSignal::slot_type &observer)
{
	return m_destroyedSignal.connect(observer);
}
