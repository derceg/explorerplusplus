// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LoadSaveXml.h"
#include "ColorRuleHelper.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/XMLSettings.h"

LoadSaveXML::LoadSaveXML(Explorerplusplus *pContainer, BOOL bLoad) :
	m_pContainer(pContainer),
	m_bLoad(bLoad)
{
	if (bLoad)
	{
		/* Initialize the load environment (namely,
		load the configuration file). */
		InitializeLoadEnvironment();
	}
	else
	{
		/* Initialize the save environment. */
		InitializeSaveEnvironment();
	}
}

LoadSaveXML::~LoadSaveXML()
{
	if (m_bLoad)
		ReleaseLoadEnvironment();
	else
		ReleaseSaveEnvironment();
}

void LoadSaveXML::InitializeLoadEnvironment()
{
	TCHAR szConfigFile[MAX_PATH];
	VARIANT_BOOL status;
	VARIANT var;

	m_bLoadedCorrectly = FALSE;

	m_pXMLDom = NXMLSettings::DomFromCOM();

	if (!m_pXMLDom)
		goto clean;

	GetProcessImageName(GetCurrentProcessId(), szConfigFile, SIZEOF_ARRAY(szConfigFile));
	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, NExplorerplusplus::XML_FILENAME);

	var = NXMLSettings::VariantString(NExplorerplusplus::XML_FILENAME);
	m_pXMLDom->load(var, &status);

	if (status != VARIANT_TRUE)
		goto clean;

	m_bLoadedCorrectly = TRUE;

clean:
	if (&var) VariantClear(&var);

	return;
}

void LoadSaveXML::ReleaseLoadEnvironment()
{
	if (m_bLoadedCorrectly)
	{
		m_pXMLDom->Release();
		m_pXMLDom = nullptr;
	}
}

void LoadSaveXML::InitializeSaveEnvironment()
{
	IXMLDOMProcessingInstruction *pi = nullptr;
	IXMLDOMComment *pc = nullptr;
	BSTR									bstr = nullptr;
	BSTR									bstr1 = nullptr;
	BSTR									bstr_wsnt = SysAllocString(L"\n\t");

	m_pXMLDom = NXMLSettings::DomFromCOM();

	if (!m_pXMLDom)
		goto clean;

	/* Insert the XML header. */
	bstr = SysAllocString(L"xml");
	bstr1 = SysAllocString(L"version='1.0'");
	m_pXMLDom->createProcessingInstruction(bstr, bstr1, &pi);
	NXMLSettings::AppendChildToParent(pi, m_pXMLDom);

	pi->Release();
	pi = nullptr;
	SysFreeString(bstr);
	bstr = nullptr;
	SysFreeString(bstr1);
	bstr1 = nullptr;

	/* Short header comment, explaining file purpose. */
	bstr = SysAllocString(L" Preference file for Explorer++ ");
	m_pXMLDom->createComment(bstr, &pc);
	NXMLSettings::AppendChildToParent(pc, m_pXMLDom);
	SysFreeString(bstr);
	bstr = nullptr;
	pc->Release();
	pc = nullptr;

	/* Create the root element. CANNOT use '+' signs
	within the element name. */
	bstr = SysAllocString(L"ExplorerPlusPlus");
	m_pXMLDom->createElement(bstr, &m_pRoot);
	SysFreeString(bstr);
	bstr = nullptr;

	NXMLSettings::AppendChildToParent(m_pRoot, m_pXMLDom);

	NXMLSettings::AddWhiteSpaceToNode(m_pXMLDom, bstr_wsnt, m_pRoot);

clean:
	if (bstr) SysFreeString(bstr);
	if (bstr1) SysFreeString(bstr1);

	if (pi) pi->Release();
	if (pc) pc->Release();
}

void LoadSaveXML::ReleaseSaveEnvironment()
{
	HANDLE	hProcess;
	TCHAR	szConfigFile[MAX_PATH];
	DWORD	dwProcessId;
	BSTR	bstr_wsn = SysAllocString(L"\n");
	BSTR	bstr = nullptr;
	VARIANT	var;

	NXMLSettings::AddWhiteSpaceToNode(m_pXMLDom, bstr_wsn, m_pRoot);

	m_pXMLDom->get_xml(&bstr);

	/* To ensure the configuration file is saved to the same directory
	as the executable, determine the fully qualified path of the executable,
	then save the configuration file in that directory. */
	dwProcessId = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);
	GetModuleFileNameEx(hProcess, nullptr, szConfigFile, SIZEOF_ARRAY(szConfigFile));
	CloseHandle(hProcess);

	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, NExplorerplusplus::XML_FILENAME);

	var = NXMLSettings::VariantString(szConfigFile);
	m_pXMLDom->save(var);

	m_pRoot->Release();
	m_pRoot = nullptr;

	m_pXMLDom->Release();
	m_pXMLDom = nullptr;
}

void LoadSaveXML::LoadGenericSettings()
{
	m_pContainer->LoadGenericSettingsFromXML(m_pXMLDom);
}

void LoadSaveXML::LoadBookmarks()
{
	m_pContainer->LoadBookmarksFromXML(m_pXMLDom);
}

int LoadSaveXML::LoadPreviousTabs()
{
	return m_pContainer->LoadTabSettingsFromXML(m_pXMLDom);
}

void LoadSaveXML::LoadDefaultColumns()
{
	m_pContainer->LoadDefaultColumnsFromXML(m_pXMLDom);
}

void LoadSaveXML::LoadApplicationToolbar()
{
	m_pContainer->LoadApplicationToolbarFromXML(m_pXMLDom);
}

void LoadSaveXML::LoadToolbarInformation()
{
	m_pContainer->LoadToolbarInformationFromXML(m_pXMLDom);
}

void LoadSaveXML::LoadColorRules()
{
	NColorRuleHelper::LoadColorRulesFromXML(m_pXMLDom, m_pContainer->m_ColorRules);
}

void LoadSaveXML::LoadDialogStates()
{
	m_pContainer->LoadDialogStatesFromXML(m_pXMLDom);
}

void LoadSaveXML::SaveGenericSettings()
{
	m_pContainer->SaveGenericSettingsToXML(m_pXMLDom, m_pRoot);
}

void LoadSaveXML::SaveBookmarks()
{
	m_pContainer->SaveBookmarksToXML(m_pXMLDom, m_pRoot);
}

void LoadSaveXML::SaveTabs()
{
	m_pContainer->SaveTabSettingsToXML(m_pXMLDom, m_pRoot);
}

void LoadSaveXML::SaveDefaultColumns()
{
	m_pContainer->SaveDefaultColumnsToXML(m_pXMLDom, m_pRoot);
}

void LoadSaveXML::SaveApplicationToolbar()
{
	m_pContainer->SaveApplicationToolbarToXML(m_pXMLDom, m_pRoot);
}

void LoadSaveXML::SaveToolbarInformation()
{
	m_pContainer->SaveToolbarInformationToXML(m_pXMLDom, m_pRoot);
}

void LoadSaveXML::SaveColorRules()
{
	NColorRuleHelper::SaveColorRulesToXML(m_pXMLDom, m_pRoot, m_pContainer->m_ColorRules);
}

void LoadSaveXML::SaveDialogStates()
{
	m_pContainer->SaveDialogStatesToXML(m_pXMLDom, m_pRoot);
}