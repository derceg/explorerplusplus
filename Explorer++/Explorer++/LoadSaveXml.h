// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LoadSaveInterface.h"
#include <MsXml2.h>
#include <objbase.h>

class Explorerplusplus;

class LoadSaveXML : public ILoadSave
{
public:

	LoadSaveXML(Explorerplusplus *pContainer, BOOL bLoad);
	~LoadSaveXML();

	/* Loading functions. */
	void	LoadGenericSettings() override;
	void	LoadBookmarks() override;
	int		LoadPreviousTabs() override;
	void	LoadDefaultColumns() override;
	void	LoadApplicationToolbar() override;
	void	LoadToolbarInformation() override;
	void	LoadColorRules() override;
	void	LoadDialogStates() override;

	/* Saving functions. */
	void	SaveGenericSettings() override;
	void	SaveBookmarks() override;
	void	SaveTabs() override;
	void	SaveDefaultColumns() override;
	void	SaveApplicationToolbar() override;
	void	SaveToolbarInformation() override;
	void	SaveColorRules() override;
	void	SaveDialogStates() override;

private:

	void	InitializeLoadEnvironment();
	void	ReleaseLoadEnvironment();
	void	InitializeSaveEnvironment();
	void	ReleaseSaveEnvironment();

	Explorerplusplus *m_pContainer;
	BOOL					m_bLoad;

	/* These are used for saving + loading. */
	IXMLDOMDocument *m_pXMLDom;

	/* Used exclusively for loading. */
	BOOL					m_bLoadedCorrectly;

	/* Used exclusively for saving. */
	IXMLDOMElement *m_pRoot;
};