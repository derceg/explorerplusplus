// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Manages the 'tab backing' panel, which sits
 * behind the tab control.
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "TabBacking.h"
#include "TabContainer.h"
#include "ToolbarHelper.h"
#include "../Helper/Macros.h"

void Explorerplusplus::CreateTabBacking()
{
	m_hTabBacking = CreateWindow(WC_STATIC, EMPTY_STRING,
		WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_NOTIFY, 0, 0, 0, 0,
		m_hContainer, nullptr, GetModuleHandle(nullptr), nullptr);

	/* Create the toolbar that will appear on the tab control.
	Only contains the close button used to close tabs. */
	std::tie(m_hTabWindowToolbar, m_tabWindowToolbarImageList) =
		ToolbarHelper::CreateCloseButtonToolbar(m_hTabBacking, TABTOOLBAR_CLOSE,
			ResourceHelper::LoadString(m_resourceInstance, IDS_TAB_CLOSE_TIP),
			m_app->GetIconResourceLoader());

	SIZE toolbarSize;
	[[maybe_unused]] auto sizeRes =
		SendMessage(m_hTabWindowToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&toolbarSize));
	assert(sizeRes);
	SetWindowPos(m_hTabWindowToolbar, nullptr, 0, 0, toolbarSize.cx, toolbarSize.cy,
		SWP_NOZORDER | SWP_NOMOVE);

	m_tabToolbarTooltipFontSetter = std::make_unique<MainFontSetter>(
		reinterpret_cast<HWND>(SendMessage(m_hTabWindowToolbar, TB_GETTOOLTIPS, 0, 0)), m_config);

	AddTabsInitializedObserver(std::bind_front(&Explorerplusplus::OnTabsInitialized, this));
}

void Explorerplusplus::OnTabsInitialized()
{
	GetActivePane()->GetTabContainer()->tabCreatedSignal.AddObserver(
		[this](int tabId, BOOL switchToNewTab)
		{
			UNREFERENCED_PARAMETER(tabId);
			UNREFERENCED_PARAMETER(switchToNewTab);

			UpdateTabToolbar();
		});

	GetActivePane()->GetTabContainer()->tabUpdatedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabUpdated, this));

	GetActivePane()->GetTabContainer()->tabSelectedSignal.AddObserver(
		[this](const Tab &tab)
		{
			UNREFERENCED_PARAMETER(tab);

			UpdateTabToolbar();
		});

	GetActivePane()->GetTabContainer()->tabRemovedSignal.AddObserver(
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
		if (GetActivePane()->GetTabContainer()->IsTabSelected(tab))
		{
			UpdateTabToolbar();
		}
		break;
	}
}

void Explorerplusplus::UpdateTabToolbar()
{
	const int nTabs = GetActivePane()->GetTabContainer()->GetNumTabs();

	const Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();

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
