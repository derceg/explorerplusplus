// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BookmarkTree;
class ColorRuleModel;

class AppStorage
{
public:
	virtual ~AppStorage() = default;

	virtual void LoadBookmarks(BookmarkTree *bookmarkTree) = 0;
	virtual void LoadColorRules(ColorRuleModel *model) = 0;
};
