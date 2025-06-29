// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SelectionType.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/PidlHelper.h"
#include <boost/signals2.hpp>

struct FolderColumns;
struct FolderSettings;
class NavigationManager;
class NavigationRequest;
class ShellNavigationController;
class Tab;

class ShellBrowser
{
public:
	using DestroyedSignal = boost::signals2::signal<void()>;

	ShellBrowser();
	virtual ~ShellBrowser() = default;

	int GetId() const;

	const Tab *GetTab() const;
	void SetTab(const Tab *tab);

	const PidlAbsolute &GetDirectory() const;
	const NavigationRequest *MaybeGetLatestActiveNavigation() const;

	virtual const FolderSettings &GetFolderSettings() const = 0;
	virtual ShellNavigationController *GetNavigationController() const = 0;

	virtual ViewMode GetViewMode() const = 0;
	virtual void SetViewMode(ViewMode viewMode) = 0;

	virtual SortMode GetSortMode() const = 0;
	virtual void SetSortMode(SortMode sortMode) = 0;
	virtual SortDirection GetSortDirection() const = 0;
	virtual void SetSortDirection(SortDirection direction) = 0;

	virtual void SetColumns(const FolderColumns &folderColumns) = 0;
	virtual const FolderColumns &GetColumns() = 0;

	virtual bool IsAutoArrangeEnabled() const = 0;
	virtual void SetAutoArrangeEnabled(bool enabled) = 0;
	virtual bool CanAutoSizeColumns() const = 0;
	virtual void AutoSizeColumns() = 0;

	virtual bool CanCreateNewFolder() const = 0;
	virtual void CreateNewFolder() = 0;
	virtual bool CanSplitFile() const = 0;
	virtual void SplitFile() = 0;
	virtual bool CanMergeFiles() const = 0;
	virtual void MergeFiles() = 0;

	virtual void SelectAllItems() = 0;
	virtual void InvertSelection() = 0;
	virtual bool CanStartWildcardSelection(SelectionType selectionType) const = 0;
	virtual void StartWildcardSelection(SelectionType selectionType) = 0;
	virtual void SelectItemsMatchingPattern(const std::wstring &pattern,
		SelectionType selectionType) = 0;
	virtual bool CanClearSelection() const = 0;
	virtual void ClearSelection() = 0;

	virtual std::wstring GetFilterText() const = 0;
	virtual void SetFilterText(const std::wstring &filter) = 0;
	virtual bool IsFilterCaseSensitive() const = 0;
	virtual void SetFilterCaseSensitive(bool caseSensitive) = 0;
	virtual bool IsFilterEnabled() const = 0;
	virtual void SetFilterEnabled(bool enabled) = 0;
	virtual void EditFilterSettings() = 0;

	virtual bool CanSaveDirectoryListing() const = 0;
	virtual void SaveDirectoryListing() = 0;

	virtual boost::signals2::connection AddDestroyedObserver(
		const DestroyedSignal::slot_type &observer) = 0;

protected:
	virtual NavigationManager *GetNavigationManager() = 0;
	virtual const NavigationManager *GetNavigationManager() const = 0;

private:
	static inline int idCounter = 1;
	const int m_id;

	const Tab *m_tab = nullptr;
};
