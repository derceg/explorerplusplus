// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryTracker.h"
#include "HistoryModel.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/NavigationRequest.h"

HistoryTracker::HistoryTracker(HistoryModel *historyModel, NavigationEvents *navigationEvents) :
	m_historyModel(historyModel)
{
	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&HistoryTracker::OnNavigationCommitted, this),
		NavigationEventScope::Global()));
}

void HistoryTracker::OnNavigationCommitted(const NavigationRequest *request)
{
	m_historyModel->AddHistoryItem(request->GetNavigateParams().pidl);
}
