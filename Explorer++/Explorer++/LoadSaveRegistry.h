// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LoadSaveInterface.h"

class Explorerplusplus;

class LoadSaveRegistry : public ILoadSave
{
public:
	LoadSaveRegistry(Explorerplusplus *pContainer);

	/* Loading functions. */
	void LoadGenericSettings() override;
	void LoadBookmarks() override;
	void LoadPreviousTabs() override;
	void LoadDefaultColumns() override;
	void LoadApplicationToolbar() override;
	void LoadMainRebarInformation() override;
	void LoadColorRules() override;
	void LoadDialogStates() override;

	/* Saving functions. */
	void SaveGenericSettings() override;
	void SaveBookmarks() override;
	void SaveTabs() override;
	void SaveDefaultColumns() override;
	void SaveApplicationToolbar() override;
	void SaveMainRebarInformation() override;
	void SaveColorRules() override;
	void SaveDialogStates() override;

private:
	Explorerplusplus *m_pContainer;
};
