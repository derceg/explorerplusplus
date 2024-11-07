// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LoadSaveInterface.h"
#include <wil/com.h>
#include <MsXml2.h>
#include <objbase.h>

class App;
class Explorerplusplus;

class LoadSaveXML : public ILoadSave
{
public:
	LoadSaveXML(App *app, Explorerplusplus *pContainer, BOOL bLoad);
	~LoadSaveXML();

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
	void InitializeLoadEnvironment();
	void ReleaseLoadEnvironment();
	void InitializeSaveEnvironment();
	void ReleaseSaveEnvironment();

	App *const m_app;
	Explorerplusplus *m_pContainer;
	BOOL m_bLoad;

	/* Used for saving + loading. */
	wil::com_ptr_nothrow<IXMLDOMDocument> m_pXMLDom;

	/* Used exclusively for saving. */
	wil::com_ptr_nothrow<IXMLDOMElement> m_pRoot;
};
