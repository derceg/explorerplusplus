// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Manages the 'tab backing' panel, which sits
 * behind the tab control.
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"
#include "TabBacking.h"
#include "../Helper/Macros.h"

void Explorerplusplus::CreateTabBacking()
{
	m_hTabBacking = CreateWindow(WC_STATIC,EMPTY_STRING,
	WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SS_NOTIFY,
	0,0,0,0,m_hContainer,NULL,GetModuleHandle(0),NULL);

	/* Create the toolbar that will appear on the tab control.
	Only contains the close button used to close tabs. */
	TCHAR szTabCloseTip[64];
	LoadString(m_hLanguageModule, IDS_TAB_CLOSE_TIP, szTabCloseTip, SIZEOF_ARRAY(szTabCloseTip));
	m_hTabWindowToolbar = CreateTabToolbar(m_hTabBacking, TABTOOLBAR_CLOSE, szTabCloseTip);

	AddTabsInitializedObserver(boost::bind(&Explorerplusplus::OnTabsInitialized, this));
}

void Explorerplusplus::OnTabsInitialized()
{
	m_tabContainer->tabCreatedSignal.AddObserver([this](int tabId, BOOL switchToNewTab) {
		UNREFERENCED_PARAMETER(tabId);
		UNREFERENCED_PARAMETER(switchToNewTab);

		UpdateTabToolbar();
	});

	m_tabContainer->tabUpdatedSignal.AddObserver(boost::bind(&Explorerplusplus::OnTabUpdated, this, _1, _2));

	m_tabContainer->tabSelectedSignal.AddObserver([this](const Tab &tab) {
		UNREFERENCED_PARAMETER(tab);

		UpdateTabToolbar();
	});

	m_tabContainer->tabRemovedSignal.AddObserver([this](int tabId) {
		UNREFERENCED_PARAMETER(tabId);

		UpdateTabToolbar();
	});
}

void Explorerplusplus::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LOCKED:
	case Tab::PropertyType::ADDRESS_LOCKED:
		/* If the tab that was locked/unlocked is the
		currently selected tab, then the tab close
		button on the toolbar will need to be updated. */
		if (m_tabContainer->IsTabSelected(tab))
		{
			UpdateTabToolbar();
		}
		break;
	}
}

void Explorerplusplus::UpdateTabToolbar()
{
	const int nTabs = m_tabContainer->GetNumTabs();

	const Tab &selectedTab = m_tabContainer->GetSelectedTab();

	if (nTabs > 1 && !(selectedTab.GetLocked() || selectedTab.GetAddressLocked()))
	{
		/* Enable the tab close button. */
		SendMessage(m_hTabWindowToolbar, TB_SETSTATE,
			TABTOOLBAR_CLOSE, TBSTATE_ENABLED);
	}
	else
	{
		/* Disable the tab close toolbar button. */
		SendMessage(m_hTabWindowToolbar, TB_SETSTATE,
			TABTOOLBAR_CLOSE, TBSTATE_INDETERMINATE);
	}
}