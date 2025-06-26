// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/NavigationManager.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/PidlHelper.h"
#include <concurrencpp/concurrencpp.h>
#include <memory>

class NavigationEvents;
struct PreservedFolderState;
class PreservedHistoryEntry;
class ShellEnumeratorFake;
class ShellNavigationController;
class TabNavigationInterface;

class ShellBrowserFake : public ShellBrowser
{
public:
	ShellBrowserFake(NavigationEvents *navigationEvents, TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry, const PreservedFolderState &preservedFolderState);
	ShellBrowserFake(NavigationEvents *navigationEvents, TabNavigationInterface *tabNavigation,
		const FolderSettings &folderSettings = {}, const FolderColumns &initialColumns = {});
	~ShellBrowserFake();

	void NavigateToPath(const std::wstring &path,
		HistoryEntryType addHistoryType = HistoryEntryType::AddEntry,
		PidlAbsolute *outputPidl = nullptr);

	// ShellBrowser
	const FolderSettings &GetFolderSettings() const override;
	ShellNavigationController *GetNavigationController() const override;
	ViewMode GetViewMode() const override;
	void SetViewMode(ViewMode viewMode) override;
	SortMode GetSortMode() const override;
	void SetSortMode(SortMode sortMode) override;
	SortDirection GetSortDirection() const override;
	void SetSortDirection(SortDirection direction) override;
	void SetColumns(const FolderColumns &folderColumns) override;
	const FolderColumns &GetColumns() override;
	bool IsAutoArrangeEnabled() const override;
	void SetAutoArrangeEnabled(bool enabled) override;
	bool CanAutoSizeColumns() const override;
	void AutoSizeColumns() override;
	bool CanCreateNewFolder() const override;
	void CreateNewFolder() override;
	bool CanSplitFile() const override;
	void SplitFile() override;
	bool CanMergeFiles() const override;
	void MergeFiles() override;
	void SelectAllItems() override;
	void InvertSelection() override;
	bool CanStartWildcardSelection(SelectionType selectionType) const override;
	void StartWildcardSelection(SelectionType selectionType) override;
	void SelectItemsMatchingPattern(const std::wstring &pattern,
		SelectionType selectionType) override;
	void ClearSelection() override;
	std::wstring GetFilterText() const override;
	void SetFilterText(const std::wstring &filter) override;
	bool IsFilterCaseSensitive() const override;
	void SetFilterCaseSensitive(bool caseSensitive) override;
	bool IsFilterEnabled() const override;
	void SetFilterEnabled(bool enabled) override;
	void EditFilterSettings() override;
	bool CanSaveDirectoryListing() const override;
	void SaveDirectoryListing() override;
	boost::signals2::connection AddDestroyedObserver(
		const DestroyedSignal::slot_type &observer) override;

protected:
	NavigationManager *GetNavigationManager() override;
	const NavigationManager *GetNavigationManager() const override;

private:
	FolderSettings m_folderSettings;
	FolderColumns m_folderColumns;
	const std::shared_ptr<ShellEnumeratorFake> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::inline_executor> m_inlineExecutor;
	NavigationManager m_navigationManager;
	std::unique_ptr<ShellNavigationController> m_navigationController;
	DestroyedSignal m_destroyedSignal;
};
