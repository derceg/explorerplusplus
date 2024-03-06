// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct Column_t;
struct FolderColumns;

enum class ColumnValidationType
{
	RealFolder,
	ControlPanel,
	MyComputer,
	RecycleBin,
	Printers,
	NetworkConnections,
	MyNetworkPlaces
};

void ValidateColumns(FolderColumns &folderColumns);
void ValidateSingleColumnSet(ColumnValidationType columnValidationType,
	std::vector<Column_t> &columns);
