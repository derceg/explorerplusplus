// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "HolderWindow.h"
#include "IconResourceLoader.h"
#include "MainToolbar.h"
#include "MainToolbarButtons.h"
#include "Runtime.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"

/* TODO: This should be linked to OnSize(). */
void Explorerplusplus::SetListViewInitialPosition(HWND hListView)
{
	RECT rc;
	int mainWindowWidth;
	int mainWindowHeight;
	int indentBottom = 0;
	int indentTop = 0;
	int indentLeft = 0;
	int indentRight = 0;
	int indentRebar = 0;

	GetClientRect(m_hContainer, &rc);

	mainWindowWidth = GetRectWidth(&rc);
	mainWindowHeight = GetRectHeight(&rc);

	if (m_hMainRebar)
	{
		GetWindowRect(m_hMainRebar, &rc);
		indentRebar += GetRectHeight(&rc);
	}

	if (m_config->showStatusBar)
	{
		GetWindowRect(m_hStatusBar, &rc);
		indentBottom += GetRectHeight(&rc);
	}

	if (m_config->showDisplayWindow.get())
	{
		if (m_config->displayWindowVertical)
		{
			indentRight += m_displayWindowWidth;
		}
		else
		{
			indentBottom += m_displayWindowHeight;
		}
	}

	if (m_config->showFolders.get())
	{
		GetClientRect(m_treeViewHolder->GetHWND(), &rc);
		indentLeft = GetRectWidth(&rc);
	}

	indentTop = indentRebar;

	RECT tabWindowRect;
	GetClientRect(GetActivePane()->GetTabContainer()->GetHWND(), &tabWindowRect);

	int tabWindowHeight = GetRectHeight(&tabWindowRect);

	if (m_bShowTabBar)
	{
		if (!m_config->showTabBarAtBottom.get())
		{
			indentTop += tabWindowHeight;
		}
	}

	int width = mainWindowWidth - indentLeft - indentRight;
	int height = mainWindowHeight - indentTop - indentBottom;

	if (m_config->showTabBarAtBottom.get())
	{
		height -= tabWindowHeight;
	}

	SetWindowPos(hListView, nullptr, indentLeft, indentTop, width, height,
		SWP_HIDEWINDOW | SWP_NOZORDER);
}

void Explorerplusplus::ToggleFolders()
{
	m_config->showFolders = !m_config->showFolders.get();
	lShowWindow(m_treeViewHolder->GetHWND(), m_config->showFolders.get());
	UpdateLayout();
}

concurrencpp::null_result Explorerplusplus::ScheduleUpdateLayout()
{
	if (!m_browserInitialized || m_browserClosing)
	{
		co_return;
	}

	auto destroyed = m_destroyed;

	// This function is designed to be called from the UI thread and the call here will also resume
	// on the UI thread. Rather than immediately resuming, however, this call will result in a
	// message being posted. Therefore, this function will only resume once the message has been
	// processed.
	co_await concurrencpp::resume_on(m_app->GetRuntime()->GetUiThreadExecutor());

	if (*destroyed)
	{
		co_return;
	}

	UpdateLayout();
}

void Explorerplusplus::UpdateLayout()
{
	RECT rc;
	GetClientRect(m_hContainer, &rc);
	SendMessage(m_hContainer, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
}

void Explorerplusplus::ToggleDualPane()
{
	m_config->dualPane = !m_config->dualPane;
}
