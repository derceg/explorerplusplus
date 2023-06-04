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
#include "ResourceHelper.h"
#include "TabBacking.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"

void Explorerplusplus::CreateTabBacking()
{
	m_hTabBacking = CreateWindow(WC_STATIC, EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_NOTIFY, 0, 0, 0, 0,
		m_hContainer, nullptr, GetModuleHandle(nullptr), nullptr);

	/* Create the toolbar that will appear on the tab control.
	Only contains the close button used to close tabs. */
	m_hTabWindowToolbar = CreateTabToolbar(m_hTabBacking, TABTOOLBAR_CLOSE,
		ResourceHelper::LoadString(m_resourceInstance, IDS_TAB_CLOSE_TIP));

	AddTabsInitializedObserver(std::bind_front(&Explorerplusplus::OnTabsInitialized, this));
}

void Explorerplusplus::OnTabsInitialized()
{
	m_tabContainer->tabCreatedSignal.AddObserver(
		[this](int tabId, BOOL switchToNewTab)
		{
			UNREFERENCED_PARAMETER(tabId);
			UNREFERENCED_PARAMETER(switchToNewTab);

			UpdateTabToolbar();
		});

	m_tabContainer->tabUpdatedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabUpdated, this));

	m_tabContainer->tabSelectedSignal.AddObserver(
		[this](const Tab &tab)
		{
			UNREFERENCED_PARAMETER(tab);

			UpdateTabToolbar();
		});

	m_tabContainer->tabRemovedSignal.AddObserver(
		[this](int tabId)
		{
			UNREFERENCED_PARAMETER(tabId);

			UpdateTabToolbar();
		});
}

void Explorerplusplus::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LockState:
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

	if (nTabs > 1 && selectedTab.GetLockState() == Tab::LockState::NotLocked)
	{
		/* Enable the tab close button. */
		SendMessage(m_hTabWindowToolbar, TB_SETSTATE, TABTOOLBAR_CLOSE, TBSTATE_ENABLED);
	}
	else
	{
		/* Disable the tab close toolbar button. */
		SendMessage(m_hTabWindowToolbar, TB_SETSTATE, TABTOOLBAR_CLOSE, TBSTATE_INDETERMINATE);
	}
}
