// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LoadSaveInterface.h"

class App;
class Explorerplusplus;

class LoadSaveRegistry : public ILoadSave
{
public:
	LoadSaveRegistry(App *app, Explorerplusplus *pContainer);

	/* Loading functions. */
	void LoadGenericSettings() override;
	void LoadPreviousTabs() override;
	void LoadDefaultColumns() override;
	void LoadMainRebarInformation() override;
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
	App *const m_app;
	Explorerplusplus *m_pContainer;
};
