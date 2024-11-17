// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct WindowStorageData;

/* Save/load interface. This allows multiple
methods of saving/loading data, as long as it
conforms to this specification. */
class ILoadSave
{
public:
	virtual ~ILoadSave() = default;

	/* Loading functions. */
	virtual void LoadGenericSettings() = 0;

	/* Saving functions. */
	virtual void SaveGenericSettings() = 0;
	virtual void SaveWindows(const std::vector<WindowStorageData> &windows) = 0;
	virtual void SaveBookmarks() = 0;
	virtual void SaveDefaultColumns() = 0;
	virtual void SaveApplicationToolbar() = 0;
	virtual void SaveColorRules() = 0;
	virtual void SaveDialogStates() = 0;
};
