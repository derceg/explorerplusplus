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
	LoadSaveXML(App *app, Explorerplusplus *pContainer);
	~LoadSaveXML();

	void SaveGenericSettings() override;
	void SaveWindows(const std::vector<WindowStorageData> &windows) override;
	void SaveBookmarks() override;
	void SaveDefaultColumns() override;
	void SaveApplicationToolbar() override;
	void SaveColorRules() override;
	void SaveDialogStates() override;

private:
	void InitializeSaveEnvironment();
	void ReleaseSaveEnvironment();

	App *const m_app;
	Explorerplusplus *m_pContainer;

	wil::com_ptr_nothrow<IXMLDOMDocument> m_pXMLDom;
	wil::com_ptr_nothrow<IXMLDOMElement> m_pRoot;
};
