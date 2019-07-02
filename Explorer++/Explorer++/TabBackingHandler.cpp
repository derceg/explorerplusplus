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