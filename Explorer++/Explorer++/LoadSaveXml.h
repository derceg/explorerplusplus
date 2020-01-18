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