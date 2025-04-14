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
#include "ResourceLoader.h"
#include "TabBacking.h"
#include "TabContainerImpl.h"
#include "ToolbarHelper.h"

void Explorerplusplus::CreateTabBacking()
{
	m_hTabBacking = CreateWindow(WC_STATIC, L"",
		WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_NOTIFY, 0, 0, 0, 0,
		m_hContainer, nullptr, GetModuleHandle(nullptr), nullptr);

	/* Create the toolbar that will appear on the tab control.
	Only contains the close button used to close tabs. */
	std::tie(m_hTabWindowToolbar, m_tabWindowToolbarImageList) =
		ToolbarHelper::CreateCloseButtonToolbar(m_hTabBacking, TABTOOLBAR_CLOSE,
			m_app->GetResourceLoader()->LoadString(IDS_TAB_CLOSE_TIP), m_app->GetResourceLoader());

	SIZE toolbarSize;
	[[maybe_unused]] auto sizeRes =
		SendMessage(m_hTabWindowToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&toolbarSize));
	assert(sizeRes);
	SetWindowPos(m_hTabWindowToolbar, nullptr, 0, 0, toolbarSize.cx, toolbarSize.cy,
		SWP_NOZORDER | SWP_NOMOVE);

	m_tabToolbarTooltipFontSetter = std::make_unique<MainFontSetter>(
		reinterpret_cast<HWND>(SendMessage(m_hTabWindowToolbar, TB_GETTOOLTIPS, 0, 0)), m_config);

	m_connections.push_back(m_app->GetTabEvents()->AddCreatedObserver(
		[this](const Tab &tab, bool selected)
		{
			UNREFERENCED_PARAMETER(tab);
			UNREFERENCED_PARAMETER(selected);

			UpdateTabToolbar();
		},
		TabEventScope::ForBrowser(*this)));

	m_connections.push_back(m_app->GetTabEvents()->AddSelectedObserver(
		[this](const Tab &tab)
		{
			UNREFERENCED_PARAMETER(tab);

			UpdateTabToolbar();
		},
		TabEventScope::ForBrowser(*this)));

	m_connections.push_back(m_app->GetTabEvents()->AddRemovedObserver(
		[this](const Tab &tab)
		{
			UNREFERENCED_PARAMETER(tab);

			UpdateTabToolbar();
		},
		TabEventScope::ForBrowser(*this)));

	m_connections.push_back(m_app->GetTabEvents()->AddUpdatedObserver(
		std::bind_front(&Explorerplusplus::OnTabUpdated, this), TabEventScope::ForBrowser(*this)));
}

void Explorerplusplus::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LockState:
		/* If the tab that was locked/unlocked is the
		currently selected tab, then the tab close
		button on the toolbar will need to be updated. */
		if (GetActivePane()->GetTabContainerImpl()->IsTabSelected(tab))
		{
			UpdateTabToolbar();
		}
		break;
	}
}

void Explorerplusplus::UpdateTabToolbar()
{
	const int nTabs = GetActivePane()->GetTabContainerImpl()->GetNumTabs();

	const Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();

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
