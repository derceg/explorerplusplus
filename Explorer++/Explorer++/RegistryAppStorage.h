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

	void LoadBookmarks(BookmarkTree *bookmarkTree) override;
	void LoadColorRules(ColorRuleModel *model) override;
	void LoadApplications(Applications::ApplicationModel *model) override;

private:
	const wil::unique_hkey m_applicationKey;
};
