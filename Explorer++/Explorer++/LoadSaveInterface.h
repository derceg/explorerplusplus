// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

/* Save/load interface. This allows multiple
methods of saving/loading data, as long as it
conforms to this specification. */
class ILoadSave
{
public:
	virtual ~ILoadSave() = default;

	/* Loading functions. */
	virtual void LoadGenericSettings() = 0;
	virtual void LoadPreviousTabs() = 0;
	virtual void LoadDefaultColumns() = 0;
	virtual void LoadMainRebarInformation() = 0;

	/* Saving functions. */
	virtual void SaveGenericSettings() = 0;
	virtual void SaveBookmarks() = 0;
	virtual void SaveTabs() = 0;
	virtual void SaveDefaultColumns() = 0;
	virtual void SaveApplicationToolbar() = 0;
	virtual void SaveMainRebarInformation() = 0;
	virtual void SaveColorRules() = 0;
	virtual void SaveDialogStates() = 0;
};
