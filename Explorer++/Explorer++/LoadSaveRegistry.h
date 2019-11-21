// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LoadSaveInterface.h"

class Explorerplusplus;

class CLoadSaveRegistry : public ILoadSave
{
public:

	CLoadSaveRegistry(Explorerplusplus *pContainer);

	/* Loading functions. */
	void	LoadGenericSettings();
	void	LoadBookmarks();
	int		LoadPreviousTabs();
	void	LoadDefaultColumns();
	void	LoadApplicationToolbar();
	void	LoadToolbarInformation();
	void	LoadColorRules();
	void	LoadDialogStates();

	/* Saving functions. */
	void	SaveGenericSettings();
	void	SaveBookmarks();
	void	SaveTabs();
	void	SaveDefaultColumns();
	void	SaveApplicationToolbar();
	void	SaveToolbarInformation();
	void	SaveColorRules();
	void	SaveDialogStates();

private:

	Explorerplusplus *m_pContainer;
};