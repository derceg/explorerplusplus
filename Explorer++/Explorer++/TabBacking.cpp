// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabBacking.h"
#include "CoreInterface.h"
#include "MainFontSetter.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "TabContainer.h"
#include "TabEvents.h"
#include "ToolbarHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"

TabBacking *TabBacking::Create(HWND parent, BrowserWindow *browser, CoreInterface *coreInterface,
	const ResourceLoader *resourceLoader, const Config *config, TabEvents *tabEvents)
{
	return new TabBacking(parent, browser, coreInterface, resourceLoader, config, tabEvents);
}

TabBacking::TabBacking(HWND parent, BrowserWindow *browser, CoreInterface *coreInterface,
	const ResourceLoader *resourceLoader, const Config *config, TabEvents *tabEvents) :
	m_hwnd(CreateWindow(WC_STATIC, L"",
		WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_NOTIFY, 0, 0, 0, 0, parent,
		nullptr, GetModuleHandle(nullptr), nullptr)),
	m_browser(browser),
	m_coreInterface(coreInterface)
{
	std::tie(m_toolbar, m_toolbarImageList) = ToolbarHelper::CreateCloseButtonToolbar(m_hwnd,
		CLOSE_BUTTON_ID, resourceLoader->LoadString(IDS_TAB_CLOSE_TIP), resourceLoader);

	SIZE toolbarSize;
	auto sizeRes = SendMessage(m_toolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&toolbarSize));
	DCHECK(sizeRes);
	SetWindowPos(m_toolbar, nullptr, 0, 0, toolbarSize.cx, toolbarSize.cy,
		SWP_NOZORDER | SWP_NOMOVE);

	m_toolbarTooltipFontSetter = std::make_unique<MainFontSetter>(
		reinterpret_cast<HWND>(SendMessage(m_toolbar, TB_GETTOOLTIPS, 0, 0)), config);

	m_connections.push_back(m_browser->AddLifecycleStateChangedObserver(
		std::bind_front(&TabBacking::OnBrowserLifecycleStateChanged, this)));

	m_connections.push_back(tabEvents->AddCreatedObserver(
		std::bind(&TabBacking::UpdateToolbar, this), TabEventScope::ForBrowser(*m_browser)));
	m_connections.push_back(tabEvents->AddSelectedObserver(
		std::bind(&TabBacking::UpdateToolbar, this), TabEventScope::ForBrowser(*m_browser)));
	m_connections.push_back(tabEvents->AddRemovedObserver(
		std::bind(&TabBacking::UpdateToolbar, this), TabEventScope::ForBrowser(*m_browser)));
	m_connections.push_back(tabEvents->AddUpdatedObserver(
		std::bind_front(&TabBacking::OnTabUpdated, this), TabEventScope::ForBrowser(*m_browser)));

	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&TabBacking::WndProc, this)));
}

HWND TabBacking::GetHWND() const
{
	return m_hwnd;
}

void TabBacking::OnBrowserLifecycleStateChanged(BrowserWindow::LifecycleState updatedState)
{
	if (updatedState == BrowserWindow::LifecycleState::Main)
	{
		UpdateToolbar();
	}
}

void TabBacking::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LockState:
		if (m_coreInterface->GetTabContainer()->IsTabSelected(tab))
		{
			UpdateToolbar();
		}
		break;

	case Tab::PropertyType::Name:
		break;
	}
}

void TabBacking::UpdateToolbar()
{
	if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
	{
		return;
	}

	auto *tabContainer = m_coreInterface->GetTabContainer();
	const Tab &selectedTab = tabContainer->GetSelectedTab();

	if (selectedTab.GetLockState() == Tab::LockState::NotLocked)
	{
		SendMessage(m_toolbar, TB_SETSTATE, CLOSE_BUTTON_ID, TBSTATE_ENABLED);
	}
	else
	{
		SendMessage(m_toolbar, TB_SETSTATE, CLOSE_BUTTON_ID, TBSTATE_INDETERMINATE);
	}
}

LRESULT TabBacking::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		OnCommand(wParam, lParam);
		break;

	case WM_SIZE:
		UpdateLayout();
		break;

	case WM_NCDESTROY:
		OnNcDestroy();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void TabBacking::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		switch (LOWORD(wParam))
		{
		case CLOSE_BUTTON_ID:
			OnCloseButtonClicked();
			break;
		}
	}
}

void TabBacking::OnCloseButtonClicked()
{
	auto *tabContainer = m_coreInterface->GetTabContainer();
	tabContainer->CloseTab(tabContainer->GetSelectedTab());
}

void TabBacking::UpdateLayout()
{
	RECT backingRect;
	auto res = GetClientRect(m_hwnd, &backingRect);
	DCHECK(res);

	RECT toolbarRect;
	res = GetClientRect(m_toolbar, &toolbarRect);
	DCHECK(res);

	res = SetWindowPos(m_toolbar, nullptr,
		GetRectWidth(&backingRect) - GetRectWidth(&toolbarRect)
			- DpiCompatibility::GetInstance().ScaleValue(m_toolbar,
				ToolbarHelper::CLOSE_TOOLBAR_X_OFFSET),
		(GetRectHeight(&backingRect) - GetRectHeight(&toolbarRect)) / 2, 0, 0,
		SWP_NOZORDER | SWP_NOSIZE);
	DCHECK(res);
}

void TabBacking::OnNcDestroy()
{
	delete this;
}
