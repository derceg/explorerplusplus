// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LoadSaveInterface.h"
#include <wil/com.h>
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

	/* Used for saving + loading. */
	wil::com_ptr_nothrow<IXMLDOMDocument> m_pXMLDom;

	/* Used exclusively for saving. */
	wil::com_ptr_nothrow<IXMLDOMElement> m_pRoot;
};