// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

namespace Applications
{

class ApplicationModel;

}

class BookmarkTree;
class ColorRuleModel;
struct Config;
struct FolderColumns;
struct WindowStorageData;

class AppStorage
{
public:
	virtual ~AppStorage() = default;

	virtual void LoadConfig(Config &config) = 0;
	[[nodiscard]] virtual std::vector<WindowStorageData> LoadWindows() = 0;
	virtual void LoadBookmarks(BookmarkTree *bookmarkTree) = 0;
	virtual void LoadColorRules(ColorRuleModel *model) = 0;
	virtual void LoadApplications(Applications::ApplicationModel *model) = 0;
	virtual void LoadDialogStates() = 0;
	virtual void LoadDefaultColumns(FolderColumns &defaultColumns) = 0;

	virtual void SaveConfig(const Config &config) = 0;
	virtual void SaveWindows(const std::vector<WindowStorageData> &windows) = 0;
	virtual void SaveBookmarks(const BookmarkTree *bookmarkTree) = 0;
	virtual void SaveColorRules(const ColorRuleModel *model) = 0;
	virtual void SaveApplications(const Applications::ApplicationModel *model) = 0;
	virtual void SaveDialogStates() = 0;
	virtual void SaveDefaultColumns(const FolderColumns &defaultColumns) = 0;
	virtual void Commit() = 0;
};
