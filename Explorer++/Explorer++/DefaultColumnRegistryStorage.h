// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct FolderColumns;

namespace DefaultColumnRegistryStorage
{

void Load(HKEY applicationKey, FolderColumns &defaultColumns);
void Save(HKEY applicationKey, const FolderColumns &defaultColumns);

}
