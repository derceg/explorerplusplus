// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AppStorage.h"
#include <wil/resource.h>

class BookmarkTree;

class RegistryAppStorage : public AppStorage
{
public:
	RegistryAppStorage(wil::unique_hkey applicationKey);

	void LoadConfig(Config &config) override;
	[[nodiscard]] std::vector<WindowStorageData> LoadWindows() override;
	void LoadBookmarks(BookmarkTree *bookmarkTree) override;
	void LoadColorRules(ColorRuleModel *model) override;
	void LoadApplications(Applications::ApplicationModel *model) override;
	void LoadDialogStates() override;
	void LoadDefaultColumns(FolderColumns &defaultColumns) override;
	void LoadFrequentLocations(FrequentLocationsModel *frequentLocationsModel) override;

	void SaveConfig(const Config &config) override;
	void SaveWindows(const std::vector<WindowStorageData> &windows) override;
	void SaveBookmarks(const BookmarkTree *bookmarkTree) override;
	void SaveColorRules(const ColorRuleModel *model) override;
	void SaveApplications(const Applications::ApplicationModel *model) override;
	void SaveDialogStates() override;
	void SaveDefaultColumns(const FolderColumns &defaultColumns) override;
	void SaveFrequentLocations(const FrequentLocationsModel *frequentLocationsModel) override;
	void Commit() override;

private:
	const wil::unique_hkey m_applicationKey;
};
