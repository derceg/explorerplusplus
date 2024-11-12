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
struct FolderColumns;
struct WindowStorageData;

class AppStorage
{
public:
	virtual ~AppStorage() = default;

	[[nodiscard]] virtual std::vector<WindowStorageData> LoadWindows() = 0;
	virtual void LoadBookmarks(BookmarkTree *bookmarkTree) = 0;
	virtual void LoadColorRules(ColorRuleModel *model) = 0;
	virtual void LoadApplications(Applications::ApplicationModel *model) = 0;
	virtual void LoadDialogStates() = 0;
	virtual void LoadDefaultColumns(FolderColumns &defaultColumns) = 0;
};
