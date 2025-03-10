// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryShellBrowserHelper.h"
#include "HistoryModel.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/NavigationRequest.h"

HistoryShellBrowserHelper::HistoryShellBrowserHelper(ShellBrowser *shellBrowser,
	HistoryModel *historyModel, NavigationEvents *navigationEvents) :
	ShellBrowserHelper(shellBrowser),
	m_historyModel(historyModel)
{
	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&HistoryShellBrowserHelper::OnNavigationCommitted, this),
		NavigationEventScope::ForShellBrowser(*shellBrowser)));
}

void HistoryShellBrowserHelper::OnNavigationCommitted(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(shellBrowser);

	m_historyModel->AddHistoryItem(request->GetNavigateParams().pidl);
}
