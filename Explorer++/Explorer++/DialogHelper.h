// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Pidl.h"
#include <optional>
#include <vector>

class ResourceLoader;

namespace DialogHelper
{

struct ItemPidlAndFindData
{
	ItemPidlAndFindData(const PidlAbsolute &pidl, std::optional<WIN32_FIND_DATA> findData = {}) :
		pidl(pidl),
		findData(findData)
	{
	}

	PidlAbsolute pidl;
	std::optional<WIN32_FIND_DATA> findData;
};

bool CanShowSetFileAttributesDialogForItems(const std::vector<PidlAbsolute> &pidls);
void MaybeShowSetFileAttributesDialog(const ResourceLoader *resourceLoader, HWND parent,
	const std::vector<ItemPidlAndFindData> &items);

}
