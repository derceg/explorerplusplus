// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct FolderColumns;

namespace ColumnRegistryStorage
{

void LoadAllColumnSets(HKEY parentKey, FolderColumns &folderColumns);
void SaveAllColumnSets(HKEY parentKey, const FolderColumns &folderColumns);

}
